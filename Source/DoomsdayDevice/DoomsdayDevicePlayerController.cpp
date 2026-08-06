// Copyright Epic Games, Inc. All Rights Reserved.


#include "DoomsdayDevicePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "DoomsdayDeviceCameraManager.h"
#include "DoomsdayDeviceCharacter.h"
#include "Blueprint/UserWidget.h"
#include "DoomsdayDevice.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "Gameplay/InteractionComponent.h"
#include "Gameplay/InventorySubsystem.h"
#include "Flow/FactsDBSubsystem.h"
#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"
#include "Player/PlayerTags.h"

ADoomsdayDevicePlayerController::ADoomsdayDevicePlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ADoomsdayDeviceCameraManager::StaticClass();

	UInteractionComponent::OnPlayerEnter.AddUObject(this, &ADoomsdayDevicePlayerController::OnInteractionEnter);
	UInteractionComponent::OnPlayerExit.AddUObject(this, &ADoomsdayDevicePlayerController::OnInteractionExit);
}

void ADoomsdayDevicePlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogDoomsdayDevice, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// if a tool was already collected (e.g. after a level transition), show the slots bar;
	// the widget pulls unlock/selection state itself on construct
	if (IsLocalPlayerController())
	{
		if (UBasicUIManager* UIManager = GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			// the HUD carries the interaction reticle, so it must not depend on the inventory
			UIManager->DisplayHUD();

			const UInventorySubsystem* Inventory = GetGameInstance() ? GetGameInstance()->GetSubsystem<UInventorySubsystem>() : nullptr;
			if (Inventory)
			{
				for (const FToolSlotDefinition& Slot : GetDefault<UPlayerSettings>()->ToolSlots)
				{
					if (Inventory->HasItem(Slot.ToolTag))
					{
						UIManager->OpenWidget(GetDefault<UPlayerSettings>()->ToolSlotsWidget);
						break;
					}
				}
			}
		}

		// drive the dialogue hint widget from the "hint available" fact
		if (UFactsDBSubsystem* FactsDB = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFactsDBSubsystem>() : nullptr)
		{
			FactsDB->OnFactChanged.RemoveAll(this);
			FactsDB->OnFactChanged.AddUObject(this, &ADoomsdayDevicePlayerController::OnHintFactChanged);

			if (HintFactTag.IsValid())
			{
				// sync initial visibility (e.g. after a level transition where the fact is already set)
				OnHintFactChanged(HintFactTag, FactsDB->GetFactValue(HintFactTag));
			}
		}

		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void ADoomsdayDevicePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFactsDBSubsystem* FactsDB = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFactsDBSubsystem>() : nullptr)
	{
		FactsDB->OnFactChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ADoomsdayDevicePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			EnhancedInputComponent->BindAction(InteractionAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnInteractionUsed);
			if (DropAction)
			{
				EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDropUsed);
			}
			EnhancedInputComponent->BindAction(ContinueDialogueAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueContinued);
			EnhancedInputComponent->BindAction(DialogueHintAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueHintUsed);

			EnhancedInputComponent->BindAction(SelectFirstChoiceAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueChoiceSelected, 0);
			EnhancedInputComponent->BindAction(SelectSecondChoiceAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueChoiceSelected, 1);
			EnhancedInputComponent->BindAction(SelectThirdChoiceAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueChoiceSelected, 2);
			EnhancedInputComponent->BindAction(SelectFourthChoiceAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueChoiceSelected, 3);

			for (int32 SlotIndex = 0; SlotIndex < ToolSlotActions.Num(); ++SlotIndex)
			{
				if (ToolSlotActions[SlotIndex])
				{
					EnhancedInputComponent->BindAction(ToolSlotActions[SlotIndex], ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnToolSlotPressed, SlotIndex);
				}
			}
		}
	}
	
}

bool ADoomsdayDevicePlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ADoomsdayDevicePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (PossibleInteractions.Num() > 0)
	{
		const FVector CameraLocation = PlayerCameraManager->GetCameraLocation();
		PossibleInteractions.Sort([CameraLocation](const TWeakObjectPtr<UInteractionComponent>& A, const TWeakObjectPtr<UInteractionComponent>& B)
			{
				const float DistanceA = FVector::DistSquared(CameraLocation, A.Get()->GetComponentLocation());
				const float DistanceB = FVector::DistSquared(CameraLocation, B.Get()->GetComponentLocation());
				return DistanceA < DistanceB;
			});

		if (!ActiveInteraction.IsValid() && ActiveInteraction != PossibleInteractions[0])
		{
			ActivateInteraction(PossibleInteractions[0]);
		}
	}
	else if (ActiveInteraction.IsValid())
	{
		DeactivateInteraction();
	}

	// the prompt flips without re-targeting: equipping/unequipping a tool, StartCarry auto-stowing
	// one, or an EvaluatePrompt override changing its mind. Cheap when nothing changed.
	RefreshInteractionPrompt();
}

void ADoomsdayDevicePlayerController::OnInteractionEnter(const TWeakObjectPtr<UInteractionComponent> Interaction)
{
	PossibleInteractions.Add(Interaction);
}

void ADoomsdayDevicePlayerController::OnInteractionExit(const TWeakObjectPtr<UInteractionComponent> Interaction)
{
	if (ActiveInteraction.IsValid() && ActiveInteraction == Interaction)
	{
		DeactivateInteraction();
	}

	PossibleInteractions.Remove(Interaction);
}

void ADoomsdayDevicePlayerController::ActivateInteraction(const TWeakObjectPtr<UInteractionComponent> Interaction)
{
	if (ActiveInteraction.IsValid())
	{
		DeactivateInteraction();
	}

	ActiveInteraction = Interaction;
	GetLocalPlayer()->GetSubsystem<UBasicUIManager>()->OpenWidget(GetDefault<UPlayerSettings>()->InteractionWidget);

	// OpenWidget ran the widget's NativeConstruct, which pulled whatever the cache held at the time;
	// force a push so the brand new widget is correct on its first frame
	bPromptPushed = false;
	RefreshInteractionPrompt();
}

void ADoomsdayDevicePlayerController::DeactivateInteraction()
{
	ActiveInteraction = nullptr;
	GetLocalPlayer()->GetSubsystem<UBasicUIManager>()->CloseWidget(GetDefault<UPlayerSettings>()->InteractionWidget);

	// resets the reticle to Idle
	RefreshInteractionPrompt();
}

void ADoomsdayDevicePlayerController::RefreshInteractionPrompt()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UBasicUIManager* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UBasicUIManager>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	// checked, unlike the sort in PlayerTick: a stale interaction clears the prompt instead of crashing
	const UInteractionComponent* Interaction = ActiveInteraction.Get();
	if (!Interaction)
	{
		if (bPromptPushed)
		{
			bPromptPushed = false;
			PromptSource = nullptr;
			CurrentPrompt = FInteractionPrompt();

			UIManager->NotifyInteractionPromptCleared();
		}

		return;
	}

	const ADoomsdayDeviceCharacter* PlayerCharacter = Cast<ADoomsdayDeviceCharacter>(GetPawn());
	const FGameplayTag EquippedToolTag = PlayerCharacter ? PlayerCharacter->GetEquippedToolTag() : FGameplayTag();

	const FInteractionPrompt NewPrompt = Interaction->EvaluatePrompt(EquippedToolTag);

	// the PromptSource term re-pushes when targeting switches to a different component that happens
	// to produce the same text
	if (bPromptPushed && PromptSource == ActiveInteraction && NewPrompt.IsEquivalentTo(CurrentPrompt))
	{
		return;
	}

	CurrentPrompt = NewPrompt;
	PromptSource = ActiveInteraction;
	bPromptPushed = true;

	UE_LOG(LogDoomsdayDevice, Verbose, TEXT("Interaction prompt: '%s' (CanUse=%d)"), *CurrentPrompt.PromptText.ToString(), CurrentPrompt.bCanUse ? 1 : 0);

	UIManager->NotifyInteractionPromptChanged(CurrentPrompt);
}

void ADoomsdayDevicePlayerController::OnInteractionUsed()
{
	if (ActiveInteraction.IsValid())
	{
		ADoomsdayDeviceCharacter* PlayerCharacter = Cast<ADoomsdayDeviceCharacter>(GetPawn());
		const FGameplayTag EquippedToolTag = PlayerCharacter ? PlayerCharacter->GetEquippedToolTag() : FGameplayTag();

		// same evaluation the prompt shows, so an EvaluatePrompt override that blocks for a new
		// reason also blocks the press - the two can never disagree. Re-evaluated rather than read
		// from CurrentPrompt to cover a press landing before the first tick after activation.
		if (ActiveInteraction->EvaluatePrompt(EquippedToolTag).bCanUse)
		{
			// animate only for interactions that actually required a tool; the matched tool is guaranteed equipped here
			if (PlayerCharacter && ActiveInteraction->RequiredToolTag.IsValid())
			{
				PlayerCharacter->PlayEquippedToolUseMontage();
			}

			ActiveInteraction->OnUsed.Broadcast();
		}
		else
		{
			ActiveInteraction->OnUseDenied.Broadcast();
			OnInteractionUseDenied.Broadcast(ActiveInteraction->RequiredToolTag);
		}
	}
	else
	{
		// no interaction targeted: pressing "use" while carrying a heavy item drops it
		OnDropUsed();
	}
}

void ADoomsdayDevicePlayerController::OnDropUsed()
{
	if (ADoomsdayDeviceCharacter* PlayerCharacter = Cast<ADoomsdayDeviceCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsCarrying())
		{
			PlayerCharacter->DropCarriedItem();
		}
	}
}

void ADoomsdayDevicePlayerController::OnDialogueContinued()
{
	if (UBasicUIManager* UIManager = GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
	{
		// One press does both: cut the voice-over and snap the typewriter to full text.
		// Kept in separate locals on purpose - || would short-circuit and skip the reveal.
		const bool bStoppedVoice = UIManager->StopDialogueVoice();
		const bool bSkippedReveal = UIManager->SkipDialogueLineReveal();

		if (bStoppedVoice || bSkippedReveal)
		{
			// The press just finished presenting the line; on an auto-skip line that starts the delay
			// countdown. Needed for the case where only the voice-over was cut - a skipped reveal already
			// broadcasts on its own, and the repeat is harmless.
			UIManager->RefreshDialogueLinePresentation();

			// Press consumed: the line does not advance.
			return;
		}
	}

	ContinueDialogueEvent.Broadcast();
}

void ADoomsdayDevicePlayerController::OnDialogueChoiceSelected(const FInputActionValue& Value, int32 Index)
{
	SelectDialogueChoiceEvent.Broadcast(Index);
}

void ADoomsdayDevicePlayerController::OnDialogueHintUsed()
{
	// the hint input is only meaningful while a hint is on screen (widget visibility mirrors the fact)
	const UBasicUIManager* UIManager = GetLocalPlayer()->GetSubsystem<UBasicUIManager>();
	if (UIManager && UIManager->IsWidgetOpen(GetDefault<UPlayerSettings>()->DialogueHintWidget))
	{
		DialogueHintEvent.Broadcast();
	}
}

void ADoomsdayDevicePlayerController::OnHintFactChanged(const FGameplayTag& ChangedTag, int32 NewValue)
{
	if (!HintFactTag.IsValid() || ChangedTag != HintFactTag)
	{
		return;
	}

	if (UBasicUIManager* UIManager = GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
	{
		const TSoftClassPtr<UUserWidget> HintWidget = GetDefault<UPlayerSettings>()->DialogueHintWidget;
		if (NewValue > 0)
		{
			UIManager->OpenWidget(HintWidget);
		}
		else
		{
			UIManager->CloseWidget(HintWidget);
		}
	}
}

void ADoomsdayDevicePlayerController::OnToolSlotPressed(const FInputActionValue& Value, int32 SlotIndex)
{
	// keys 1-4 are shared with the dialogue choice actions; dialogue wins while its screen is open
	if (const UBasicUIManager* UIManager = GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
	{
		if (UIManager->IsWidgetOpen(GetDefault<UPlayerSettings>()->DialogueWidget))
		{
			return;
		}
	}

	if (ADoomsdayDeviceCharacter* PlayerCharacter = Cast<ADoomsdayDeviceCharacter>(GetPawn()))
	{
		PlayerCharacter->ToggleToolSlot(SlotIndex);
	}
}
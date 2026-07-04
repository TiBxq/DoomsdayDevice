// Copyright Epic Games, Inc. All Rights Reserved.


#include "DoomsdayDevicePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "DoomsdayDeviceCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "DoomsdayDevice.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "Gameplay/InteractionComponent.h"
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
			EnhancedInputComponent->BindAction(ContinueDialogueAction, ETriggerEvent::Started, this, &ADoomsdayDevicePlayerController::OnDialogueContinued);
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
}

void ADoomsdayDevicePlayerController::DeactivateInteraction()
{
	ActiveInteraction = nullptr;
	GetLocalPlayer()->GetSubsystem<UBasicUIManager>()->CloseWidget(GetDefault<UPlayerSettings>()->InteractionWidget);
}

void ADoomsdayDevicePlayerController::OnInteractionUsed()
{
	if (ActiveInteraction.IsValid())
	{
		ActiveInteraction->OnUsed.Broadcast();
	}
}

void ADoomsdayDevicePlayerController::OnDialogueContinued()
{
	ContinueDialogueEvent.Broadcast();
}
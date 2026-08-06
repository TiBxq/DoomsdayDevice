// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Player/BasicUIManager.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "Player/PlayerSettings.h"
#include "UI/DialogueWidget.h"
#include "UI/ToolSlotsWidget.h"
#include "UI/HUDWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicUIManager)

UBasicUIManager::UBasicUIManager()
{
}

void UBasicUIManager::OpenWidget(const TSoftClassPtr<UUserWidget> SoftClass)
{
	if (SoftClass == nullptr || OpenedWidgets.Contains(SoftClass))
	{
		return;
	}

	UClass* WidgetClass = SoftClass.LoadSynchronous();
	if (!WidgetClass || WidgetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}

	UUserWidget* Widget = Cast<UUserWidget>(UWidgetBlueprintLibrary::Create(GetWorld(), WidgetClass, GetWorld()->GetFirstPlayerController()));
	OpenedWidgets.Add(SoftClass, Widget);
	Widget->AddToViewport();
}

void UBasicUIManager::CloseWidget(const TSoftClassPtr<UUserWidget> SoftClass)
{
	if (UUserWidget* Widget = OpenedWidgets.FindRef(SoftClass))
	{
		Widget->RemoveFromParent();
		OpenedWidgets.Remove(SoftClass);
	}
}

void UBasicUIManager::ToggleWidget(const TSoftClassPtr<UUserWidget> SoftClass)
{
	if (OpenedWidgets.Contains(SoftClass))
	{
		CloseWidget(SoftClass);
	}
	else
	{
		OpenWidget(SoftClass);
	}
}

bool UBasicUIManager::IsWidgetOpen(const TSoftClassPtr<UUserWidget> SoftClass) const
{
	return OpenedWidgets.Contains(SoftClass);
}

void UBasicUIManager::HideWidgets()
{
	for (auto WidgetIt = OpenedWidgets.CreateIterator(); WidgetIt; ++WidgetIt)
	{
		const TPair<TSoftClassPtr<UUserWidget>, UUserWidget*> Widget = *WidgetIt;

		HiddenWidgets.Add(Widget.Key);
		CloseWidget(Widget.Key);
	}
}

void UBasicUIManager::RestoreWidgets()
{
	for (const TSoftClassPtr<UUserWidget>& WidgetClass : HiddenWidgets)
	{
		OpenWidget(WidgetClass);
	}
	HiddenWidgets.Empty();
}

void UBasicUIManager::DisplayHUD()
{
	GetHUDWidget(true); // widget will be created if it doesn't exist
}

void UBasicUIManager::DisplayDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData, USoundBase* VoiceOver)
{
	// Resolve the widget first so the first line's synchronous widget-class load doesn't offset the audio.
	UDialogueWidget* Widget = GetDialogueWidget(true);
	if (!Widget)
	{
		return;
	}

	StopDialogueVoice(); // never let a previous line's voice-over overlap the new one

	// Re-bound per line: EndDialogue destroys the widget, so the subscription can't outlive it.
	Widget->OnLineRevealCompletedNative.RemoveAll(this);
	Widget->OnLineRevealCompletedNative.AddUObject(this, &UBasicUIManager::OnDialogueLineRevealCompleted);

	if (VoiceOver)
	{
		if (UWorld* World = GetWorld())
		{
			DialogueVoiceComponent = UGameplayStatics::SpawnSound2D(World, VoiceOver, /*VolumeMultiplier*/ 1.f, /*PitchMultiplier*/ 1.f, /*StartTime*/ 0.f, /*ConcurrencySettings*/ nullptr, /*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ false);

			if (DialogueVoiceComponent)
			{
				DialogueVoiceComponent->OnAudioFinished.AddDynamic(this, &UBasicUIManager::OnDialogueVoiceFinished);
			}
		}
	}

	// Starts the typewriter reveal, so text and voice-over begin on the same frame. An instant reveal
	// completes inside this call and broadcasts right away - correctly, since the voice-over above is
	// already counted as playing by then.
	Widget->AddDialogueLine(LineText, SpeakerData);
}

void UBasicUIManager::SetupDialogueChoices(const TArray<FText>& ChoiceTexts)
{
	if (UDialogueWidget* Widget = GetDialogueWidget(true))
	{
		Widget->SetupDialogueChoices(ChoiceTexts);
	}
}

void UBasicUIManager::ClearDialogueChoices()
{
	if (UDialogueWidget* Widget = GetDialogueWidget(false))
	{
		Widget->ClearDialogueChoices();
	}
}

bool UBasicUIManager::SkipDialogueLineReveal()
{
	if (UDialogueWidget* Widget = GetDialogueWidget(false))
	{
		return Widget->SkipReveal();
	}
	return false;
}

bool UBasicUIManager::StopDialogueVoice()
{
	// IsValid also covers a component torn down with its world - this subsystem outlives the world.
	if (!IsValid(DialogueVoiceComponent))
	{
		DialogueVoiceComponent = nullptr;
		return false;
	}

	const bool bWasPlaying = DialogueVoiceComponent->IsPlaying();
	DialogueVoiceComponent->OnAudioFinished.RemoveAll(this);

	const float FadeSeconds = GetDefault<UPlayerSettings>()->DialogueVoiceStopFadeSeconds;
	if (FadeSeconds > 0.f)
	{
		DialogueVoiceComponent->bAutoDestroy = true; // the fade tail cleans itself up once we drop the reference
		DialogueVoiceComponent->FadeOut(FadeSeconds, 0.f);
	}
	else
	{
		DialogueVoiceComponent->Stop();
	}

	// Dropped immediately so IsDialogueVoicePlaying reports false even while a fade tail is still audible,
	// otherwise the next continue press would be eaten too.
	DialogueVoiceComponent = nullptr;
	return bWasPlaying;
}

bool UBasicUIManager::IsDialogueVoicePlaying() const
{
	return IsValid(DialogueVoiceComponent) && DialogueVoiceComponent->IsPlaying();
}

bool UBasicUIManager::IsDialogueLinePresented() const
{
	// Inlined rather than GetDialogueWidget(false) because this is const and must never open the widget.
	const TSoftClassPtr<UUserWidget> DialogueWidgetClass = GetDefault<UPlayerSettings>()->DialogueWidget;
	const UDialogueWidget* Widget = Cast<UDialogueWidget>(OpenedWidgets.FindRef(DialogueWidgetClass));

	return Widget && !Widget->IsRevealing() && !IsDialogueVoicePlaying();
}

void UBasicUIManager::RefreshDialogueLinePresentation()
{
	if (IsDialogueLinePresented())
	{
		// May fire more than once per line (the continue press finishes text and audio separately);
		// observers are expected to ignore repeats.
		OnDialogueLinePresented.Broadcast();
	}
}

void UBasicUIManager::OnDialogueVoiceFinished()
{
	// Natural end: release the handle so the state query stays accurate and the component can be collected.
	DialogueVoiceComponent = nullptr;

	RefreshDialogueLinePresentation();
}

void UBasicUIManager::OnDialogueLineRevealCompleted()
{
	RefreshDialogueLinePresentation();
}

void UBasicUIManager::NotifyToolSlotUnlocked(const int32 SlotIndex)
{
	if (UToolSlotsWidget* Widget = GetToolSlotsWidget(true))
	{
		Widget->SetSlotUnlocked(SlotIndex);
	}
}

void UBasicUIManager::NotifyEquippedToolChanged(const int32 NewSlotIndex)
{
	// equipping requires an unlocked slot, and unlocking opened the bar; if it is
	// currently closed (e.g. hidden for a cutscene) don't force it back open -
	// reopening re-pulls the selection state anyway
	if (UToolSlotsWidget* Widget = GetToolSlotsWidget(false))
	{
		Widget->SetEquippedSlot(NewSlotIndex);
	}
}

UToolSlotsWidget* UBasicUIManager::GetToolSlotsWidget(const bool bOpenIfNeeded)
{
	const TSoftClassPtr<UUserWidget> ToolSlotsWidgetClass = GetDefault<UPlayerSettings>()->ToolSlotsWidget;

	if (bOpenIfNeeded && !OpenedWidgets.Contains(ToolSlotsWidgetClass))
	{
		OpenWidget(ToolSlotsWidgetClass);
	}

	return Cast<UToolSlotsWidget>(OpenedWidgets.FindRef(ToolSlotsWidgetClass));
}

UDialogueWidget* UBasicUIManager::GetDialogueWidget(const bool bOpenIfNeeded)
{
	const TSoftClassPtr<UUserWidget> DialogueWidgetClass = GetDefault<UPlayerSettings>()->DialogueWidget;

	if (bOpenIfNeeded && !OpenedWidgets.Contains(DialogueWidgetClass))
	{
		OpenWidget(DialogueWidgetClass);
	}

	return Cast<UDialogueWidget>(OpenedWidgets.FindRef(DialogueWidgetClass));
}

UHUDWidget* UBasicUIManager::GetHUDWidget(bool bOpenIfNeeded)
{
	const TSoftClassPtr<UUserWidget> HUDWidgetClass = GetDefault<UPlayerSettings>()->HUDWidget;

	if (bOpenIfNeeded && !OpenedWidgets.Contains(HUDWidgetClass))
	{
		OpenWidget(HUDWidgetClass);
	}

	return Cast<UHUDWidget>(OpenedWidgets.FindRef(HUDWidgetClass));
}
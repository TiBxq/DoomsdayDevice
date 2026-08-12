// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Delegates/Delegate.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "BasicUIManager.generated.h"

class UUserWidget;
class UDialogSpeakerDataAsset;
class UDialogueWidget;
class UInteractionPromptWidget;
class UToolSlotsWidget;
class UHUDWidget;
class USoundBase;
class UAudioComponent;

struct FInteractionPrompt;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConfirmDialogueChoiceEvent, int32, Index);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueCloseFinished);

/**
 *
 */
UCLASS()
class UBasicUIManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UBasicUIManager();
	
private:
	UPROPERTY()
	TMap<TSoftClassPtr<UUserWidget>, TObjectPtr<UUserWidget>> OpenedWidgets;

	UPROPERTY()
	TArray<TSoftClassPtr<UUserWidget>> HiddenWidgets;

public:
	void OpenWidget(const TSoftClassPtr<UUserWidget> SoftClass);
	void CloseWidget(const TSoftClassPtr<UUserWidget> SoftClass);
	void ToggleWidget(const TSoftClassPtr<UUserWidget> SoftClass);
	bool IsWidgetOpen(const TSoftClassPtr<UUserWidget> SoftClass) const;

	void HideWidgets();
	void RestoreWidgets();

	// ----------- HUD --------------------

	void DisplayHUD();

	// ----------- Dialogue ---------------
	/** Displays a line; VoiceOver (optional) starts playing 2D alongside the typewriter reveal. */
	void DisplayDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData, USoundBase* VoiceOver = nullptr);

	void SetupDialogueChoices(const TArray<FText>& ChoiceTexts);
	void ClearDialogueChoices();
	void ConfirmDialogueChoice(int32 Index);

	/** Returns true if the open dialogue widget had a line reveal in progress and it was skipped. */
	bool SkipDialogueLineReveal();

	/** Stops the dialogue voice-over. Returns true if one was actually playing (press consumed). */
	bool StopDialogueVoice();

	/** True while a dialogue voice-over is audible. */
	bool IsDialogueVoicePlaying() const;

	/** True once the current line's reveal has finished and its voice-over is no longer audible. */
	bool IsDialogueLinePresented() const;

	/**
	 * Re-evaluates the presentation state and broadcasts OnDialogueLinePresented if the line is now complete.
	 * Call after anything that can finish a line early, e.g. the continue press cutting the voice-over.
	 * Inert while no dialogue widget is open.
	 */
	void RefreshDialogueLinePresentation();

	bool CloseDialogue();

	/** Fires once the current line has fully played: reveal complete and voice-over finished or cut. */
	FSimpleMulticastDelegate OnDialogueLinePresented;

	FConfirmDialogueChoiceEvent OnDialogueChoiceConfirmed;

	FOnDialogueCloseFinished OnDialogueCloseFinished;

	// ----------- Tools ---------------
	/** Opens the tool slots widget if needed and marks the slot unlocked. */
	void NotifyToolSlotUnlocked(int32 SlotIndex);

	/** Updates the tool slots widget selection; INDEX_NONE = empty hands. */
	void NotifyEquippedToolChanged(int32 NewSlotIndex);

	// ----------- Interaction ---------------
	/** Pushes the targeted interaction's prompt: text into the prompt widget, availability into the HUD reticle. */
	void NotifyInteractionPromptChanged(const FInteractionPrompt& Prompt);

	/** Nothing targeted: neutral reticle. Closing the prompt widget stays with the player controller. */
	void NotifyInteractionPromptCleared();

	void ShowInteractionWidget();
	void CloseInteractionWidget();

private:
	UDialogueWidget* GetDialogueWidget(bool bOpenIfNeeded);
	UToolSlotsWidget* GetToolSlotsWidget(bool bOpenIfNeeded);
	UHUDWidget* GetHUDWidget(bool bOpenIfNeeded);
	UInteractionPromptWidget* GetInteractionPromptWidget(bool bOpenIfNeeded);

	/** Playback handle for the current line's voice-over; kept referenced so it isn't GC'd while playing. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> DialogueVoiceComponent;

	UFUNCTION()
	void OnDialogueVoiceFinished();

	void OnDialogueLineRevealCompleted();
};

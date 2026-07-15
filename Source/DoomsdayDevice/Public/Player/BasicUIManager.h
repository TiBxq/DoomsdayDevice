// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "BasicUIManager.generated.h"

class UUserWidget;
class UDialogSpeakerDataAsset;
class UDialogueWidget;
class UToolSlotsWidget;

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

	// ----------- Dialogue ---------------
	void DisplayDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData);

	void SetupDialogueChoices(const TArray<FText>& ChoiceTexts);
	void ClearDialogueChoices();

	/** Returns true if the open dialogue widget had a line reveal in progress and it was skipped. */
	bool SkipDialogueLineReveal();

	// ----------- Tools ---------------
	/** Opens the tool slots widget if needed and marks the slot unlocked. */
	void NotifyToolSlotUnlocked(int32 SlotIndex);

	/** Updates the tool slots widget selection; INDEX_NONE = empty hands. */
	void NotifyEquippedToolChanged(int32 NewSlotIndex);

private:
	UDialogueWidget* GetDialogueWidget(bool bOpenIfNeeded);
	UToolSlotsWidget* GetToolSlotsWidget(bool bOpenIfNeeded);
};

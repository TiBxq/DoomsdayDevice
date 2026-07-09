// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "BasicUIManager.generated.h"

class UUserWidget;
class UDialogSpeakerDataAsset;

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

	void HideWidgets();
	void RestoreWidgets();

	// ----------- Dialogue ---------------
	void DisplayDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData);

	void SetupDialogueChoices(const TArray<FText>& ChoiceTexts);
	void ClearDialogueChoices();
};

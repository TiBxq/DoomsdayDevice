// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class UDialogSpeakerDataAsset;

/**
 * 
 */
UCLASS()
class DOOMSDAYDEVICE_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void AddDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData);

	void SetupDialogueChoices(const TArray<FText>& ChoiceTexts);

	void ClearDialogueChoices();

	UFUNCTION(BlueprintImplementableEvent)
	void DisplayDialogueLine(const FText& LineText, UDialogSpeakerDataAsset* SpeakerData);

	UFUNCTION(BlueprintImplementableEvent)
	void DisplayDialogueChoices(const TArray<FText>& ChoiceTexts);

	UFUNCTION(BlueprintImplementableEvent)
	void RemoveDialogueChoices();
};

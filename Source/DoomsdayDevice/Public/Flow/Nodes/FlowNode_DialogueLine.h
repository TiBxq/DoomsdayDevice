// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"

#include "FlowNode_DialogueLine.generated.h"

class UDialogSpeakerDataAsset;
class USoundBase;

/**
 * 
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Dialogue Line"))
class DOOMSDAYDEVICE_API UFlowNode_DialogueLine : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_DialogueLine();

	const FText& GetLineText() const { return LineText; }
	const FString& GetDialogueID() const { return DialogueID; }

#if WITH_EDITOR
	void SetDialogueID(const FString& InDialogueID);
#endif

protected:
	/** Stable export key, shared with the CSV dialogue export. Auto-filled by that tool while empty. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FString DialogueID;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText LineText;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TObjectPtr<UDialogSpeakerDataAsset> SpeakerData;

	/** Optional voice-over, played 2D when the line appears. The first continue press stops it. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TSoftObjectPtr<USoundBase> VoiceOver;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

	UFUNCTION()
	void OnDialogueLineCompleted();

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual void UpdateNodeConfigText_Implementation() override;
#endif
};

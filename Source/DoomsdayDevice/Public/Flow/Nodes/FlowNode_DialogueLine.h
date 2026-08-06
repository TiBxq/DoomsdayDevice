// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
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
	const UDialogSpeakerDataAsset* GetSpeakerData() const { return SpeakerData; }

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

	/** Advance this line by itself once its voice-over and its text reveal have both finished. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	bool bAutoSkip = false;

	/** Seconds to wait after the line has fully played before auto-advancing. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (EditCondition = "bAutoSkip", ClampMin = "0.0", ForceUnits = "s"))
	float AutoSkipDelay = 0.f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FString DevComment;
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

	UFUNCTION()
	void OnDialogueLineCompleted();

private:
	/** Starts the auto-skip countdown; only subscribed while auto-skip is active for this line. */
	void OnDialogueLinePresented();
	void OnAutoSkipElapsed();

	FTimerHandle AutoSkipTimerHandle;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual void UpdateNodeConfigText_Implementation() override;

	const FString& GetDevComment() const { return DevComment; }
#endif
};

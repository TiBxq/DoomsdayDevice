// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_DialogueHintTriggered.generated.h"

/**
 * Waits for the player to press the dialogue-hint input (available only while a hint is on screen).
 * Fires Success on press; Stop cancels the wait and fires Stopped.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Dialogue Hint Triggered"))
class DOOMSDAYDEVICE_API UFlowNode_DialogueHintTriggered : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_DialogueHintTriggered();

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void OnHintTriggered();
};

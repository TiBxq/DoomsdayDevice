// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_EndDialogue.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "End Dialogue"))
class DOOMSDAYDEVICE_API UFlowNode_EndDialogue : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_EndDialogue();

	/** Called by UDialogueSubsystem once the screen is closed - or straight away if this node does not own it. */
	void OnDialogueCloseCompleted();

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_DialogueLine.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Dialogue Line"))
class DOOMSDAYDEVICE_API UFlowNode_DialogueLine : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_DialogueLine();

protected:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText LineText;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

	UFUNCTION()
	void OnDialogueLineCompleted();

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

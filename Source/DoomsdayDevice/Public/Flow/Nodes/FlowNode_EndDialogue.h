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

protected:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FText> ChoiceTexts;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

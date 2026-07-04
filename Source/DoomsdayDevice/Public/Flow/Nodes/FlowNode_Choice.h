// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_Choice.generated.h"

/**
 * 
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Choice"))
class DOOMSDAYDEVICE_API UFlowNode_Choice : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_Choice();

protected:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FText> ChoiceTexts;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

	UFUNCTION()
	void OnChoiceSelected(int32 Index);

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

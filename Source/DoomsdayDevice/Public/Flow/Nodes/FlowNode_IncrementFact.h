// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_IncrementFact.generated.h"

/**
 *
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Increment Fact"))
class DOOMSDAYDEVICE_API UFlowNode_IncrementFact : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_IncrementFact();

protected:
	UPROPERTY(EditAnywhere, Category = "Facts")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, Category = "Facts")
	int32 Value;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

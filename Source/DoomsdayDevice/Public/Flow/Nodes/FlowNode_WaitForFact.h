// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "GameplayTagContainer.h"
#include "FlowNode_WaitForFact.generated.h"

class UFactsDBSubsystem;

/**
 * Waits until the specified fact's value becomes exactly equal to Value.
 * Fires Success immediately if the fact already equals Value on Start.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Wait For Fact"))
class DOOMSDAYDEVICE_API UFlowNode_WaitForFact : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_WaitForFact();

protected:
	UPROPERTY(EditAnywhere, Category = "Facts")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, Category = "Facts")
	int32 Value;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	void OnFactChanged(const FGameplayTag& ChangedTag, int32 NewValue);
	UFactsDBSubsystem* GetFactsDB() const;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

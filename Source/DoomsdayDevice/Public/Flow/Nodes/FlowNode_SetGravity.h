// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_SetGravity.generated.h"

/**
 * Overrides the level's global gravity (AWorldSettings::GlobalGravityZ) at runtime,
 * then triggers its output. Wakes settled rigid bodies so they react immediately.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Set Global Gravity"))
class DOOMSDAYDEVICE_API UFlowNode_SetGravity : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_SetGravity();

protected:
	// Target world gravity along Z, in cm/s^2. -980 ~= earth, 0 = zero-g, positive = upward.
	UPROPERTY(EditAnywhere, Category = "World")
	float GravityZ = 0.0f;

	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};

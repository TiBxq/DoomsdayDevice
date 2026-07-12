#pragma once

#include "Nodes/FlowNode.h"
#include "GameplayTagContainer.h"
#include "FlowNode_OnItemCollected.generated.h"

class UInventorySubsystem;

/**
 * Waits until the player has collected the specified item.
 * Fires Success immediately if the item was already collected before Start.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Item Collected"))
class DOOMSDAYDEVICE_API UFlowNode_OnItemCollected : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_OnItemCollected();

protected:
	/** Item to wait for, e.g. Flow.Items.Test.Cube */
	UPROPERTY(EditAnywhere, Category = "Items", meta = (Categories = "Flow.Items"))
	FGameplayTag ItemTag;

	/** Node succeeds once the player holds at least this many. */
	UPROPERTY(EditAnywhere, Category = "Items", meta = (ClampMin = 1))
	int32 RequiredCount;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	void OnItemCollected(const FGameplayTag& Tag, const int32 NewCount);
	UInventorySubsystem* GetInventory() const;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

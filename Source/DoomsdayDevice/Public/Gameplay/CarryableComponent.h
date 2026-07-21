#pragma once

#include "Gameplay/InteractionComponent.h"
#include "GameplayTagContainer.h"
#include "CarryableComponent.generated.h"

class UItemSlotComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCarryEvent);

/**
 * Interaction that makes its owner a heavy item the player carries in hands.
 * While carried the owner is attached to the player and movement is slowed.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class DOOMSDAYDEVICE_API UCarryableComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:
	UCarryableComponent(const FObjectInitializer& ObjectInitializer);

	/** Item identity, e.g. Flow.Items.Test.HeavyCube */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry", meta = (Categories = "Flow.Items"))
	FGameplayTag ItemTag;

	/** Player's MaxWalkSpeed is multiplied by this while the item is carried */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float CarrySpeedMultiplier;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Carry")
	bool IsSlotted() const { return CurrentSlot.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Carry")
	UItemSlotComponent* GetCurrentSlot() const;

	void SetCurrentSlot(UItemSlotComponent* Slot);

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FCarryEvent OnDropped;

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FCarryEvent OnStartCarry;

protected:
	TWeakObjectPtr<UItemSlotComponent> CurrentSlot;

	UFUNCTION()
	void HandleUsed();
};

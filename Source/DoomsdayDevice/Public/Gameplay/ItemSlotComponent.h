#pragma once

#include "Gameplay/InteractionComponent.h"
#include "GameplayTagContainer.h"
#include "ItemSlotComponent.generated.h"

class UCarryableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemSlotEvent, UCarryableComponent*, Item);

/**
 * A slot the player can place a carried item into. The component's own transform
 * is the rest point of the connected item. While occupied the slot interaction is
 * disabled and the slotted item's interaction takes over (used to take it back).
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class DOOMSDAYDEVICE_API UItemSlotComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:
	UItemSlotComponent(const FObjectInitializer& ObjectInitializer);

	/** Items whose ItemTag matches this tag (hierarchically) can be connected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot", meta = (Categories = "Flow.Items"))
	FGameplayTag AcceptedItemTag;

	UPROPERTY(BlueprintAssignable, Category = "Slot")
	FItemSlotEvent OnItemConnected;

	UPROPERTY(BlueprintAssignable, Category = "Slot")
	FItemSlotEvent OnItemDisconnected;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Slot")
	bool IsOccupied() const { return ConnectedItem.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Slot")
	UCarryableComponent* GetConnectedItem() const;

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void ConnectItem(UCarryableComponent* Item);

	/**
	 * Detaches the connected item and re-enables the slot. Deliberately leaves the
	 * item's interaction and collision untouched - the caller decides what happens
	 * to the item next (the carryable picks itself up, a Flow/BP call leaves it standing).
	 */
	UFUNCTION(BlueprintCallable, Category = "Slot")
	void DisconnectItem();

protected:
	TWeakObjectPtr<UCarryableComponent> ConnectedItem;

	UFUNCTION()
	void HandleUsed();
};

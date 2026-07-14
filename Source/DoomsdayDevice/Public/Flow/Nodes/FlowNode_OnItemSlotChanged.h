#pragma once

#include "Nodes/Actor/FlowNode_ComponentObserver.h"
#include "FlowNode_OnItemSlotChanged.generated.h"

class UCarryableComponent;
class UFlowComponent;
class UItemSlotComponent;

/**
 * Observes item slots (found by the identity tags of their Flow component) and fires
 * Connected / Disconnected whenever an item is placed into or taken out of the slot.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Item Slot Changed"))
class DOOMSDAYDEVICE_API UFlowNode_OnItemSlotChanged : public UFlowNode_ComponentObserver
{
	GENERATED_BODY()

public:
	UFlowNode_OnItemSlotChanged();

protected:
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UItemSlotComponent>> ObservedSlots;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void OnSlotConnected(UCarryableComponent* Item);

	UFUNCTION()
	void OnSlotDisconnected(UCarryableComponent* Item);
};

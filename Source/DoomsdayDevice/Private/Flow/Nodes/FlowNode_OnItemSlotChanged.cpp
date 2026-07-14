#include "Flow/Nodes/FlowNode_OnItemSlotChanged.h"
#include "Gameplay/ItemSlotComponent.h"

#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnItemSlotChanged)

UFlowNode_OnItemSlotChanged::UFlowNode_OnItemSlotChanged()
{
	// fire indefinitely; only the Stop input finishes the node
	SuccessLimit = 0;

	// Stopped must stay - the base class triggers it from the Stop input
	OutputPins = {FFlowPin(TEXT("Connected")), FFlowPin(TEXT("Disconnected")), FFlowPin(TEXT("Stopped"))};
}

void UFlowNode_OnItemSlotChanged::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!ObservedSlots.Contains(Actor))
	{
		TArray<UItemSlotComponent*> FoundSlots;
		Actor->GetComponents<UItemSlotComponent>(FoundSlots);

		if (FoundSlots.Num() > 0)
		{
			RegisteredActors.Emplace(Actor, Component);

			ObservedSlots.Emplace(Actor, FoundSlots[0]);
			FoundSlots[0]->OnItemConnected.AddDynamic(this, &UFlowNode_OnItemSlotChanged::OnSlotConnected);
			FoundSlots[0]->OnItemDisconnected.AddDynamic(this, &UFlowNode_OnItemSlotChanged::OnSlotDisconnected);

			// the slot might have been filled before this node started observing
			if (FoundSlots[0]->IsOccupied())
			{
				OnSlotConnected(FoundSlots[0]->GetConnectedItem());
			}
		}
	}
}

void UFlowNode_OnItemSlotChanged::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (const TWeakObjectPtr<UItemSlotComponent>* SlotPtr = ObservedSlots.Find(Actor))
	{
		if (SlotPtr->IsValid())
		{
			(*SlotPtr)->OnItemConnected.RemoveAll(this);
			(*SlotPtr)->OnItemDisconnected.RemoveAll(this);
		}

		ObservedSlots.Remove(Actor);
	}
}

void UFlowNode_OnItemSlotChanged::Cleanup()
{
	Super::Cleanup();

	for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<UItemSlotComponent>>& Slot : ObservedSlots)
	{
		if (Slot.Value.IsValid())
		{
			Slot.Value->OnItemConnected.RemoveAll(this);
			Slot.Value->OnItemDisconnected.RemoveAll(this);
		}
	}
	ObservedSlots.Empty();
}

void UFlowNode_OnItemSlotChanged::OnSlotConnected(UCarryableComponent* Item)
{
	TriggerOutput(TEXT("Connected"), false);
}

void UFlowNode_OnItemSlotChanged::OnSlotDisconnected(UCarryableComponent* Item)
{
	TriggerOutput(TEXT("Disconnected"), false);
}

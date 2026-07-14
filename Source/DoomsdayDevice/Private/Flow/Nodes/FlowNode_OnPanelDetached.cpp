#include "Flow/Nodes/FlowNode_OnPanelDetached.h"
#include "Gameplay/WallPanel.h"

#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnPanelDetached)

void UFlowNode_OnPanelDetached::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!ObservedPanels.Contains(Actor))
	{
		if (AWallPanel* Panel = Cast<AWallPanel>(Actor.Get()))
		{
			RegisteredActors.Emplace(Actor, Component);

			ObservedPanels.Emplace(Actor, Panel);
			Panel->OnPanelDetached.AddDynamic(this, &UFlowNode_OnPanelDetached::OnEventReceived);

			// the panel might have fallen before this node started observing
			if (Panel->IsFullyDetached())
			{
				OnEventReceived();
			}
		}
	}
}

void UFlowNode_OnPanelDetached::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (const TWeakObjectPtr<AWallPanel>* PanelPtr = ObservedPanels.Find(Actor))
	{
		if (PanelPtr->IsValid())
		{
			(*PanelPtr)->OnPanelDetached.RemoveAll(this);
		}

		ObservedPanels.Remove(Actor);
	}
}

void UFlowNode_OnPanelDetached::Cleanup()
{
	Super::Cleanup();

	for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<AWallPanel>>& Panel : ObservedPanels)
	{
		if (Panel.Value.IsValid())
		{
			Panel.Value->OnPanelDetached.RemoveAll(this);
		}
	}
	ObservedPanels.Empty();
}

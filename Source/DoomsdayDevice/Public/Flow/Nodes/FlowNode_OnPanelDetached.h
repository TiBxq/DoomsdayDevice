#pragma once

#include "Nodes/Actor/FlowNode_ComponentObserver.h"
#include "FlowNode_OnPanelDetached.generated.h"

class AWallPanel;
class UFlowComponent;

/**
 * Observes wall panels (found by the identity tags of their Flow component) and fires
 * Success once the last screw is ejected and the panel falls off the wall.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Panel Detached"))
class DOOMSDAYDEVICE_API UFlowNode_OnPanelDetached : public UFlowNode_ComponentObserver
{
	GENERATED_BODY()

protected:
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AWallPanel>> ObservedPanels;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void Cleanup() override;
};

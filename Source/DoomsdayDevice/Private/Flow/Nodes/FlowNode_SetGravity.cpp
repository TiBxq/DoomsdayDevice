// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_SetGravity.h"

#include "Engine/World.h"
#include "EngineUtils.h"                      // TActorIterator
#include "GameFramework/WorldSettings.h"
#include "Components/PrimitiveComponent.h"

UFlowNode_SetGravity::UFlowNode_SetGravity()
{
#if WITH_EDITOR
	Category = TEXT("World");
#endif
}

void UFlowNode_SetGravity::ExecuteInput(const FName& PinName)
{
	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->bGlobalGravitySet = true;
			WorldSettings->GlobalGravityZ = GravityZ;
			WorldSettings->WorldGravityZ = GravityZ; // keep replicated/cached value in sync
		}

		// Wake settled rigid bodies so they react to the new gravity immediately.
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<UPrimitiveComponent*> PrimitiveComponents;
			It->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
			for (UPrimitiveComponent* Prim : PrimitiveComponents)
			{
				if (Prim->IsSimulatingPhysics())
				{
					Prim->WakeAllRigidBodies();
				}
			}
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_SetGravity::GetNodeDescription() const
{
	return FString::Printf(TEXT("Gravity Z: %.0f"), GravityZ);
}
#endif

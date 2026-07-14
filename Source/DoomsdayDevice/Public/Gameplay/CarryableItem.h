#pragma once

#include "GameFramework/Actor.h"
#include "CarryableItem.generated.h"

class UCarryableComponent;
class UFlowComponent;
class UStaticMeshComponent;

/**
 * Ready-made heavy carryable actor: mesh + Flow identity + carry interaction.
 * Defaults to the Flow.Items.Test.HeavyCube test item; override tags per instance for real items.
 */
UCLASS(ClassGroup = Gameplay)
class DOOMSDAYDEVICE_API ACarryableItem : public AActor
{
	GENERATED_BODY()

public:
	ACarryableItem();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry")
	TObjectPtr<UFlowComponent> FlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry")
	TObjectPtr<UCarryableComponent> CarryableComponent;
};

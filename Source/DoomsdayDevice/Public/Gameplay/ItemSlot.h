#pragma once

#include "GameFramework/Actor.h"
#include "ItemSlot.generated.h"

class UFlowComponent;
class UItemSlotComponent;
class UStaticMeshComponent;

/**
 * Ready-made pedestal slot actor: mesh + Flow identity + slot interaction.
 * Defaults to the Flow.Slots.Test identity accepting Flow.Items.Test.HeavyCube;
 * override tags per instance for real slots.
 */
UCLASS(ClassGroup = Gameplay)
class DOOMSDAYDEVICE_API AItemSlot : public AActor
{
	GENERATED_BODY()

public:
	AItemSlot();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot")
	TObjectPtr<UFlowComponent> FlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot")
	TObjectPtr<UItemSlotComponent> SlotComponent;
};

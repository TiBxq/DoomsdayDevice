#pragma once

#include "GameFramework/Actor.h"
#include "PickupItem.generated.h"

class UFlowComponent;
class UPickupComponent;
class UStaticMeshComponent;

/**
 * Ready-made collectible actor: mesh + Flow identity + pickup interaction.
 * Defaults to the Flow.Items.Test.Cube test item; override tags per instance for real items.
 */
UCLASS(ClassGroup = Gameplay)
class DOOMSDAYDEVICE_API APickupItem : public AActor
{
	GENERATED_BODY()

public:
	APickupItem();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UFlowComponent> FlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UPickupComponent> PickupComponent;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ToolActor.generated.h"

class UStaticMeshComponent;

/**
 * Hand-held tool visual. Spawned by the character on first equip and attached to the
 * first-person arms; hidden instead of destroyed on unequip. Mesh and grip offset are
 * configured in Blueprint subclasses (referenced by FToolSlotDefinition::ToolActorClass).
 * The root snaps to the hand socket; the mesh child's relative transform is the grip offset.
 */
UCLASS(Blueprintable)
class DOOMSDAYDEVICE_API AToolActor : public AActor
{
	GENERATED_BODY()

public:
	AToolActor();

	UStaticMeshComponent* GetMesh() const { return MeshComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};

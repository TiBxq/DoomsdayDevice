#pragma once

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Player/Saveable.h"

#include "WallPanel.generated.h"

class APanelScrew;
class UFlowComponent;
class UPhysicsConstraintComponent;
class UStaticMeshComponent;
class USceneComponent;
class USaveableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWallPanelEvent);

/**
 * Physics-based panel held on a wall by screws spawned at ScrewSlots.
 * The panel stays fixed while two or more screws remain, dangles off the last
 * remaining screw, and falls off the wall once that final screw is ejected.
 */
UCLASS(Blueprintable, ClassGroup = Gameplay)
class DOOMSDAYDEVICE_API AWallPanel : public AActor, public ISaveable
{
	GENERATED_BODY()

public:
	AWallPanel();

	virtual void BeginPlay() override;

	/** Screw spawn points in panel space; slot +X points out of the wall, so identity rotation is correct */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panel", meta = (MakeEditWidget))
	TArray<FTransform> ScrewSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panel")
	TSubclassOf<APanelScrew> ScrewClass;

	/** Identity tags stamped on every spawned screw's Flow component, so graphs can gate all screws at once */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panel", meta = (Categories = "Flow.Interaction"))
	FGameplayTagContainer ScrewIdentityTags;

	/** Fires once, when the last screw is ejected and the panel falls */
	UPROPERTY(BlueprintAssignable, Category = "Panel")
	FWallPanelEvent OnPanelDetached;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	TObjectPtr<USaveableComponent> SaveComponent;

	UPROPERTY(SaveGame)
	TSet<int32> EjectedIndices;

	/** Ejects the most recently spawned remaining screw; debug/Flow hook */
	UFUNCTION(BlueprintCallable, Category = "Panel")
	void EjectNextScrew();

	UFUNCTION(BlueprintPure, Category = "Panel")
	bool IsFullyDetached() const { return bDetached; }

	virtual void OnPostLoadSaved_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Panel")
	TObjectPtr<USceneComponent> PanelRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Panel")
	TObjectPtr<UFlowComponent> FlowComponent;

	UFUNCTION()
	void HandleScrewEjected(APanelScrew* Screw);

	void BeginDangle(APanelScrew* RemainingScrew);
	void BeginFall();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<APanelScrew>> SpawnedScrews;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicsConstraintComponent> HangConstraint;

	bool bDetached = false;
};

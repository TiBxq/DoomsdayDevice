#pragma once

#include "Gameplay/InteractionComponent.h"
#include "GameplayTagContainer.h"
#include "PickupComponent.generated.h"

/**
 * Interaction that grants an item (identified by gameplay tag) when used.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class DOOMSDAYDEVICE_API UPickupComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:
	UPickupComponent(const FObjectInitializer& ObjectInitializer);

	/** Item granted on pickup, e.g. Flow.Items.Test.Cube */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (Categories = "Flow.Items"))
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = 1))
	int32 Count;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	bool bDestroyOwnerOnPickup;

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void HandlePickedUp();
};

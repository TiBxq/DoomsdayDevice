#pragma once

#include "Gameplay/InteractionComponent.h"
#include "ScrewInteractionComponent.generated.h"

/**
 * Interaction that ejects its owning APanelScrew from the wall panel when used.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class DOOMSDAYDEVICE_API UScrewInteractionComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:
	UScrewInteractionComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void HandleUsed();
};

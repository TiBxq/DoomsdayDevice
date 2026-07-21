#pragma once

#include "GameFramework/Actor.h"
#include "PanelScrew.generated.h"

class UFlowComponent;
class UScrewInteractionComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPanelScrewEvent, class APanelScrew*, Screw);

/**
 * Screw holding a wall panel: interacting ejects it with physics, leaving it as debris.
 * Spawned by AWallPanel at its screw slots; the panel stamps identity tags at spawn.
 */
UCLASS(Blueprintable, ClassGroup = Gameplay)
class DOOMSDAYDEVICE_API APanelScrew : public AActor
{
	GENERATED_BODY()

public:
	APanelScrew();

	/** Linear eject velocity along the screw axis, in uu/s (applied as velocity change, independent of mass) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screw", meta = (ClampMin = 0.0))
	float EjectSpeed;

	/** Random angular velocity added on eject, in rad/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screw", meta = (ClampMin = 0.0))
	float EjectSpin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screw")
	FName EjectedProfileName;

	UPROPERTY(BlueprintAssignable, Category = "Screw")
	FPanelScrewEvent OnEjected;

	/** Pops the screw out of the panel: disables interaction, then detaches and simulates physics */
	UFUNCTION(BlueprintCallable, Category = "Screw")
	void Eject();

	/** Out-of-the-wall axis: the cylinder mesh's long axis (actor up) */
	FVector GetScrewAxis() const { return GetActorUpVector(); }

	UFlowComponent* GetFlowComponent() const { return FlowComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Screw")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Screw")
	TObjectPtr<UFlowComponent> FlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Screw")
	TObjectPtr<UScrewInteractionComponent> InteractionComponent;

private:
	bool bEjected = false;
};

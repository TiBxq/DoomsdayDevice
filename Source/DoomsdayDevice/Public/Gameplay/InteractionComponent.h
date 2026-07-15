// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Components/ArrowComponent.h"
#include "GameplayTagContainer.h"
#include "InteractionComponent.generated.h"

class APlayerCameraManager;

DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerInInteractionEvent, TWeakObjectPtr<class UInteractionComponent> /*Interaction*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInteractionComponentEvent);

/**
 * Interaction Component
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class  UInteractionComponent : public UArrowComponent
{
	GENERATED_UCLASS_BODY()

	static FPlayerInInteractionEvent OnPlayerEnter;
	static FPlayerInInteractionEvent OnPlayerExit;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bEnabled;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = 50.0f))
	float Distance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bPrecise = false;

	/** If set, Use is blocked (OnUseDenied instead of OnUsed) unless the equipped tool matches this tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (Categories = "Flow.Items.Tools"))
	FGameplayTag RequiredToolTag;

private:
	bool bCanInteract;
	TWeakObjectPtr<APlayerCameraManager> CameraManager;
	
public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Enable();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Disable();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsToolRequirementMet(const FGameplayTag& EquippedToolTag) const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionComponentEvent OnUsed;

	/** Fired instead of OnUsed when the player tries to use this without the required tool in hand. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionComponentEvent OnUseDenied;
};

// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Components/ArrowComponent.h"
#include "GameplayTagContainer.h"

#include "Gameplay/InteractionPrompt.h"

#include "InteractionComponent.generated.h"

class APlayerCameraManager;
class UInteractionPromptData;

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

	/** Prompt shown while targeting this. Leave unset for the project default (UPlayerSettings::DefaultPrompt). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractionPromptData> Prompt;

private:
	bool bCanInteract;
	TWeakObjectPtr<APlayerCameraManager> CameraManager;

	/** Loads DefaultPrompt into ResolvedDefaultPrompt. Called from BeginPlay; a no-op once resolved. */
	void ResolveDefaultPrompt();

	/**
	 * UPlayerSettings::DefaultPrompt, loaded in BeginPlay and used while Prompt is unset. The hard
	 * reference that keeps the asset alive lives here, on a normally collected component, and must
	 * never be cached on the settings CDO instead - see the UPlayerSettings class comment.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInteractionPromptData> ResolvedDefaultPrompt;

public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Enable();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Disable();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsToolRequirementMet(const FGameplayTag& EquippedToolTag) const;

	/**
	 * Current prompt for this interaction. Called every frame while the player targets it, so
	 * overrides must be cheap. The default implementation blocks on RequiredToolTag; override it to
	 * add other block reasons (slot occupied, hands full, story state) without touching the player
	 * controller or the UI - the controller gates the actual Use press on this too.
	 *
	 * Override _Implementation and call Super::EvaluatePrompt_Implementation, never Super::EvaluatePrompt.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FInteractionPrompt EvaluatePrompt(const FGameplayTag& EquippedToolTag) const;
	virtual FInteractionPrompt EvaluatePrompt_Implementation(const FGameplayTag& EquippedToolTag) const;

	/** Prompt, or the resolved UPlayerSettings::DefaultPrompt when this interaction leaves it unset. Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	const UInteractionPromptData* GetPromptData() const;

	/** The prompt asset's UseText, or the built-in fallback when nothing is configured. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetResolvedUseText() const;

	/** The prompt asset's BlockedText, or the "<Tool> required" / generic text from UPlayerSettings. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetResolvedBlockedText() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionComponentEvent OnUsed;

	/** Fired instead of OnUsed when the player tries to use this without the required tool in hand. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionComponentEvent OnUseDenied;
};

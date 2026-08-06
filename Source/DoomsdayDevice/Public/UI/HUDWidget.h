// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HUDWidget.generated.h"

class UImage;

UENUM(BlueprintType)
enum class EInteractionReticleState : uint8
{
	/** Nothing interactable in range. */
	Idle,
	/** An interaction is targeted and can be used right now. */
	Available,
	/** An interaction is targeted but blocked, e.g. the required tool is not in hand. */
	Blocked
};

/**
 * Base for the HUD. Owns the reticle state: the player controller pushes it through UBasicUIManager
 * whenever the targeted interaction's prompt changes. The setter is idempotent, so the pull on
 * construct and the push never double-fire the Blueprint event.
 */
UCLASS()
class DOOMSDAYDEVICE_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Tints the bound reticle and fires BP_ReticleStateChanged. No-op when the state is unchanged. */
	void SetReticleState(EInteractionReticleState NewState);

	UFUNCTION(BlueprintPure, Category = "HUD")
	EInteractionReticleState GetReticleState() const { return ReticleState; }

	/** The colour the reticle uses for a state, so Blueprints can match other elements to it. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	FLinearColor GetReticleStateColor(EInteractionReticleState State) const;

	/** Richer treatment - scale, animation, icon swap. Fires after the native tint is applied. */
	UFUNCTION(BlueprintImplementableEvent)
	void BP_ReticleStateChanged(EInteractionReticleState NewState);

protected:
	virtual void NativeConstruct() override;

	/** Optional: without an Image named "Reticle" the native tint is skipped. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UImage> Reticle;

	/** Multiplied into the authored reticle brush. White reproduces the authored look exactly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Reticle")
	FLinearColor ReticleIdleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Reticle")
	FLinearColor ReticleAvailableColor = FLinearColor(0.15f, 0.95f, 0.30f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Reticle")
	FLinearColor ReticleBlockedColor = FLinearColor(0.95f, 0.20f, 0.15f, 1.0f);

	/**
	 * A RoundedBox brush draws its outline straight from the brush and ignores the widget's
	 * ColorAndOpacity, so the ring stays neutral unless this rewrites the brush too. Turn off if
	 * the reticle should keep a fixed-colour ring.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Reticle")
	bool bTintReticleOutline = true;

private:
	void ApplyReticleColor();

	/** Authored brush captured on construct; state tints are built from this, never from the last one. */
	UPROPERTY(Transient)
	FSlateBrush BaseReticleBrush;

	EInteractionReticleState ReticleState = EInteractionReticleState::Idle;
};

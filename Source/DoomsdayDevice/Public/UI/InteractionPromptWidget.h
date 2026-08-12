#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Gameplay/InteractionPrompt.h"

#include "InteractionPromptWidget.generated.h"

class UTextBlock;

/**
 * Base for the interaction ("Use") prompt. UBasicUIManager destroys and recreates this widget every
 * time the player targets a new interaction, so NativeConstruct pulls the prompt the controller has
 * already computed and later changes arrive as pushes through UBasicUIManager. The setter is
 * idempotent, so the pull-then-push overlap never double-fires the Blueprint event.
 *
 * Mirrors UToolSlotsWidget.
 */
UCLASS()
class DOOMSDAYDEVICE_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Writes Label (when bound) and fires BP_PromptChanged. No-op when the prompt is unchanged. */
	void SetPrompt(const FInteractionPrompt& NewPrompt);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FInteractionPrompt GetPrompt() const { return Prompt; }

	/** Richer treatment - icon swap, colour, animation. Fires after the native Label update. */
	UFUNCTION(BlueprintImplementableEvent)
	void BP_PromptChanged(const FInteractionPrompt& NewPrompt);

	void SetPromptVisible(bool bNew);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleAppearFinished();

	/** Optional: without a TextBlock named "Label" the designer-authored text is left alone. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Interaction")
	TObjectPtr<UTextBlock> Label;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ShowAnim;

	bool bWantVisible = false;
	bool bPlayingForward = false;
	bool bFullyShown = false;

private:
	FInteractionPrompt Prompt;
	bool bHasPrompt = false;
};

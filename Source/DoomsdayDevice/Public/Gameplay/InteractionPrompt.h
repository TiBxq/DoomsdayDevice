#pragma once

#include "CoreMinimal.h"
#include "Internationalization/Text.h"

#include "InteractionPrompt.generated.h"

/**
 * What the UI should say about an interaction right now - the evaluated result of
 * UInteractionComponent::EvaluatePrompt, as opposed to the authored UInteractionPromptData.
 *
 * Lives in its own header because UBasicUIManager, UHUDWidget and UInteractionPromptWidget need
 * the struct but not the component.
 */
USTRUCT(BlueprintType)
struct DOOMSDAYDEVICE_API FInteractionPrompt
{
	GENERATED_BODY()

	/** False while something blocks Use; the default evaluation blocks on RequiredToolTag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanUse = true;

	/** Player-facing prompt for the current state, e.g. "Use" or "Screwdriver required". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptText;

	/**
	 * Change detection for the per-tick push. DeepCompare makes two separately built texts with
	 * identical content compare equal, so a steady state doesn't re-push. A false negative only
	 * costs one redundant push - every consumer downstream is idempotent too.
	 */
	bool IsEquivalentTo(const FInteractionPrompt& Other) const
	{
		return bCanUse == Other.bCanUse
			&& PromptText.IdenticalTo(Other.PromptText, ETextIdenticalModeFlags::DeepCompare);
	}
};

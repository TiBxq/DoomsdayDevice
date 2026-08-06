#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Internationalization/Text.h"

#include "InteractionPromptData.generated.h"

/**
 * One authored interaction prompt - "Use", "Open", "Pick up". Assets live in
 * Content/_Doomsaday/Interactions/ and are shared by every interaction using that verb, so the
 * wording is changed in one place instead of on each component.
 *
 * Interactions that leave UInteractionComponent::Prompt unset fall back to
 * UPlayerSettings::DefaultPrompt.
 */
UCLASS(BlueprintType)
class DOOMSDAYDEVICE_API UInteractionPromptData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Prompt while the interaction can be used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	FText UseText;

	/**
	 * Prompt while Use is blocked. Leave empty to auto-generate from the interaction's
	 * RequiredToolTag, e.g. "Screwdriver required".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	FText BlockedText;
};

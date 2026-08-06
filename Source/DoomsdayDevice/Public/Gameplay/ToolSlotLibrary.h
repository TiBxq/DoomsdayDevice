#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Gameplay/ToolSlotDefinition.h"

#include "ToolSlotLibrary.generated.h"

/**
 * Single home for UPlayerSettings::ToolSlots lookups. UToolSlotsWidget forwards its static helpers
 * here; the interaction prompt and UPickupComponent use the tag lookups.
 */
UCLASS()
class DOOMSDAYDEVICE_API UToolSlotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Tools")
	static int32 GetNumToolSlots();

	UFUNCTION(BlueprintPure, Category = "Tools")
	static bool GetToolSlotDefinition(int32 SlotIndex, FToolSlotDefinition& OutDefinition);

	/** Slot whose ToolTag is exactly ToolTag. INDEX_NONE when none matches. */
	UFUNCTION(BlueprintPure, Category = "Tools")
	static int32 FindToolSlotIndexByTag(const FGameplayTag& ToolTag);

	/**
	 * First slot that satisfies RequirementTag, i.e. Slot.ToolTag.MatchesTag(RequirementTag) - the
	 * mirror of UInteractionComponent::IsToolRequirementMet. With a broad requirement tag (a parent
	 * category) this names the first matching slot, by design.
	 */
	UFUNCTION(BlueprintPure, Category = "Tools")
	static int32 FindToolSlotIndexForRequirement(const FGameplayTag& RequirementTag);

	/**
	 * DisplayName of the first slot satisfying RequirementTag. Empty both when no slot matches and
	 * when the matched slot has a blank DisplayName, so callers have one "fall back" condition.
	 */
	UFUNCTION(BlueprintPure, Category = "Tools")
	static FText GetToolDisplayNameForRequirement(const FGameplayTag& RequirementTag);
};

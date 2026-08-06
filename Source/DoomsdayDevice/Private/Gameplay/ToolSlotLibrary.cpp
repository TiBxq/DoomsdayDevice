#include "Gameplay/ToolSlotLibrary.h"

#include "Player/PlayerSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolSlotLibrary)

int32 UToolSlotLibrary::GetNumToolSlots()
{
	return GetDefault<UPlayerSettings>()->ToolSlots.Num();
}

bool UToolSlotLibrary::GetToolSlotDefinition(const int32 SlotIndex, FToolSlotDefinition& OutDefinition)
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	if (!ToolSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutDefinition = ToolSlots[SlotIndex];
	return true;
}

int32 UToolSlotLibrary::FindToolSlotIndexByTag(const FGameplayTag& ToolTag)
{
	if (!ToolTag.IsValid())
	{
		return INDEX_NONE;
	}

	return GetDefault<UPlayerSettings>()->ToolSlots.IndexOfByPredicate([&ToolTag](const FToolSlotDefinition& Slot)
		{
			return Slot.ToolTag == ToolTag;
		});
}

int32 UToolSlotLibrary::FindToolSlotIndexForRequirement(const FGameplayTag& RequirementTag)
{
	if (!RequirementTag.IsValid())
	{
		return INDEX_NONE;
	}

	// hierarchical, mirroring UInteractionComponent::IsToolRequirementMet: a child tool satisfies a parent requirement
	return GetDefault<UPlayerSettings>()->ToolSlots.IndexOfByPredicate([&RequirementTag](const FToolSlotDefinition& Slot)
		{
			return Slot.ToolTag.MatchesTag(RequirementTag);
		});
}

FText UToolSlotLibrary::GetToolDisplayNameForRequirement(const FGameplayTag& RequirementTag)
{
	const int32 SlotIndex = FindToolSlotIndexForRequirement(RequirementTag);
	if (SlotIndex == INDEX_NONE)
	{
		return FText::GetEmpty();
	}

	return GetDefault<UPlayerSettings>()->ToolSlots[SlotIndex].DisplayName;
}

#include "UI/ToolSlotsWidget.h"

#include "Engine/GameInstance.h"

#include "DoomsdayDeviceCharacter.h"
#include "Gameplay/InventorySubsystem.h"
#include "Player/PlayerSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolSlotsWidget)

void UToolSlotsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	UnlockedSlots.Init(false, ToolSlots.Num());
	SelectedSlot = INDEX_NONE;

	BP_InitializeSlots(ToolSlots.Num());

	const UGameInstance* GameInstance = GetGameInstance();
	if (const UInventorySubsystem* Inventory = GameInstance ? GameInstance->GetSubsystem<UInventorySubsystem>() : nullptr)
	{
		for (int32 SlotIndex = 0; SlotIndex < ToolSlots.Num(); ++SlotIndex)
		{
			if (Inventory->HasItem(ToolSlots[SlotIndex].ToolTag))
			{
				SetSlotUnlocked(SlotIndex);
			}
		}
	}

	if (const ADoomsdayDeviceCharacter* PlayerCharacter = Cast<ADoomsdayDeviceCharacter>(GetOwningPlayerPawn()))
	{
		SetEquippedSlot(PlayerCharacter->GetEquippedToolSlot());
	}
}

void UToolSlotsWidget::SetSlotUnlocked(const int32 SlotIndex)
{
	if (!UnlockedSlots.IsValidIndex(SlotIndex) || UnlockedSlots[SlotIndex])
	{
		return;
	}

	UnlockedSlots[SlotIndex] = true;
	BP_SlotUnlocked(SlotIndex);
}

void UToolSlotsWidget::SetEquippedSlot(const int32 NewSlotIndex)
{
	if (NewSlotIndex == SelectedSlot)
	{
		return;
	}

	if (SelectedSlot != INDEX_NONE)
	{
		BP_SlotDeselected(SelectedSlot);
	}

	SelectedSlot = NewSlotIndex;

	if (SelectedSlot != INDEX_NONE)
	{
		BP_SlotSelected(SelectedSlot);
	}
}

int32 UToolSlotsWidget::GetNumToolSlots()
{
	return GetDefault<UPlayerSettings>()->ToolSlots.Num();
}

bool UToolSlotsWidget::GetToolSlotDefinition(const int32 SlotIndex, FToolSlotDefinition& OutDefinition)
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	if (!ToolSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutDefinition = ToolSlots[SlotIndex];
	return true;
}

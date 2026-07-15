#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/ToolSlotDefinition.h"

#include "ToolSlotsWidget.generated.h"

/**
 * Base for the tool slots bar. Pulls full state on construct (slot definitions from
 * UPlayerSettings, unlocks from UInventorySubsystem, selection from the player pawn),
 * then receives deltas through UBasicUIManager. Setters are idempotent, so the
 * pull-then-push overlap never double-fires the Blueprint events.
 */
UCLASS()
class DOOMSDAYDEVICE_API UToolSlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSlotUnlocked(int32 SlotIndex);

	/** INDEX_NONE = empty hands */
	void SetEquippedSlot(int32 NewSlotIndex);

	UFUNCTION(BlueprintPure, Category = "Tools")
	static int32 GetNumToolSlots();

	UFUNCTION(BlueprintPure, Category = "Tools")
	static bool GetToolSlotDefinition(int32 SlotIndex, FToolSlotDefinition& OutDefinition);

	/** Build the slot entries; fired once on construct, before the unlock/selection events */
	UFUNCTION(BlueprintImplementableEvent)
	void BP_InitializeSlots(int32 NumSlots);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_SlotUnlocked(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_SlotSelected(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_SlotDeselected(int32 SlotIndex);

protected:
	virtual void NativeConstruct() override;

private:
	TArray<bool> UnlockedSlots;
	int32 SelectedSlot = INDEX_NONE;
};

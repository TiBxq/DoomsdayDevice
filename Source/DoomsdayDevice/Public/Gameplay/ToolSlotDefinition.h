#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"

#include "ToolSlotDefinition.generated.h"

class AToolActor;
class UTexture2D;

/**
 * One static tool slot: which item tag unlocks it, and what to show in hands / on the HUD.
 * Slot index = index in UPlayerSettings::ToolSlots = hotkey number - 1.
 */
USTRUCT(BlueprintType)
struct DOOMSDAYDEVICE_API FToolSlotDefinition
{
	GENERATED_BODY()

	/** Collecting an item matching this tag unlocks the slot; the equipped tool reports this tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool", meta = (Categories = "Flow.Items.Tools"))
	FGameplayTag ToolTag;

	/** Actor spawned and attached to the first-person arms while the slot is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool")
	TSoftClassPtr<AToolActor> ToolActorClass;

	/** HUD icon for the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool")
	FText DisplayName;
};

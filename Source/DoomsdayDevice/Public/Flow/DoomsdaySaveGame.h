#pragma once

#include "FlowSave.h"
#include "GameplayTagContainer.h"
#include "DoomsdaySaveGame.generated.h"

/**
 * Checkpoint save payload. Extends the Flow plugin's UFlowSaveGame (which carries all
 * serialized Flow graph/component state) with the game-specific state that lives outside
 * Flow: collected items and narrative facts. One object, one "Checkpoint" slot.
 */
UCLASS()
class DOOMSDAYDEVICE_API UDoomsdaySaveGame : public UFlowSaveGame
{
	GENERATED_BODY()

public:
	/** Mirror of UInventorySubsystem's collected items (Flow.Items.* -> count). */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Doomsday")
	TMap<FGameplayTag, int32> SavedItems;

	/** Mirror of UFactsDBSubsystem's narrative facts (Flow.Facts.* -> value). */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Doomsday")
	TMap<FGameplayTag, int32> SavedFacts;
};

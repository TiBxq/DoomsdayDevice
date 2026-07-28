#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"

#include "InventorySubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FItemCollectedEvent, const FGameplayTag& /*ItemTag*/, int32 /*NewCount*/);

/**
 * Items collected by the player, keyed by gameplay tag (Flow.Items.*).
 */
UCLASS()
class DOOMSDAYDEVICE_API UInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FItemCollectedEvent OnItemCollected;

	UFUNCTION(BlueprintCallable)
	void CollectItem(const FGameplayTag& Tag, const int32 Count = 1);

	UFUNCTION(BlueprintCallable)
	bool HasItem(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintCallable)
	int32 GetItemCount(const FGameplayTag& Tag) const;

	/** Whole item table, for serializing into a save game. */
	const TMap<FGameplayTag, int32>& GetAllItems() const { return Items; }

	/** Replace the whole item table (used when restoring a save). Does not broadcast. */
	void SetItems(const TMap<FGameplayTag, int32>& InItems);

	/** Clear all collected items (used when starting a new game). Does not broadcast. */
	void ResetInventory();

private:
	TMap<FGameplayTag, int32> Items;
};

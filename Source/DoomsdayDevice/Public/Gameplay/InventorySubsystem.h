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

private:
	TMap<FGameplayTag, int32> Items;
};

#include "Gameplay/InventorySubsystem.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventorySubsystem)

void UInventorySubsystem::CollectItem(const FGameplayTag& Tag, const int32 Count)
{
	if (!Tag.IsValid() || Count <= 0)
	{
		return;
	}

	int32& ItemCount = Items.FindOrAdd(Tag);
	ItemCount += Count;

	const int32 NewCount = ItemCount;
	OnItemCollected.Broadcast(Tag, NewCount);

	UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: collected %s (total %d)"), *Tag.ToString(), NewCount);
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Collected %s (total %d)"), *Tag.ToString(), NewCount));
	}
#endif
}

bool UInventorySubsystem::HasItem(const FGameplayTag& Tag) const
{
	return GetItemCount(Tag) > 0;
}

int32 UInventorySubsystem::GetItemCount(const FGameplayTag& Tag) const
{
	const int32* Count = Items.Find(Tag);
	return Count ? *Count : 0;
}

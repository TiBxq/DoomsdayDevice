#include "Flow/Nodes/FlowNode_OnItemCollected.h"
#include "Gameplay/InventorySubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnItemCollected)

UFlowNode_OnItemCollected::UFlowNode_OnItemCollected()
	: RequiredCount(1)
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Condition;
	Category = TEXT("Items");
#endif

	InputPins = {FFlowPin(TEXT("Start")), FFlowPin(TEXT("Stop"))};
	OutputPins = {FFlowPin(TEXT("Success")), FFlowPin(TEXT("Stopped"))};
}

void UFlowNode_OnItemCollected::ExecuteInput(const FName& PinName)
{
	if (!ItemTag.IsValid())
	{
		LogError(TEXT("Item Tag is missing!"));
		return;
	}

	if (PinName == TEXT("Start"))
	{
		if (UInventorySubsystem* Inventory = GetInventory())
		{
			if (Inventory->GetItemCount(ItemTag) >= RequiredCount)
			{
				TriggerOutput(TEXT("Success"), true);
			}
			else
			{
				Inventory->OnItemCollected.RemoveAll(this);
				Inventory->OnItemCollected.AddUObject(this, &UFlowNode_OnItemCollected::OnItemCollected);
			}
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		TriggerOutput(TEXT("Stopped"), true);
	}
}

void UFlowNode_OnItemCollected::OnItemCollected(const FGameplayTag& Tag, const int32 NewCount)
{
	// MatchesTag allows waiting on a parent tag, e.g. Flow.Items.Test matches Flow.Items.Test.Cube;
	// NewCount is the count of the exact collected tag
	if (Tag.MatchesTag(ItemTag) && NewCount >= RequiredCount)
	{
		TriggerOutput(TEXT("Success"), true);
	}
}

void UFlowNode_OnItemCollected::Cleanup()
{
	if (UInventorySubsystem* Inventory = GetInventory())
	{
		Inventory->OnItemCollected.RemoveAll(this);
	}

	Super::Cleanup();
}

UInventorySubsystem* UFlowNode_OnItemCollected::GetInventory() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UInventorySubsystem>();
		}
	}

	return nullptr;
}

#if WITH_EDITOR
FString UFlowNode_OnItemCollected::GetNodeDescription() const
{
	FString Description = ItemTag.ToString();
	if (RequiredCount > 1)
	{
		Description += TEXT(" x ") + FString::FromInt(RequiredCount);
	}

	return Description;
}

EDataValidationResult UFlowNode_OnItemCollected::ValidateNode()
{
	if (!ItemTag.IsValid())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Item Tag is missing!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

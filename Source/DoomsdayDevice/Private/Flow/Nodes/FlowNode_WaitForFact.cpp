// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_WaitForFact.h"
#include "Flow/FactsDBSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

UFlowNode_WaitForFact::UFlowNode_WaitForFact()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Condition;
	Category = TEXT("Facts");
#endif

	InputPins = {FFlowPin(TEXT("Start")), FFlowPin(TEXT("Stop"))};
	OutputPins = {FFlowPin(TEXT("Success")), FFlowPin(TEXT("Stopped"))};
}

void UFlowNode_WaitForFact::ExecuteInput(const FName& PinName)
{
	if (!Tag.IsValid())
	{
		LogError(TEXT("Tag is missing!"));
		return;
	}

	if (PinName == TEXT("Start"))
	{
		if (UFactsDBSubsystem* FactsDB = GetFactsDB())
		{
			if (FactsDB->GetFactValue(Tag) == Value)
			{
				TriggerOutput(TEXT("Success"), true);
			}
			else
			{
				FactsDB->OnFactChanged.RemoveAll(this);
				FactsDB->OnFactChanged.AddUObject(this, &UFlowNode_WaitForFact::OnFactChanged);
			}
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		TriggerOutput(TEXT("Stopped"), true);
	}
}

void UFlowNode_WaitForFact::OnFactChanged(const FGameplayTag& ChangedTag, int32 NewValue)
{
	if (ChangedTag == Tag && NewValue == Value)
	{
		TriggerOutput(TEXT("Success"), true);
	}
}

void UFlowNode_WaitForFact::Cleanup()
{
	if (UFactsDBSubsystem* FactsDB = GetFactsDB())
	{
		FactsDB->OnFactChanged.RemoveAll(this);
	}

	Super::Cleanup();
}

UFactsDBSubsystem* UFlowNode_WaitForFact::GetFactsDB() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UFactsDBSubsystem>();
		}
	}

	return nullptr;
}

#if WITH_EDITOR
FString UFlowNode_WaitForFact::GetNodeDescription() const
{
	return Tag.ToString() + TEXT(": ") + FString::FromInt(Value);
}

EDataValidationResult UFlowNode_WaitForFact::ValidateNode()
{
	if (!Tag.IsValid())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Tag is missing!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

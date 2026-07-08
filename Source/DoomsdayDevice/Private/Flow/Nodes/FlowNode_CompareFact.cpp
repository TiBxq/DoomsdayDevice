// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_CompareFact.h"

#include "Player/BasicUIManager.h"
#include "DoomsdayDevicePlayerController.h"
#include "Flow/FactsDBSubsystem.h"

UFlowNode_CompareFact::UFlowNode_CompareFact()
{
#if WITH_EDITOR
	Category = TEXT("Facts");
#endif

	OutputPins = { FFlowPin(TEXT("Greater")), FFlowPin(TEXT("Equal")), FFlowPin(TEXT("Less")) };
}

void UFlowNode_CompareFact::ExecuteInput(const FName& PinName)
{
	if (GetWorld())
	{
		if (UFactsDBSubsystem* FactsSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFactsDBSubsystem>())
		{
			const int32 FactValue = FactsSubsystem->GetFactValue(Tag);
			if (FactValue > Value)
			{
				TriggerOutput(TEXT("Greater"), true);
			}
			else if (FactValue == Value)
			{
				TriggerOutput(TEXT("Equal"), true);
			}
			else if (FactValue < Value)
			{
				TriggerOutput(TEXT("Less"), true);
			}
		}
	}
}

#if WITH_EDITOR 
FString UFlowNode_CompareFact::GetNodeDescription() const
{
	return Tag.ToString() + TEXT(": ") + FString::FromInt(Value);
}

EDataValidationResult UFlowNode_CompareFact::ValidateNode()
{
	if (!Tag.IsValid())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Tag is missing!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
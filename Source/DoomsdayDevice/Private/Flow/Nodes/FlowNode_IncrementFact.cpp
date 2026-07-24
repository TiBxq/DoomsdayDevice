// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_IncrementFact.h"

#include "Flow/FactsDBSubsystem.h"

UFlowNode_IncrementFact::UFlowNode_IncrementFact()
{
#if WITH_EDITOR
	Category = TEXT("Facts");
#endif
}

void UFlowNode_IncrementFact::ExecuteInput(const FName& PinName)
{
	if (GetWorld())
	{
		if (UFactsDBSubsystem* FactsSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFactsDBSubsystem>())
		{
			FactsSubsystem->IncrementFact(Tag, Value);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_IncrementFact::GetNodeDescription() const
{
	return Tag.ToString() + TEXT(": ") + FString::FromInt(Value);
}

EDataValidationResult UFlowNode_IncrementFact::ValidateNode()
{
	if (!Tag.IsValid())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Tag is missing!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

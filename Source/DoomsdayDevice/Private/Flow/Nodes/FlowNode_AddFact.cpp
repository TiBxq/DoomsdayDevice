// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_AddFact.h"

#include "Player/BasicUIManager.h"
#include "DoomsdayDevicePlayerController.h"
#include "Flow/FactsDBSubsystem.h"

UFlowNode_AddFact::UFlowNode_AddFact()
{
#if WITH_EDITOR
	Category = TEXT("Facts");
#endif
}

void UFlowNode_AddFact::ExecuteInput(const FName& PinName)
{
	if (GetWorld())
	{
		if (UFactsDBSubsystem* FactsSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFactsDBSubsystem>())
		{
			FactsSubsystem->AddFact(Tag, Value);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR 
FString UFlowNode_AddFact::GetNodeDescription() const
{
	return Tag.ToString() + TEXT(": ") + FString::FromInt(Value);
}

EDataValidationResult UFlowNode_AddFact::ValidateNode()
{
	if (!Tag.IsValid())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Tag is missing!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
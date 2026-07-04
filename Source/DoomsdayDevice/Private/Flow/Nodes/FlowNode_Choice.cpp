// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_Choice.h"

#include "Player/BasicUIManager.h"
#include "DoomsdayDevicePlayerController.h"

UFlowNode_Choice::UFlowNode_Choice()
{
#if WITH_EDITOR
	Category = TEXT("Dialogue");
#endif

	OutputPins = { FFlowPin(TEXT("Displayed")), FFlowPin(TEXT("Choice_1")), FFlowPin(TEXT("Choice_2")), FFlowPin(TEXT("Choice_3")), FFlowPin(TEXT("Choice_4")) };
}

void UFlowNode_Choice::ExecuteInput(const FName& PinName)
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			UIManager->SetupDialogueChoices(ChoiceTexts);
			TriggerOutput(TEXT("Displayed"));
		}

		PC->SelectDialogueChoiceEvent.AddDynamic(this, &UFlowNode_Choice::OnChoiceSelected);
	}
}

void UFlowNode_Choice::Cleanup()
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->SelectDialogueChoiceEvent.RemoveAll(this);
	}
	Super::Cleanup();
}

#if WITH_EDITOR 
FString UFlowNode_Choice::GetNodeDescription() const
{
	return Super::GetNodeDescription();
}

EDataValidationResult UFlowNode_Choice::ValidateNode()
{
	if (ChoiceTexts.Num() == 0 || ChoiceTexts.Num() > 4)
	{
		if (ChoiceTexts.Num() > 4)
		{
			ValidationLog.Error<UFlowNode>(TEXT("Too Many Choices"), this);
		}

		if (ChoiceTexts.Num() == 0)
		{
			ValidationLog.Error<UFlowNode>(TEXT("No Choice Texts Assigned"), this);
		}

		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

void UFlowNode_Choice::OnChoiceSelected(int32 Index)
{
	if (Index >= ChoiceTexts.Num())
	{
		return;
	}

	switch (Index)
	{
	case 0:
		TriggerOutput(TEXT("Choice_1"), true);
		break;
	case 1:
		TriggerOutput(TEXT("Choice_2"), true);
		break;
	case 2:
		TriggerOutput(TEXT("Choice_3"), true);
		break;
	case 3:
		TriggerOutput(TEXT("Choice_4"), true);
		break;
	}

	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			UIManager->ClearDialogueChoices();
		}
	}
}
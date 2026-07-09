// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_DialogueLine.h"

#include "Player/BasicUIManager.h"
#include "DoomsdayDevicePlayerController.h"

UFlowNode_DialogueLine::UFlowNode_DialogueLine()
{
#if WITH_EDITOR
	Category = TEXT("Dialogue");
#endif

	OutputPins = { FFlowPin(TEXT("Displayed")), FFlowPin(TEXT("Completed")) };
}

void UFlowNode_DialogueLine::ExecuteInput(const FName& PinName)
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			PC->ContinueDialogueEvent.RemoveDynamic(this, &UFlowNode_DialogueLine::OnDialogueLineCompleted);
			PC->ContinueDialogueEvent.AddDynamic(this, &UFlowNode_DialogueLine::OnDialogueLineCompleted);

			UIManager->DisplayDialogueLine(LineText, SpeakerData);

			TriggerOutput(TEXT("Displayed"));
		}
	}
}

void UFlowNode_DialogueLine::Cleanup()
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->ContinueDialogueEvent.RemoveAll(this);
	}
	Super::Cleanup();
}

#if WITH_EDITOR 
FString UFlowNode_DialogueLine::GetNodeDescription() const
{
	return LineText.ToString();
}

EDataValidationResult UFlowNode_DialogueLine::ValidateNode()
{
	return EDataValidationResult::Valid;
}
#endif

void UFlowNode_DialogueLine::OnDialogueLineCompleted()
{
	TriggerOutput(TEXT("Completed"), true);
}
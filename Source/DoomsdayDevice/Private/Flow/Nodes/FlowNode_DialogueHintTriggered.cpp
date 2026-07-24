// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_DialogueHintTriggered.h"

#include "DoomsdayDevicePlayerController.h"

#include "Engine/World.h"

UFlowNode_DialogueHintTriggered::UFlowNode_DialogueHintTriggered()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Condition;
	Category = TEXT("Dialogue");
#endif

	InputPins = {FFlowPin(TEXT("Start")), FFlowPin(TEXT("Stop"))};
	OutputPins = {FFlowPin(TEXT("Success")), FFlowPin(TEXT("Stopped"))};
}

void UFlowNode_DialogueHintTriggered::ExecuteInput(const FName& PinName)
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (PinName == TEXT("Start"))
		{
			PC->DialogueHintEvent.RemoveDynamic(this, &UFlowNode_DialogueHintTriggered::OnHintTriggered);
			PC->DialogueHintEvent.AddDynamic(this, &UFlowNode_DialogueHintTriggered::OnHintTriggered);
		}
		else if (PinName == TEXT("Stop"))
		{
			TriggerOutput(TEXT("Stopped"), true);
		}
	}
}

void UFlowNode_DialogueHintTriggered::OnHintTriggered()
{
	TriggerOutput(TEXT("Success"), true);
}

void UFlowNode_DialogueHintTriggered::Cleanup()
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->DialogueHintEvent.RemoveAll(this);
	}

	Super::Cleanup();
}

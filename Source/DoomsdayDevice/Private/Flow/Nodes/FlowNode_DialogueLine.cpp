// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_DialogueLine.h"

#include "Player/BasicUIManager.h"
#include "DoomsdayDevicePlayerController.h"
#include "Dialogue/DialogSpeakerDataAsset.h"

#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "FlowNode_DialogueLine"

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

			UIManager->DisplayDialogueLine(LineText, SpeakerData, VoiceOver.LoadSynchronous());

			TriggerOutput(TEXT("Displayed"));
		}
	}
}

void UFlowNode_DialogueLine::Cleanup()
{
	if (const UWorld* World = GetWorld())
	{
		if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(World->GetFirstPlayerController()))
		{
			PC->ContinueDialogueEvent.RemoveAll(this);

			// TriggerOutput runs Finish() before the downstream node, so this can never cut the next line's voice-over.
			if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
			{
				UIManager->StopDialogueVoice();
			}
		}
	}
	Super::Cleanup();
}

void UFlowNode_DialogueLine::OnDialogueLineCompleted()
{
	TriggerOutput(TEXT("Completed"), true);
}

#if WITH_EDITOR
void UFlowNode_DialogueLine::SetDialogueID(const FString& InDialogueID)
{
	DialogueID = InDialogueID;

	// The exporter writes the ID without going through PostEditChangeProperty, so refresh the node face by hand.
	UpdateNodeConfigText();
}

FString UFlowNode_DialogueLine::GetNodeDescription() const
{
	return LineText.ToString();
}

EDataValidationResult UFlowNode_DialogueLine::ValidateNode()
{
	return EDataValidationResult::Valid;
}

void UFlowNode_DialogueLine::UpdateNodeConfigText_Implementation()
{
	TArray<FText> ConfigLines;

	if (!DialogueID.IsEmpty())
	{
		ConfigLines.Add(FText::FromString(DialogueID));
	}

	if (SpeakerData)
	{
		// The VO marker lets authors spot un-voiced lines at a glance across the dialogue graphs.
		const FText VoiceOverInfo = VoiceOver.IsNull()
			? LOCTEXT("DialogueLineNoVoiceOver", "no VO")
			: LOCTEXT("DialogueLineHasVoiceOver", "VO");

		ConfigLines.Add(FText::Format(LOCTEXT("DialogueLineInfo", "Speaker: {0} • {1}"), { SpeakerData->DisplayName, VoiceOverInfo }));
	}

	if (ConfigLines.Num() > 0)
	{
		SetNodeConfigText(FText::Join(FText::FromString(TEXT("\n")), ConfigLines));
	}
}

#endif

#undef LOCTEXT_NAMESPACE
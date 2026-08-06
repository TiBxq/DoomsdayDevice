// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_DialogueLine.h"

#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"
#include "DoomsdayDevicePlayerController.h"
#include "Dialogue/DialogSpeakerDataAsset.h"

#include "Sound/SoundBase.h"
#include "TimerManager.h"

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

			// The master switch is read once per line; toggling it mid-line deliberately has no effect.
			// Subscribed before the line is displayed: an instant reveal broadcasts from inside that call.
			if (bAutoSkip && GetDefault<UPlayerSettings>()->bDialogueAutoSkipEnabled)
			{
				UIManager->OnDialogueLinePresented.RemoveAll(this);
				UIManager->OnDialogueLinePresented.AddUObject(this, &UFlowNode_DialogueLine::OnDialogueLinePresented);
			}

			UIManager->DisplayDialogueLine(LineText, SpeakerData, VoiceOver.LoadSynchronous());

			TriggerOutput(TEXT("Displayed"));
		}
	}
}

void UFlowNode_DialogueLine::Cleanup()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoSkipTimerHandle);

		if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(World->GetFirstPlayerController()))
		{
			PC->ContinueDialogueEvent.RemoveAll(this);

			// TriggerOutput runs Finish() before the downstream node, so this can never cut the next line's voice-over.
			if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
			{
				// Unsubscribed before StopDialogueVoice so this node can't be re-notified during its own teardown.
				UIManager->OnDialogueLinePresented.RemoveAll(this);
				UIManager->StopDialogueVoice();
			}
		}
	}
	AutoSkipTimerHandle.Invalidate();

	Super::Cleanup();
}

void UFlowNode_DialogueLine::OnDialogueLineCompleted()
{
	TriggerOutput(TEXT("Completed"), true);
}

void UFlowNode_DialogueLine::OnDialogueLinePresented()
{
	// The manager can broadcast twice for one line - a single press both cuts the voice-over and snaps the text.
	if (AutoSkipTimerHandle.IsValid())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		// Always a timer, never a direct call: this can arrive from inside the widget's tick or an audio
		// callback, where advancing the graph would tear the widget down mid-frame. A rate <= 0 makes
		// FTimerManager clear the timer instead of firing it, so floor it - it then lands on the next tick.
		World->GetTimerManager().SetTimer(AutoSkipTimerHandle, this, &UFlowNode_DialogueLine::OnAutoSkipElapsed,
			FMath::Max(AutoSkipDelay, UE_KINDA_SMALL_NUMBER), false);
	}
}

void UFlowNode_DialogueLine::OnAutoSkipElapsed()
{
	AutoSkipTimerHandle.Invalidate();

	OnDialogueLineCompleted(); // same path as the continue press
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

	// Its own line rather than part of the speaker row, so speaker-less lines still show it.
	if (bAutoSkip)
	{
		ConfigLines.Add(FText::Format(LOCTEXT("DialogueLineAutoSkip", "Auto-skip: {0}s"), FText::AsNumber(AutoSkipDelay)));
	}

	if (ConfigLines.Num() > 0)
	{
		SetNodeConfigText(FText::Join(FText::FromString(TEXT("\n")), ConfigLines));
	}
}

#endif

#undef LOCTEXT_NAMESPACE
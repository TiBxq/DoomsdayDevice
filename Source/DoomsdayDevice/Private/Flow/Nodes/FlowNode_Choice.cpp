// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_Choice.h"

#include "Dialogue/DialogueSubsystem.h"
#include "Player/BasicUIManager.h"
#include "DoomsdayDevicePlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

UFlowNode_Choice::UFlowNode_Choice()
{
#if WITH_EDITOR
	Category = TEXT("Dialogue");
#endif

	OutputPins = { FFlowPin(TEXT("Displayed")), FFlowPin(TEXT("Choice_1")), FFlowPin(TEXT("Choice_2")), FFlowPin(TEXT("Choice_3")), FFlowPin(TEXT("Choice_4")) };
}

void UFlowNode_Choice::ExecuteInput(const FName& PinName)
{
	// A presentation node only displays as part of the dialogue that owns the screen. Refusal means this branch
	// is an orphan - typically one whose trigger was already in flight when an interrupt landed - so it dies
	// here rather than painting over the dialogue that legitimately owns the screen.
	UDialogueSubsystem* Dialogue = GetWorld() ? GetWorld()->GetSubsystem<UDialogueSubsystem>() : nullptr;
	if (Dialogue && !Dialogue->TryJoinSession(this))
	{
		Finish();
		return;
	}

	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			// Bound before the Displayed branch runs, and inside this block: arming a choice with no buttons on
			// screen would leave the player unable to answer it. Re-triggering an already-active node re-runs
			// ExecuteInput without re-running OnActivate, so pair the bind with a RemoveDynamic - a plain
			// AddDynamic double-binds and one keypress then confirms the choice twice.
			PC->SelectDialogueChoiceEvent.RemoveDynamic(this, &UFlowNode_Choice::OnChoiceSelected);
			PC->SelectDialogueChoiceEvent.AddDynamic(this, &UFlowNode_Choice::OnChoiceSelected);

			UIManager->SetupDialogueChoices(ChoiceTexts);
			TriggerOutput(TEXT("Displayed"));
		}
	}
}

void UFlowNode_Choice::Cleanup()
{
	const UWorld* World = GetWorld();
	if (ADoomsdayDevicePlayerController* PC = World ? Cast<ADoomsdayDevicePlayerController>(World->GetFirstPlayerController()) : nullptr)
	{
		PC->SelectDialogueChoiceEvent.RemoveAll(this);

		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			UIManager->OnDialogueChoiceConfirmed.RemoveAll(this);

			// The only teardown an aborted node gets, so the buttons must come off here rather than on the
			// confirmed path. TriggerOutput runs Finish() -> Cleanup() before the downstream branch, so this
			// still clears ahead of the next line on the normal path too.
			UIManager->ClearDialogueChoices();
		}
	}

	if (UDialogueSubsystem* Dialogue = World ? World->GetSubsystem<UDialogueSubsystem>() : nullptr)
	{
		Dialogue->LeaveSession(this);
	}

	Super::Cleanup();
}

void UFlowNode_Choice::ForceFinishNode()
{
	Super::ForceFinishNode(); // cleans up AddOns and fires the Blueprint hook - it does not finish the node

	// UFlowNodeBase::ForceFinishNode is only a notification; nothing in the Flow plugin ever finishes the node
	// for us. Finish() deactivates, runs Cleanup() and drops this node from the asset's ActiveNodes without
	// triggering any output pin, so an aborted dialogue branch stops here instead of running on.
	if (!HasFinished())
	{
		Finish();
	}
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

	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			PC->SelectDialogueChoiceEvent.RemoveAll(this);

			UIManager->ConfirmDialogueChoice(Index);
			UIManager->OnDialogueChoiceConfirmed.AddDynamic(this, &UFlowNode_Choice::OnChoiceConfirmed);
		}
	}
}

void UFlowNode_Choice::OnChoiceConfirmed(int32 Index)
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

	// No ClearDialogueChoices() here: TriggerOutput(.., true) already ran Finish() -> Cleanup(), which clears.
	// Clearing after the trigger used to race the downstream branch - whether it landed before or after depended
	// on FlowAsset's deferred-transition scope stack, i.e. on graph topology.
}
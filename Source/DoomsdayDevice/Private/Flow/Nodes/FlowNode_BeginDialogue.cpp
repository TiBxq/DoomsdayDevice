// Fill out your copyright notice in the Description page of Project Settings.

#include "Flow/Nodes/FlowNode_BeginDialogue.h"

#include "Flow/Nodes/FlowNode_EndDialogue.h"

#include "FlowAsset.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_BeginDialogue)

#define LOCTEXT_NAMESPACE "FlowNode_BeginDialogue"

UFlowNode_BeginDialogue::UFlowNode_BeginDialogue()
{
#if WITH_EDITOR
	Category = TEXT("Dialogue");
	NodeDisplayStyle = FlowNodeStyle::Latent;
#endif

	InputPins = {FFlowPin(TEXT("Start")), FFlowPin(TEXT("Stop"))};
	OutputPins = {FFlowPin(TEXT("Started")), FFlowPin(TEXT("Cancelled"))};
}

UDialogueSubsystem* UFlowNode_BeginDialogue::GetDialogueSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UDialogueSubsystem>() : nullptr;
}

void UFlowNode_BeginDialogue::ExecuteInput(const FName& PinName)
{
	UDialogueSubsystem* Dialogue = GetDialogueSubsystem();
	if (!Dialogue)
	{
		// No arbitration available (a world type without the subsystem). Fail open rather than swallow the
		// dialogue: the old, unarbitrated behaviour is still better than a branch that silently stops.
		if (PinName == TEXT("Start"))
		{
			TriggerOutput(TEXT("Started"));
		}
		return;
	}

	if (PinName == TEXT("Stop"))
	{
		if (!bRequestActive)
		{
			return;
		}

		// Only really meaningful while queued: it lets a dialogue give up when the moment for it has passed.
		bRequestActive = false;
		SavedState = EDialogueSavedState::NotStarted;

		Dialogue->CancelRequest(this);
		Dialogue->ReleaseIfOwner(this);

		TriggerOutput(TEXT("Cancelled"), true);
		return;
	}

	if (bRequestActive)
	{
		// Re-entering an already-active node re-runs ExecuteInput without re-running OnActivate, so a second
		// Start would queue this node twice, or make it ask for a screen it already owns.
		LogError(TEXT("Begin Dialogue was re-triggered while it still owns or is waiting for the dialogue screen"));
		return;
	}

	bRequestActive = true;
	HandleAcquireResult(Dialogue->TryAcquire(this, BusyPolicy));
}

void UFlowNode_BeginDialogue::HandleAcquireResult(const EDialogueAcquireResult Result)
{
	switch (Result)
	{
	case EDialogueAcquireResult::Started:
		SavedState = EDialogueSavedState::Holding;

		// bFinish stays false: the node has to stay active for the whole dialogue, both to hold the screen and
		// so it still has a Cancelled pin to trigger if something interrupts it later.
		TriggerOutput(TEXT("Started"));
		break;

	case EDialogueAcquireResult::Queued:
		SavedState = EDialogueSavedState::Waiting;
		break;

	case EDialogueAcquireResult::Dropped:
	default:
		bRequestActive = false;
		SavedState = EDialogueSavedState::NotStarted;
		TriggerOutput(TEXT("Cancelled"), true);
		break;
	}
}

void UFlowNode_BeginDialogue::NotifySessionStarted()
{
	SavedState = EDialogueSavedState::Holding;
	TriggerOutput(TEXT("Started"));
}

void UFlowNode_BeginDialogue::NotifySessionCancelled()
{
	bRequestActive = false;
	SavedState = EDialogueSavedState::NotStarted;

	TriggerOutput(TEXT("Cancelled"), true);
}

void UFlowNode_BeginDialogue::NotifySessionEnded()
{
	bRequestActive = false;
	SavedState = EDialogueSavedState::NotStarted;

	// The dialogue played out normally, so no pin fires here - the End Dialogue node carries execution onward.
	if (!HasFinished())
	{
		Finish();
	}
}

void UFlowNode_BeginDialogue::Cleanup()
{
	// Reached on every teardown path - normal completion, an aborted graph, sub-graph teardown, PIE stop - so
	// this is what guarantees the screen is never left held by a node that no longer exists.
	if (UDialogueSubsystem* Dialogue = GetDialogueSubsystem())
	{
		Dialogue->CancelRequest(this);
		Dialogue->ReleaseIfOwner(this);
	}

	bRequestActive = false;

	Super::Cleanup();
}

void UFlowNode_BeginDialogue::ForceFinishNode()
{
	Super::ForceFinishNode();

	if (!HasFinished())
	{
		Finish();
	}
}

void UFlowNode_BeginDialogue::OnSave_Implementation()
{
	// SavedState is kept up to date as the node changes hands, and nothing else is worth persisting. Session
	// identity deliberately is not saved: the subsystem is world-scoped and comes back empty for a loaded world.
}

void UFlowNode_BeginDialogue::OnLoad_Implementation()
{
	UDialogueSubsystem* Dialogue = GetDialogueSubsystem();
	if (!Dialogue)
	{
		return;
	}

	switch (SavedState)
	{
	case EDialogueSavedState::Waiting:
		// Still owed its turn, so ask again. Queue order follows the order the nodes are restored in.
		bRequestActive = true;
		HandleAcquireResult(Dialogue->TryAcquire(this, BusyPolicy));
		break;

	case EDialogueSavedState::Holding:
		// The checkpoint was taken mid-dialogue, and the line nodes do not restore what was on screen. Rather
		// than resume into a blank screen, abandon the dialogue down Cancelled: the designer's cleanup runs, a
		// sub-graph still reaches its Finish node, and arbitration starts from a clean slate.
		UE_LOG(LogDialogueSession, Warning,
			TEXT("Begin Dialogue in %s held the screen when this checkpoint was taken; cancelling it on load."),
			GetFlowAsset() ? *GetFlowAsset()->GetName() : TEXT("<no asset>"));

		bRequestActive = false;
		SavedState = EDialogueSavedState::NotStarted;
		TriggerOutput(TEXT("Cancelled"), true);
		break;

	default:
		break;
	}
}

#if WITH_EDITOR
FString UFlowNode_BeginDialogue::GetNodeDescription() const
{
	switch (BusyPolicy)
	{
	case EDialogueBusyPolicy::Interrupt:
		return TEXT("Cancels any dialogue already playing");
	case EDialogueBusyPolicy::Drop:
		return TEXT("Skipped entirely if a dialogue is already playing");
	case EDialogueBusyPolicy::Wait:
	default:
		return TEXT("Waits for any dialogue already playing");
	}
}

void UFlowNode_BeginDialogue::UpdateNodeConfigText_Implementation()
{
	const UEnum* PolicyEnum = StaticEnum<EDialogueBusyPolicy>();
	SetNodeConfigText(FText::Format(LOCTEXT("BeginDialogueConfig", "If busy: {0}"),
		PolicyEnum ? PolicyEnum->GetDisplayNameTextByValue(static_cast<int64>(BusyPolicy)) : FText::GetEmpty()));
}

EDataValidationResult UFlowNode_BeginDialogue::ValidateNode()
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	// A dropped or interrupted dialogue leaves through Cancelled and nowhere else. If that pin dangles the
	// branch dies silently - and inside a sub-graph the parent's Sub Graph node then never finishes.
	if (!GetConnection(TEXT("Cancelled")).NodeGuid.IsValid())
	{
		ValidationLog.Error<UFlowNode>(
			TEXT("Cancelled is not connected - a dropped or interrupted dialogue would stop here with no cleanup. "
				 "Route it onward, and inside a sub-graph route it to Finish."), this);
		Result = EDataValidationResult::Invalid;
	}

	if (const UFlowAsset* Asset = GetFlowAsset())
	{
		int32 BeginCount = 0;
		bool bHasEnd = false;

		for (const TPair<FGuid, UFlowNode*>& Pair : Asset->GetNodes())
		{
			if (!Pair.Value)
			{
				continue;
			}

			if (Pair.Value->IsA<UFlowNode_BeginDialogue>())
			{
				++BeginCount;
			}
			else if (Pair.Value->IsA<UFlowNode_EndDialogue>())
			{
				bHasEnd = true;
			}
		}

		if (BeginCount > 1)
		{
			ValidationLog.Warning<UFlowNode>(
				TEXT("More than one Begin Dialogue in this graph. Dialogue lines are matched to a session by the "
					 "graph they live in, so give each dialogue its own sub-graph."), this);
		}

		if (!bHasEnd)
		{
			ValidationLog.Warning<UFlowNode>(
				TEXT("No End Dialogue in this graph. The screen is released when this node finishes, but the "
					 "dialogue widget itself will not be closed."), this);
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE

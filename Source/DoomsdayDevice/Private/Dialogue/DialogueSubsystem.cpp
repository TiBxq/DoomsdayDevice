// Fill out your copyright notice in the Description page of Project Settings.

#include "Dialogue/DialogueSubsystem.h"

#include "Flow/Nodes/FlowNode_BeginDialogue.h"
#include "Flow/Nodes/FlowNode_EndDialogue.h"
#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueSubsystem)

DEFINE_LOG_CATEGORY(LogDialogueSession);

namespace
{
	FString DescribeNode(const UFlowNode* Node)
	{
		if (!Node)
		{
			return TEXT("<null>");
		}

		const UFlowAsset* Asset = Node->GetFlowAsset();
		return FString::Printf(TEXT("%s (%s)"), *Node->GetName(), Asset ? *Asset->GetName() : TEXT("<no asset>"));
	}
}

void UDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UDialogueSubsystem::Deinitialize()
{
	// The world is going away and the Flow node instances with it. Drop everything without firing Cancelled:
	// those nodes are mid-teardown, and UFlowSubsystem is GameInstance-scoped so its shutdown order relative to
	// world subsystems is not guaranteed. Nothing here reaches into UBasicUIManager either - that subsystem
	// outlives the world, and the local player may already be gone.
	DisarmCloseWatchdog();
	UnbindCloseDelegate();

	CurrentSession.Reset();
	WaitingQueue.Reset();
	PendingCancelDispatch.Reset();
	PendingEndDialogueNode.Reset();
	SlotState = EDialogueSlotState::Idle;
	bStartDispatchPending = false;
	bArbitrating = false;

	Super::Deinitialize();
}

bool UDialogueSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Game and PIE only. The base class would also build one for the level editor world, where it would tick
	// forever with nothing to arbitrate and answer DumpDialogueState ahead of the PIE world's instance.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UDialogueSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDialogueSubsystem, STATGROUP_Tickables);
}

bool UDialogueSubsystem::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return HasAnyFlags(RF_ClassDefaultObject) ? false : ProcessConsoleExec(Cmd, Ar, nullptr);
}

UBasicUIManager* UDialogueSubsystem::GetUIManager() const
{
	const UWorld* World = GetWorld();
	if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			return LocalPlayer->GetSubsystem<UBasicUIManager>();
		}
	}

	return nullptr;
}

void UDialogueSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Cancels before starts: during an interrupt the slot is still Closing, so a Begin Dialogue reached from a
	// Cancelled branch queues behind the interrupter rather than stealing the screen from it.
	DispatchPendingCancels();
	DispatchPendingStart();

	if (CloseWatchdogDeadline > 0.0 && FPlatformTime::Seconds() >= CloseWatchdogDeadline)
	{
		UE_LOG(LogDialogueSession, Error,
			TEXT("Dialogue screen never reported its close within %.1fs - force-closing so dialogue can continue. ")
			TEXT("Check that the widget's OnDialogueClose animation has its Finished pin wired to FinishDialogueClose."),
			GetDefault<UPlayerSettings>()->DialogueCloseTimeoutSeconds);

		DisarmCloseWatchdog();

		if (UBasicUIManager* UIManager = GetUIManager())
		{
			UIManager->ForceCloseDialogueWidget();
		}

		FinishClose();
	}

	// An implicit session belongs to a legacy graph with no Begin/End Dialogue pair to close it out. If it has
	// had nothing on screen for a couple of ticks, retire it rather than let it hold the slot forever.
	if (SlotState == EDialogueSlotState::Running && CurrentSession.bImplicit)
	{
		const bool bAnyAlive = CurrentSession.PresentationNodes.ContainsByPredicate(
			[](const TWeakObjectPtr<UFlowNode>& Node) { return Node.IsValid(); });

		CurrentSession.EmptyTicks = bAnyAlive ? 0 : CurrentSession.EmptyTicks + 1;

		if (CurrentSession.EmptyTicks > 2)
		{
			UE_LOG(LogDialogueSession, Verbose, TEXT("Retiring stranded implicit session (%s)."),
				CurrentSession.OwningAsset.IsValid() ? *CurrentSession.OwningAsset->GetName() : TEXT("<stale>"));

			CurrentSession.Reset();
			SlotState = EDialogueSlotState::Idle;
			bStartDispatchPending = true;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// Begin Dialogue
// ---------------------------------------------------------------------------------------------------------

EDialogueAcquireResult UDialogueSubsystem::TryAcquire(UFlowNode_BeginDialogue* Requester, const EDialogueBusyPolicy Policy)
{
	if (!Requester)
	{
		return EDialogueAcquireResult::Dropped;
	}

	if (!IsBusy())
	{
		CurrentSession.Reset();
		CurrentSession.Owner = Requester;
		CurrentSession.OwningAsset = Requester->GetFlowAsset();
		SlotState = EDialogueSlotState::Running;

		UE_LOG(LogDialogueSession, Log, TEXT("Acquire: %s started immediately."), *DescribeNode(Requester));
		return EDialogueAcquireResult::Started;
	}

	switch (Policy)
	{
	case EDialogueBusyPolicy::Drop:
		UE_LOG(LogDialogueSession, Log, TEXT("Acquire: %s dropped (screen busy)."), *DescribeNode(Requester));
		return EDialogueAcquireResult::Dropped;

	case EDialogueBusyPolicy::Interrupt:
	{
		if (bArbitrating)
		{
			// Reached from a Cancelled branch running inside an interrupt we are still performing. Queue it
			// instead - interrupting an interrupt would recurse through the state we are halfway through.
			UE_LOG(LogDialogueSession, Error,
				TEXT("Acquire: %s asked to interrupt from inside an interrupt - queued instead. A Cancelled ")
				TEXT("branch should not start another interrupting dialogue."), *DescribeNode(Requester));

			WaitingQueue.AddUnique(Requester);
			return EDialogueAcquireResult::Queued;
		}

		if (CurrentSession.bImplicit)
		{
			// A legacy dialogue with no Begin Dialogue node. Its presentation nodes did register, so the abort
			// below still tears the UI down cleanly - there is just no Cancelled pin to route cleanup through.
			UE_LOG(LogDialogueSession, Warning,
				TEXT("Acquire: %s is interrupting an untracked dialogue (%s). That dialogue's branch will stop ")
				TEXT("with no cleanup path - give it a Begin Dialogue node."), *DescribeNode(Requester),
				CurrentSession.OwningAsset.IsValid() ? *CurrentSession.OwningAsset->GetName() : TEXT("<stale>"));
		}

		TGuardValue<bool> ArbitrationGuard(bArbitrating, true);

		FDialogueSession Outgoing = CurrentSession;
		CurrentSession.Reset();

		UE_LOG(LogDialogueSession, Log, TEXT("Acquire: %s is interrupting %s."), *DescribeNode(Requester),
			*DescribeNode(Outgoing.Owner.Get()));

		KillPresentationNodes(Outgoing);

		if (UBasicUIManager* UIManager = GetUIManager())
		{
			UIManager->StopDialogueVoice();
			UIManager->ClearDialogueChoices();
		}

		if (Outgoing.Owner.IsValid())
		{
			PendingCancelDispatch.AddUnique(Outgoing.Owner);
		}

		// Front of the queue: the interrupter outranks anything already waiting.
		WaitingQueue.Remove(Requester);
		WaitingQueue.Insert(Requester, 0);

		BeginClose(nullptr);
		return EDialogueAcquireResult::Queued;
	}

	case EDialogueBusyPolicy::Wait:
	default:
		WaitingQueue.AddUnique(Requester);
		UE_LOG(LogDialogueSession, Log, TEXT("Acquire: %s queued at position %d."), *DescribeNode(Requester),
			WaitingQueue.Num() - 1);
		return EDialogueAcquireResult::Queued;
	}
}

void UDialogueSubsystem::CancelRequest(UFlowNode_BeginDialogue* Requester)
{
	if (WaitingQueue.Remove(Requester) > 0)
	{
		UE_LOG(LogDialogueSession, Log, TEXT("Cancel: %s left the wait queue."), *DescribeNode(Requester));
	}

	PendingCancelDispatch.Remove(Requester);
}

void UDialogueSubsystem::ReleaseIfOwner(UFlowNode_BeginDialogue* Node)
{
	WaitingQueue.Remove(Node);
	PendingCancelDispatch.Remove(Node);

	if (CurrentSession.Owner.Get() != Node)
	{
		return;
	}

	// The owner finished without going through End Dialogue: the graph was torn down, aborted, or simply
	// authored without one. Free the screen so the rest of the game's dialogue keeps working.
	UE_LOG(LogDialogueSession, Verbose, TEXT("Release: owner %s finished; freeing the screen."), *DescribeNode(Node));

	CurrentSession.Reset();

	if (SlotState == EDialogueSlotState::Running)
	{
		SlotState = EDialogueSlotState::Idle;
		bStartDispatchPending = true;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Presentation nodes
// ---------------------------------------------------------------------------------------------------------

bool UDialogueSubsystem::IsAssetInCurrentSession(const UFlowAsset* Asset) const
{
	const UFlowAsset* SessionAsset = CurrentSession.OwningAsset.Get();
	if (!SessionAsset || !Asset)
	{
		return false;
	}

	// Walk up through sub-graph instances, so a Begin Dialogue in a parent graph still owns lines authored
	// inside a child sub-graph.
	for (const UFlowAsset* It = Asset; It; It = It->GetParentInstance())
	{
		if (It == SessionAsset)
		{
			return true;
		}
	}

	return false;
}

bool UDialogueSubsystem::TryJoinSession(UFlowNode* PresentationNode)
{
	if (!PresentationNode)
	{
		return false;
	}

	if (SlotState == EDialogueSlotState::Running && IsAssetInCurrentSession(PresentationNode->GetFlowAsset()))
	{
		CurrentSession.PresentationNodes.AddUnique(PresentationNode);
		CurrentSession.EmptyTicks = 0;
		return true;
	}

	if (SlotState == EDialogueSlotState::Idle)
	{
		// A dialogue with no Begin Dialogue node. Open an implicit session so it still owns the screen
		// exclusively - that is what lets a Begin Dialogue elsewhere wait behind it, or interrupt it.
		CurrentSession.Reset();
		CurrentSession.OwningAsset = PresentationNode->GetFlowAsset();
		CurrentSession.bImplicit = true;
		CurrentSession.PresentationNodes.Add(PresentationNode);
		SlotState = EDialogueSlotState::Running;

		UE_LOG(LogDialogueSession, Warning,
			TEXT("Untracked dialogue started at %s - with no Begin Dialogue node it cannot be queued behind or ")
			TEXT("cancelled cleanly. Add one at the head of this dialogue."), *DescribeNode(PresentationNode));

		return true;
	}

	// Either another dialogue owns the screen or the screen is closing. Refusing here is what stops an orphaned
	// branch - typically one whose trigger was already queued when an interrupt landed - from painting over the
	// dialogue that legitimately owns the screen.
	if (SlotState == EDialogueSlotState::Closing)
	{
		// Transient and expected: the previous dialogue is still animating out.
		UE_LOG(LogDialogueSession, Verbose, TEXT("Refused %s while the screen was closing."), *DescribeNode(PresentationNode));
	}
	else
	{
		// A second dialogue tried to talk over the one already playing. With a Begin Dialogue node it would have
		// waited its turn; without one there is nowhere to wait, so this branch stops here instead. Losing it is
		// still better than overwriting a line - or a pending choice - the player is in the middle of.
		UE_LOG(LogDialogueSession, Warning,
			TEXT("%s tried to display while another dialogue owns the screen, and stopped instead. Give its ")
			TEXT("dialogue a Begin Dialogue node so it can wait its turn."), *DescribeNode(PresentationNode));
	}

	return false;
}

void UDialogueSubsystem::LeaveSession(UFlowNode* PresentationNode)
{
	CurrentSession.PresentationNodes.Remove(PresentationNode);
}

void UDialogueSubsystem::KillPresentationNodes(FDialogueSession& Session)
{
	// Copy first: ForceFinishNode -> Finish -> Cleanup calls back into LeaveSession, which mutates the array.
	TArray<TWeakObjectPtr<UFlowNode>> Victims = Session.PresentationNodes;
	Session.PresentationNodes.Reset();

	for (const TWeakObjectPtr<UFlowNode>& Victim : Victims)
	{
		if (UFlowNode* Node = Victim.Get())
		{
			if (!Node->HasFinished())
			{
				Node->ForceFinishNode();
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// End Dialogue / closing
// ---------------------------------------------------------------------------------------------------------

void UDialogueSubsystem::RequestEndDialogue(UFlowNode_EndDialogue* Node)
{
	if (!Node)
	{
		return;
	}

	const bool bOwnsScreen = SlotState == EDialogueSlotState::Running && IsAssetInCurrentSession(Node->GetFlowAsset());
	if (!bOwnsScreen)
	{
		// Almost always an End Dialogue sitting on a cancelled dialogue's Cancelled branch. It must not touch the
		// UI: the screen now belongs to whichever dialogue interrupted it, and closing it here would strand that
		// one. Let the node continue anyway, so its branch can still reach the sub-graph's Finish node.
		UE_LOG(LogDialogueSession, Verbose,
			TEXT("End Dialogue %s does not own the screen - passing through without touching the UI."),
			*DescribeNode(Node));

		Node->OnDialogueCloseCompleted();
		return;
	}

	if (UBasicUIManager* UIManager = GetUIManager())
	{
		// Stop first, so ending a dialogue off a line's Displayed pin can't leave a voice talking over a closed screen.
		UIManager->StopDialogueVoice();
		UIManager->ClearDialogueChoices();
	}

	UE_LOG(LogDialogueSession, Log, TEXT("End: %s is closing the screen."), *DescribeNode(Node));

	BeginClose(Node);
}

void UDialogueSubsystem::BeginClose(UFlowNode_EndDialogue* EndNode)
{
	SlotState = EDialogueSlotState::Closing;
	PendingEndDialogueNode = EndNode;

	UBasicUIManager* UIManager = GetUIManager();
	if (UIManager && UIManager->CloseDialogue())
	{
		// CloseDialogue only asks the widget to play its close animation; the widget calls back into the manager
		// once it finishes. Until then the slot stays Closing, so nothing can write into a dying widget.
		BindCloseDelegate();
		ArmCloseWatchdog();
		return;
	}

	// No screen was open - there is nothing to wait for.
	FinishClose();
}

void UDialogueSubsystem::OnUiCloseFinished()
{
	if (SlotState != EDialogueSlotState::Closing)
	{
		return; // stray broadcast, e.g. a widget Blueprint calling FinishDialogueClose twice
	}

	FinishClose();
}

void UDialogueSubsystem::FinishClose()
{
	DisarmCloseWatchdog();
	UnbindCloseDelegate();

	UFlowNode_BeginDialogue* Owner = CurrentSession.Owner.Get();
	UFlowNode_EndDialogue* EndNode = PendingEndDialogueNode.Get();

	// Cleared before anyone is notified: the owner's Finish() runs Cleanup, which calls back into ReleaseIfOwner.
	CurrentSession.Reset();
	PendingEndDialogueNode.Reset();
	SlotState = EDialogueSlotState::Idle;
	bStartDispatchPending = true;

	// Owner first, so it has left the asset's ActiveNodes before the End Dialogue branch runs on into whatever
	// follows it - typically the sub-graph's Finish node.
	if (Owner)
	{
		Owner->NotifySessionEnded();
	}

	if (EndNode)
	{
		EndNode->OnDialogueCloseCompleted();
	}
}

void UDialogueSubsystem::BindCloseDelegate()
{
	if (bCloseDelegateBound)
	{
		return;
	}

	if (UBasicUIManager* UIManager = GetUIManager())
	{
		UIManager->OnDialogueCloseFinished.AddDynamic(this, &UDialogueSubsystem::OnUiCloseFinished);
		bCloseDelegateBound = true;
	}
}

void UDialogueSubsystem::UnbindCloseDelegate()
{
	if (!bCloseDelegateBound)
	{
		return;
	}

	if (UBasicUIManager* UIManager = GetUIManager())
	{
		UIManager->OnDialogueCloseFinished.RemoveAll(this);
	}

	bCloseDelegateBound = false;
}

void UDialogueSubsystem::ArmCloseWatchdog()
{
	const float Timeout = GetDefault<UPlayerSettings>()->DialogueCloseTimeoutSeconds;
	CloseWatchdogDeadline = Timeout > 0.f ? FPlatformTime::Seconds() + Timeout : 0.0;
}

void UDialogueSubsystem::DisarmCloseWatchdog()
{
	CloseWatchdogDeadline = 0.0;
}

// ---------------------------------------------------------------------------------------------------------
// Deferred dispatch
// ---------------------------------------------------------------------------------------------------------

void UDialogueSubsystem::DispatchPendingCancels()
{
	if (PendingCancelDispatch.Num() == 0)
	{
		return;
	}

	TArray<TWeakObjectPtr<UFlowNode_BeginDialogue>> Cancelling = MoveTemp(PendingCancelDispatch);
	PendingCancelDispatch.Reset();

	for (const TWeakObjectPtr<UFlowNode_BeginDialogue>& It : Cancelling)
	{
		if (UFlowNode_BeginDialogue* Node = It.Get())
		{
			Node->NotifySessionCancelled();
		}
	}
}

void UDialogueSubsystem::DispatchPendingStart()
{
	if (!bStartDispatchPending || SlotState != EDialogueSlotState::Idle)
	{
		return;
	}

	bStartDispatchPending = false;

	while (WaitingQueue.Num() > 0)
	{
		const TWeakObjectPtr<UFlowNode_BeginDialogue> Next = WaitingQueue[0];
		WaitingQueue.RemoveAt(0);

		UFlowNode_BeginDialogue* Node = Next.Get();
		if (!Node || Node->HasFinished())
		{
			continue; // its graph went away while it waited
		}

		CurrentSession.Reset();
		CurrentSession.Owner = Node;
		CurrentSession.OwningAsset = Node->GetFlowAsset();
		SlotState = EDialogueSlotState::Running;

		UE_LOG(LogDialogueSession, Log, TEXT("Start: %s promoted from the wait queue."), *DescribeNode(Node));

		Node->NotifySessionStarted();
		return;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------------------------------------

void UDialogueSubsystem::DumpDialogueState()
{
	const TCHAR* StateName =
		SlotState == EDialogueSlotState::Idle ? TEXT("Idle") :
		SlotState == EDialogueSlotState::Running ? TEXT("Running") : TEXT("Closing");

	UE_LOG(LogDialogueSession, Display, TEXT("--- Dialogue screen: %s ---"), StateName);
	UE_LOG(LogDialogueSession, Display, TEXT("  owner      : %s%s"), *DescribeNode(CurrentSession.Owner.Get()),
		CurrentSession.bImplicit ? TEXT(" [implicit]") : TEXT(""));
	UE_LOG(LogDialogueSession, Display, TEXT("  asset      : %s"),
		CurrentSession.OwningAsset.IsValid() ? *CurrentSession.OwningAsset->GetName() : TEXT("<none>"));

	for (const TWeakObjectPtr<UFlowNode>& Node : CurrentSession.PresentationNodes)
	{
		UE_LOG(LogDialogueSession, Display, TEXT("  presenting : %s"), *DescribeNode(Node.Get()));
	}

	for (int32 Index = 0; Index < WaitingQueue.Num(); ++Index)
	{
		UE_LOG(LogDialogueSession, Display, TEXT("  queued[%d]  : %s"), Index, *DescribeNode(WaitingQueue[Index].Get()));
	}

	for (const TWeakObjectPtr<UFlowNode_BeginDialogue>& Node : PendingCancelDispatch)
	{
		UE_LOG(LogDialogueSession, Display, TEXT("  cancelling : %s"), *DescribeNode(Node.Get()));
	}
}

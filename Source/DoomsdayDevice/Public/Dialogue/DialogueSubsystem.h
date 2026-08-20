// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/CoreMisc.h"
#include "Subsystems/WorldSubsystem.h"

#include "DialogueSubsystem.generated.h"

class UBasicUIManager;
class UFlowAsset;
class UFlowNode;
class UFlowNode_BeginDialogue;
class UFlowNode_EndDialogue;

DECLARE_LOG_CATEGORY_EXTERN(LogDialogueSession, Log, All);

/** What a Begin Dialogue node does when another dialogue already owns the screen. */
UENUM()
enum class EDialogueBusyPolicy : uint8
{
	/** Queue up, and play once the active dialogue has finished and its screen has closed. */
	Wait UMETA(DisplayName = "Wait"),

	/** Cancel the active dialogue, then take the screen once it has closed. */
	Interrupt UMETA(DisplayName = "Interrupt"),

	/** Never play; fire Cancelled straight away. Use for ambient barks that are only worth hearing in the moment. */
	Drop UMETA(DisplayName = "Drop")
};

/** Intent persisted through a save, so a queued or holding node can restore itself (see UFlowNode_BeginDialogue::OnLoad). */
UENUM()
enum class EDialogueSavedState : uint8
{
	NotStarted,
	Waiting,
	Holding
};

enum class EDialogueAcquireResult : uint8
{
	/** The caller owns the screen now and should trigger its Started pin. */
	Started,
	/** The caller will be called back later via NotifySessionStarted or NotifySessionCancelled. */
	Queued,
	/** The caller will never play and should trigger its Cancelled pin. */
	Dropped
};

enum class EDialogueSlotState : uint8
{
	Idle,
	Running,
	/** The screen is animating closed; the next owner starts once that finishes. */
	Closing
};

/** The dialogue that currently owns the screen, plus the presentation nodes playing under it. */
struct FDialogueSession
{
	/** Null for an implicit session - a legacy dialogue with no Begin Dialogue node. */
	TWeakObjectPtr<UFlowNode_BeginDialogue> Owner;

	/** The Flow asset instance this dialogue lives in; presentation nodes join by asset lineage. */
	TWeakObjectPtr<UFlowAsset> OwningAsset;

	/** Dialogue Line / Choice nodes currently playing. These are what an interrupt force-finishes. */
	TArray<TWeakObjectPtr<UFlowNode>> PresentationNodes;

	bool bImplicit = false;

	/** Ticks this session has spent with nothing on screen; only used to retire stranded implicit sessions. */
	int32 EmptyTicks = 0;

	void Reset()
	{
		Owner.Reset();
		OwningAsset.Reset();
		PresentationNodes.Reset();
		bImplicit = false;
		EmptyTicks = 0;
	}
};

/**
 * Arbitrates which dialogue owns the screen, so a second dialogue can never overwrite a line - or worse, a
 * pending Choice - that the player is still reading. Exactly one session is active at a time; others wait in a
 * FIFO queue, drop, or interrupt, according to their Begin Dialogue node's busy policy.
 *
 * World-scoped on purpose. The state is weak pointers into world-bound Flow node instances plus a widget in a
 * viewport, so surviving level travel would be a bug: it would carry "busy" into the next level and starve its
 * first dialogue. It also means there is nothing here to serialize - the nodes restore their own intent on load.
 *
 * Single-player assumption throughout: the UI is reached via the first local player controller.
 */
UCLASS()
class DOOMSDAYDEVICE_API UDialogueSubsystem : public UTickableWorldSubsystem, public FSelfRegisteringExec
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// UWorldSubsystem - a dialogue session is play-time state, so the editor world has no use for one.
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// FSelfRegisteringExec
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/** Console: log the slot state, the current session and the wait queue. */
	UFUNCTION(Exec, Category = "Dialogue")
	void DumpDialogueState();

	/** True while any dialogue owns or is releasing the screen. Deliberately never consults widget state. */
	bool IsBusy() const { return SlotState != EDialogueSlotState::Idle; }

	// ---------- Begin Dialogue ----------

	/** Ask for the screen on behalf of a Begin Dialogue node. See EDialogueAcquireResult for how to react. */
	EDialogueAcquireResult TryAcquire(UFlowNode_BeginDialogue* Requester, EDialogueBusyPolicy Policy);

	/** Drop a queued request (the node's Stop pin). Safe to call when the node is not queued. */
	void CancelRequest(UFlowNode_BeginDialogue* Requester);

	/** Release the screen if this node owns it. Called from the node's Cleanup, so any teardown path frees the slot. */
	void ReleaseIfOwner(UFlowNode_BeginDialogue* Node);

	// ---------- Presentation nodes (Dialogue Line / Choice) ----------

	/**
	 * Register a presentation node against the running session.
	 * Returns false when the node does not belong to it - the caller must then Finish() and display nothing,
	 * which is what stops an orphaned branch painting over a live dialogue.
	 */
	bool TryJoinSession(UFlowNode* PresentationNode);

	void LeaveSession(UFlowNode* PresentationNode);

	// ---------- End Dialogue ----------

	/** Close the screen and release the slot. The node is called back once, via OnDialogueCloseCompleted. */
	void RequestEndDialogue(UFlowNode_EndDialogue* Node);

private:
	UBasicUIManager* GetUIManager() const;

	/** True if Asset is the running session's asset instance, or a sub-graph instance nested under it. */
	bool IsAssetInCurrentSession(const UFlowAsset* Asset) const;

	void BeginClose(UFlowNode_EndDialogue* EndNode);
	void FinishClose();

	UFUNCTION()
	void OnUiCloseFinished();

	void ArmCloseWatchdog();
	void DisarmCloseWatchdog();

	void BindCloseDelegate();
	void UnbindCloseDelegate();

	/** Force-finish every presentation node of the outgoing session. No output pins fire. */
	void KillPresentationNodes(FDialogueSession& Session);

	/** Promote the head of the wait queue. Only ever called from Tick, never inline from a graph callback. */
	void DispatchPendingStart();
	void DispatchPendingCancels();

	EDialogueSlotState SlotState = EDialogueSlotState::Idle;

	FDialogueSession CurrentSession;

	/** FIFO. An interrupter is inserted at the front, so it wins over anything already waiting. */
	TArray<TWeakObjectPtr<UFlowNode_BeginDialogue>> WaitingQueue;

	/**
	 * Owners whose Cancelled pin still has to fire. Always drained from Tick: firing a pin on a node in another
	 * Flow asset instance runs that whole branch inline, and whether it does depends on the deferred-transition
	 * scope stack, i.e. on graph topology. Deferring a tick makes the order the same for every graph shape.
	 */
	TArray<TWeakObjectPtr<UFlowNode_BeginDialogue>> PendingCancelDispatch;

	/** The End Dialogue node waiting on the close it started, if any. Null when the subsystem closed the screen itself. */
	TWeakObjectPtr<UFlowNode_EndDialogue> PendingEndDialogueNode;

	bool bStartDispatchPending = false;

	/** Guards against a Cancelled branch re-entering arbitration from inside arbitration. */
	bool bArbitrating = false;

	bool bCloseDelegateBound = false;

	/** Real-time deadline for the close animation to report back; 0 = disarmed. */
	double CloseWatchdogDeadline = 0.0;
};

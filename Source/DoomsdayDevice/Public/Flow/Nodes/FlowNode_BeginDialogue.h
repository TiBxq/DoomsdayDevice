// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"

#include "Dialogue/DialogueSubsystem.h"

#include "FlowNode_BeginDialogue.generated.h"

/**
 * Claims the dialogue screen for everything downstream of it, so two dialogues can never overwrite each other.
 * Place one at the head of each dialogue; end that dialogue with an End Dialogue node.
 *
 * The node stays active for the whole dialogue - it is the session handle - and finishes when End Dialogue has
 * closed the screen. Whichever way the dialogue ends, the screen is released: even an aborted or torn-down graph
 * runs Cleanup, which frees the slot.
 *
 * Wire the Cancelled pin. It fires when the dialogue is dropped or interrupted, and it is the only chance to run
 * cleanup for a dialogue that never played. In a sub-graph it must reach the Finish node, or the parent's
 * Sub Graph node stays active forever.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Begin Dialogue"))
class DOOMSDAYDEVICE_API UFlowNode_BeginDialogue : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_BeginDialogue();

	// ---- called by UDialogueSubsystem ----

	/** The screen is ours: run the dialogue. */
	void NotifySessionStarted();

	/** Another dialogue took the screen, or this one was dropped. */
	void NotifySessionCancelled();

	/** End Dialogue finished closing the screen; the node's work is done. */
	void NotifySessionEnded();

protected:
	/** What to do when another dialogue already owns the screen. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	EDialogueBusyPolicy BusyPolicy = EDialogueBusyPolicy::Wait;

	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

	/** External abort: die without triggering any output pin. */
	virtual void ForceFinishNode() override;

	virtual void OnSave_Implementation() override;
	virtual void OnLoad_Implementation() override;

private:
	UDialogueSubsystem* GetDialogueSubsystem() const;

	/** Reacts to a TryAcquire result by triggering the matching pin, or by waiting for a callback. */
	void HandleAcquireResult(EDialogueAcquireResult Result);

	/** True between acquiring/queueing and releasing. Guards against a re-trigger re-entering the queue. */
	bool bRequestActive = false;

	/** Whether this node held the screen or was still waiting, so a load can restore its intent. */
	UPROPERTY(SaveGame)
	EDialogueSavedState SavedState = EDialogueSavedState::NotStarted;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual void UpdateNodeConfigText_Implementation() override;
#endif
};

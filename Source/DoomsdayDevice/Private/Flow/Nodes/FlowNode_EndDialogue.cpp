// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_EndDialogue.h"

#include "Dialogue/DialogueSubsystem.h"
#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"
#include "DoomsdayDevicePlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

UFlowNode_EndDialogue::UFlowNode_EndDialogue()
{
#if WITH_EDITOR
	Category = TEXT("Dialogue");
#endif
}

void UFlowNode_EndDialogue::ExecuteInput(const FName& PinName)
{
	// Closing is arbitrated rather than done here: the subsystem knows whether this node still owns the screen.
	// An End Dialogue reached from a cancelled dialogue's Cancelled branch must not close the screen that the
	// interrupting dialogue has already taken - it just passes straight through to its output.
	const UWorld* World = GetWorld();
	if (UDialogueSubsystem* Dialogue = World ? World->GetSubsystem<UDialogueSubsystem>() : nullptr)
	{
		Dialogue->RequestEndDialogue(this);
		return;
	}

	// No arbitration available: fall back to closing the screen directly.
	if (const ADoomsdayDevicePlayerController* PC = World ? Cast<ADoomsdayDevicePlayerController>(World->GetFirstPlayerController()) : nullptr)
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			UIManager->StopDialogueVoice();
			UIManager->ForceCloseDialogueWidget();
		}
	}

	TriggerFirstOutput(true);
}

void UFlowNode_EndDialogue::Cleanup()
{
	Super::Cleanup();
}

void UFlowNode_EndDialogue::OnDialogueCloseCompleted()
{
	TriggerFirstOutput(true);
}


#if WITH_EDITOR 
FString UFlowNode_EndDialogue::GetNodeDescription() const
{
	return Super::GetNodeDescription();
}

EDataValidationResult UFlowNode_EndDialogue::ValidateNode()
{
	return EDataValidationResult::Valid;
}
#endif
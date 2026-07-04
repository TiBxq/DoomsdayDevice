// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_EndDialogue.h"

#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"
#include "DoomsdayDevicePlayerController.h"

UFlowNode_EndDialogue::UFlowNode_EndDialogue()
{
#if WITH_EDITOR
	Category = TEXT("Dialogue");
#endif
}

void UFlowNode_EndDialogue::ExecuteInput(const FName& PinName)
{
	if (ADoomsdayDevicePlayerController* PC = Cast<ADoomsdayDevicePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (UBasicUIManager* UIManager = PC->GetLocalPlayer()->GetSubsystem<UBasicUIManager>())
		{
			UIManager->CloseWidget(GetDefault<UPlayerSettings>()->DialogueWidget);
			TriggerFirstOutput(true);
		}
	}
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
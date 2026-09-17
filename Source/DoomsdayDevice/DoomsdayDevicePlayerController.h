// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"

#include "Gameplay/InteractionPrompt.h"

#include "DoomsdayDevicePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UInteractionComponent;
class UInputAction;
class ADoomsdayDeviceCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FContinueDialogueEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSelectDialogueChoiceEvent, int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionUseDeniedEvent, FGameplayTag, RequiredToolTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueHintEvent);

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class DOOMSDAYDEVICE_API ADoomsdayDevicePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	ADoomsdayDevicePlayerController();

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FContinueDialogueEvent ContinueDialogueEvent;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FSelectDialogueChoiceEvent SelectDialogueChoiceEvent;

	/** Fired when the player presses the hint input while a hint is on screen (bound by FlowNode_DialogueHintTriggered). */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FDialogueHintEvent DialogueHintEvent;

	/** Fired when Use is pressed on an interaction whose required tool is not in hand (for UI feedback). */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionUseDeniedEvent OnInteractionUseDenied;

	/** Prompt for the interaction currently targeted; default-constructed when nothing is targeted. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FInteractionPrompt GetCurrentInteractionPrompt() const { return CurrentPrompt; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasActiveInteraction() const { return ActiveInteraction.IsValid(); }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractionAction;

	/** Drops the carried heavy item */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DropAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ContinueDialogueAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SelectFirstChoiceAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SelectSecondChoiceAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SelectThirdChoiceAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SelectFourthChoiceAction;

	/** Tool slot hotkeys; array index = slot index in UPlayerSettings::ToolSlots */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<UInputAction*> ToolSlotActions;

	/** Mouse wheel tool cycling (Axis1D, IA_ToolCycle): wheel up steps to the previous slot, wheel down to the next */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ToolCycleAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DialogueHintAction;

	/** Fact whose value gates the hint widget/input: 0 = none, N = hint N. Set to Flow.Facts.HintAvailable. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FGameplayTag HintFactTag;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Unsubscribe from the game-instance-scoped facts delegate */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	virtual void PlayerTick(float DeltaTime) override;

private:
	// ------------ Interactions ---------------
	TArray<TWeakObjectPtr<UInteractionComponent>> PossibleInteractions;
	TWeakObjectPtr<UInteractionComponent> ActiveInteraction;

	void OnInteractionEnter(const TWeakObjectPtr<UInteractionComponent> Interaction);
	void OnInteractionExit(const TWeakObjectPtr<UInteractionComponent> Interaction);

	void ActivateInteraction(const TWeakObjectPtr<UInteractionComponent> Interaction);
	void DeactivateInteraction();

	/** Re-evaluates the active interaction's prompt and pushes it to the UI only when it changed. */
	void RefreshInteractionPrompt();

	FInteractionPrompt CurrentPrompt;
	TWeakObjectPtr<UInteractionComponent> PromptSource;
	bool bPromptPushed = false;

	void OnInteractionUsed();
	void OnDropUsed();

	// -------------- Dialogues ------------------
	UFUNCTION()
	void OnDialogueContinued();

	UFUNCTION()
	void OnDialogueChoiceSelected(const FInputActionValue& Value, int32 Index);

	UFUNCTION()
	void OnDialogueHintUsed();

	/** Reacts to the "hint available" fact changing: shows/hides the hint widget. */
	void OnHintFactChanged(const FGameplayTag& ChangedTag, int32 NewValue);

	// -------------- Tools ------------------
	UFUNCTION()
	void OnToolSlotPressed(const FInputActionValue& Value, int32 SlotIndex);

	UFUNCTION()
	void OnToolCycled(const FInputActionValue& Value);

	/** Shared gate for every tool-switch input: true while dialogue choices are on screen */
	bool IsToolSwitchBlocked() const;

	/** The slot the player last asked for: a switch still being held, else what is in hands */
	int32 GetRequestedToolSlot(const ADoomsdayDeviceCharacter& PlayerCharacter) const;

	/**
	 * Switches to TargetSlot now or, within UPlayerSettings::ToolSwitchDelaySeconds of the previous switch, holds it
	 * until that time is up. A later request replaces the held one.
	 */
	void RequestToolSlot(int32 TargetSlot);

	void ApplyPendingToolSwitch();
	void OnToolSwitchDelayElapsed();

	/** Runs for the switch delay after each switch made from input */
	FTimerHandle ToolSwitchDelayTimer;

	/** Held target (INDEX_NONE = empty hands); only meaningful while bToolSwitchPending */
	int32 PendingToolSlot = INDEX_NONE;
	bool bToolSwitchPending = false;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DoomsdayDevicePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UInteractionComponent;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FContinueDialogueEvent);

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

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ContinueDialogueAction;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

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

	void OnInteractionUsed();

	// -------------- Dialogues ------------------
	UFUNCTION()
	void OnDialogueContinued();
};

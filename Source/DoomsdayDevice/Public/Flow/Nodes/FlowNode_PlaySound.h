// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "GameplayTagContainer.h"

#include "FlowNode_PlaySound.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * Plays a sound from a Flow graph.
 * - Play  : starts the sound and immediately fires Played (the node stays alive so Stop still works).
 * - Stop  : stops the sound and fires Stopped.
 * When bLooping is set, the sound restarts on finish and plays until Stop.
 * PlayAtActor is optional: empty plays a non-spatial 2D sound, a tag plays attached to the matching actor.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Play Sound"))
class DOOMSDAYDEVICE_API UFlowNode_PlaySound : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_PlaySound();

protected:
	/** Sound to play. */
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> Sound;

	/** If true, the sound restarts when it finishes and keeps playing until Stop. */
	UPROPERTY(EditAnywhere, Category = "Sound")
	bool bLooping = false;

	/** Optional: identity tag of an actor to play the sound attached to (positional). Empty plays 2D. */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (Categories = "Flow"))
	FGameplayTag PlayAtActor;

	/** Linear volume scalar applied to the sound. */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	/** Linear pitch scalar applied to the sound. */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.0"))
	float PitchMultiplier = 1.0f;

	/** Spawned playback handle, kept referenced so it isn't GC'd while playing. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AudioComponent;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void Cleanup() override;

	UFUNCTION()
	void OnAudioFinished();

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual EDataValidationResult ValidateNode() override;
#endif
};

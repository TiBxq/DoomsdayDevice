// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/Nodes/FlowNode_PlaySound.h"

#include "FlowComponent.h"
#include "FlowSubsystem.h"
#include "FlowTags.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_PlaySound)

UFlowNode_PlaySound::UFlowNode_PlaySound()
{
#if WITH_EDITOR
	Category = TEXT("Sound");
	NodeDisplayStyle = FlowNodeStyle::Latent;
#endif

	InputPins = { FFlowPin(TEXT("Play")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("Played")), FFlowPin(TEXT("Stopped")) };
}

void UFlowNode_PlaySound::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Play"))
	{
		USoundBase* LoadedSound = Sound.LoadSynchronous();
		UWorld* World = GetWorld();
		if (!LoadedSound || !World)
		{
			LogError(TEXT("Sound is missing or world is unavailable"));
			TriggerOutput(TEXT("Played"), true); // don't stall the graph
			return;
		}

		// Restart cleanly if Play is re-triggered while already playing.
		if (AudioComponent)
		{
			AudioComponent->OnAudioFinished.RemoveAll(this);
			AudioComponent->Stop();
			AudioComponent = nullptr;
		}

		// Optional positional playback: resolve the actor addressed by PlayAtActor via its UFlowComponent.
		AActor* TargetActor = nullptr;
		if (PlayAtActor.IsValid())
		{
			if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
			{
				for (const TWeakObjectPtr<UFlowComponent>& FoundComponent : FlowSubsystem->GetComponents<UFlowComponent>(FGameplayTagContainer(PlayAtActor), EGameplayContainerMatchType::Any))
				{
					if (FoundComponent.IsValid())
					{
						TargetActor = FoundComponent->GetOwner();
						break;
					}
				}
			}

			if (!TargetActor)
			{
				LogError(FString::Printf(TEXT("No actor found for tag %s; playing 2D instead"), *PlayAtActor.ToString()));
			}
		}

		if (TargetActor)
		{
			AudioComponent = UGameplayStatics::SpawnSoundAttached(LoadedSound, TargetActor->GetRootComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, /*bStopWhenAttachedToDestroyed*/ true, VolumeMultiplier, PitchMultiplier, /*StartTime*/ 0.0f, /*AttenuationSettings*/ nullptr, /*ConcurrencySettings*/ nullptr, /*bAutoDestroy*/ false);
		}
		else
		{
			AudioComponent = UGameplayStatics::SpawnSound2D(World, LoadedSound, VolumeMultiplier, PitchMultiplier, /*StartTime*/ 0.0f, /*ConcurrencySettings*/ nullptr, /*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ false);
		}

		if (AudioComponent)
		{
			AudioComponent->OnAudioFinished.AddDynamic(this, &UFlowNode_PlaySound::OnAudioFinished);
		}

		TriggerOutput(TEXT("Played")); // acknowledge start; keep the node alive so Stop still works
	}
	else if (PinName == TEXT("Stop"))
	{
		if (AudioComponent)
		{
			AudioComponent->OnAudioFinished.RemoveAll(this);
			AudioComponent->Stop();
			AudioComponent = nullptr;
		}

		TriggerOutput(TEXT("Stopped"), true);
	}
}

void UFlowNode_PlaySound::OnAudioFinished()
{
	if (bLooping && AudioComponent)
	{
		// Re-play to loop non-looping assets (seamless loops should still be authored looping in the asset).
		AudioComponent->Play();
		return;
	}

	// Non-looping natural end: Played already fired and there is no completion pin, so tear the node down.
	if (AudioComponent)
	{
		AudioComponent->OnAudioFinished.RemoveAll(this);
		AudioComponent = nullptr;
	}

	Finish();
}

void UFlowNode_PlaySound::Cleanup()
{
	if (AudioComponent)
	{
		AudioComponent->OnAudioFinished.RemoveAll(this);
		AudioComponent->Stop();
		AudioComponent = nullptr;
	}

	Super::Cleanup();
}

#if WITH_EDITOR
FString UFlowNode_PlaySound::GetNodeDescription() const
{
	return Sound.IsNull() ? TEXT("Missing Sound!") : Sound.GetAssetName();
}

EDataValidationResult UFlowNode_PlaySound::ValidateNode()
{
	if (Sound.IsNull())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Sound is missing or invalid"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

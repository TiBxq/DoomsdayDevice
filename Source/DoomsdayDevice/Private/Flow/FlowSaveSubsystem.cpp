// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Flow/FlowSaveSubsystem.h"

#include "Flow/FactsDBSubsystem.h"
#include "FlowWorldSettings.h"
#include "Gameplay/InventorySubsystem.h"
#include "Player/Saveable.h"
#include "Player/SaveableComponent.h"

#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowSaveSubsystem)

FString UFlowSaveSubsystem::CheckpointSlotName = TEXT("Checkpoint");

UFlowSaveSubsystem::UFlowSaveSubsystem()
{
}

bool UFlowSaveSubsystem::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return HasAnyFlags(RF_ClassDefaultObject) ? false : ProcessConsoleExec(Cmd, Ar, nullptr);
}

bool UFlowSaveSubsystem::HasSaveGame()
{
	return UGameplayStatics::DoesSaveGameExist(CheckpointSlotName, 0);
}

void UFlowSaveSubsystem::SaveGame()
{
	UDoomsdaySaveGame* NewSaveGame = Cast<UDoomsdaySaveGame>(UGameplayStatics::CreateSaveGameObject(UDoomsdaySaveGame::StaticClass()));
	if (!NewSaveGame)
	{
		return;
	}

	NewSaveGame->LevelName = GetWorld()->GetFName();
	NewSaveGame->DestroyedActors = DestroyedActors;

	// Flow graph/component state (fills FlowComponents / FlowInstances).
	OnGameSaved(NewSaveGame);

	// Game-specific state that lives outside Flow.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UInventorySubsystem* Inventory = GameInstance->GetSubsystem<UInventorySubsystem>())
		{
			NewSaveGame->SavedItems = Inventory->GetAllItems();
		}
		if (const UFactsDBSubsystem* Facts = GameInstance->GetSubsystem<UFactsDBSubsystem>())
		{
			NewSaveGame->SavedFacts = Facts->GetAllFacts();
		}
	}

	// On-level actors data
	for (const TWeakObjectPtr<USaveableComponent>& WeakComp : Registered)
	{
		if (USaveableComponent* Comp = WeakComp.Get())
		{
			if (AActor* Actor = Comp->GetOwner())
			{
				if (Actor->Implements<USaveable>())
				{
					ISaveable::Execute_OnPreSave(Actor);
				}

				FActorSaveData Data;
				Data.SaveId = Comp->SaveId;
				SerializeActor(Actor, Data);

				if (Comp->bRuntimeSpawned)
				{
					Data.SpawnClass = Actor->GetClass();
				}
				NewSaveGame->SavedActors.Add(MoveTemp(Data));
			}
		}
	}

	UGameplayStatics::SaveGameToSlot(NewSaveGame, CheckpointSlotName, 0);
}

void UFlowSaveSubsystem::LoadGame()
{
	UDoomsdaySaveGame* LoadedSave = Cast<UDoomsdaySaveGame>(UGameplayStatics::LoadGameFromSlot(CheckpointSlotName, 0));
	if (LoadedSave)
	{
		AbortActiveFlows();
		OnGameLoaded(LoadedSave);

		const AFlowWorldSettings* WorldSettings = Cast<AFlowWorldSettings>(GetWorld()->GetWorldSettings());
		if (WorldSettings && WorldSettings->GetFlowComponent()->LoadInstance(this))
		{
			WorldSettings->GetFlowComponent()->LoadRootFlow();
		}

		RestoreGameState(LoadedSave);

		// TODO: Restore actors data
	}
}

void UFlowSaveSubsystem::PreloadSaveGame()
{
	UDoomsdaySaveGame* LoadedSave = Cast<UDoomsdaySaveGame>(UGameplayStatics::LoadGameFromSlot(CheckpointSlotName, 0));
	if (LoadedSave)
	{
		// Stage the Flow data so the next level's Flow components load themselves on BeginPlay.
		OnGameLoaded(LoadedSave);

		// Item/fact subsystems survive travel, so restore them now.
		RestoreGameState(LoadedSave);

		// Load on-level actors data to to restore them when map is loaded
		PendingActorData.Reset();
		for (FActorSaveData& Data : LoadedSave->SavedActors)
		{
			PendingActorData.Add(Data.SaveId, MoveTemp(Data));
		}
		DestroyedActors = LoadedSave->DestroyedActors;
		bLoadInProgress = true;

		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UFlowSaveSubsystem::OnPostLoadMap);
	}
}

void UFlowSaveSubsystem::DeleteSaveGame()
{
	UGameplayStatics::DeleteGameInSlot(CheckpointSlotName, 0);
	ClearLoadedSaveGame();

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UInventorySubsystem* Inventory = GameInstance->GetSubsystem<UInventorySubsystem>())
		{
			Inventory->ResetInventory();
		}
		if (UFactsDBSubsystem* Facts = GameInstance->GetSubsystem<UFactsDBSubsystem>())
		{
			Facts->ResetFacts();
		}
	}
}

void UFlowSaveSubsystem::RegisterSaveable(USaveableComponent* Comp)
{
	Registered.Add(Comp);
}

void UFlowSaveSubsystem::UnregisterSaveable(USaveableComponent* Comp, bool bDestroyed)
{
	Registered.RemoveSwap(Comp);
	if (bDestroyed && !Comp->bRuntimeSpawned)
	{
		DestroyedActors.Add(Comp->SaveId);
	}
}

void UFlowSaveSubsystem::RestoreActorIfPending(USaveableComponent* Comp)
{
	if (DestroyedActors.Contains(Comp->SaveId))
	{
		Comp->GetOwner()->Destroy();
		return;
	}

	if (const FActorSaveData* Data = PendingActorData.Find(Comp->SaveId))
	{
		AActor* Actor = Comp->GetOwner();
		Actor->SetActorTransform(Data->Transform);
		DeserializeActor(Actor, *Data);

		if (Actor->Implements<USaveable>())
		{
			ISaveable::Execute_OnPostLoadSaved(Actor);
		}
	}
}

void UFlowSaveSubsystem::RestoreGameState(const UDoomsdaySaveGame* SaveGame)
{
	if (!SaveGame)
	{
		return;
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UInventorySubsystem* Inventory = GameInstance->GetSubsystem<UInventorySubsystem>())
		{
			Inventory->SetItems(SaveGame->SavedItems);
		}
		if (UFactsDBSubsystem* Facts = GameInstance->GetSubsystem<UFactsDBSubsystem>())
		{
			Facts->SetFacts(SaveGame->SavedFacts);
		}
	}
}

void UFlowSaveSubsystem::SerializeActor(AActor* Actor, FActorSaveData& OutData)
{
	OutData.Transform = Actor->GetActorTransform();
	FMemoryWriter MemWriter(OutData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
	Ar.ArIsSaveGame = true;
	Actor->Serialize(Ar);
}

void UFlowSaveSubsystem::DeserializeActor(AActor* Actor, const FActorSaveData& Data)
{
	FMemoryReader MemReader(Data.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
	Ar.ArIsSaveGame = true;
	Actor->Serialize(Ar);
}

void UFlowSaveSubsystem::OnPostLoadMap(UWorld* World)
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	// Level-placed will be restored via RestoreActorIfPending.
	// here we are respawning runtime actors
	TArray<FGuid> ToSpawn;
	for (const auto& Pair : PendingActorData)
	{
		if (!Pair.Value.SpawnClass.IsNull())
		{
			ToSpawn.Add(Pair.Key);
		}
	}

	for (const FGuid& Id : ToSpawn)
	{
		const FActorSaveData& Data = PendingActorData[Id];

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* Spawned = World->SpawnActor<AActor>(Data.SpawnClass.LoadSynchronous(), Data.Transform, Params);

		if (USaveableComponent* Comp = Spawned ? Spawned->FindComponentByClass<USaveableComponent>() : nullptr)
		{
			Comp->SaveId = Id;
			Comp->bRuntimeSpawned = true;
			RestoreActorIfPending(Comp);
		}
	}

	bLoadInProgress = false;
}
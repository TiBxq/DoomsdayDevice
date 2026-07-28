// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Flow/FlowSaveSubsystem.h"

#include "Flow/DoomsdaySaveGame.h"
#include "Flow/FactsDBSubsystem.h"
#include "Gameplay/InventorySubsystem.h"

#include "Engine/GameInstance.h"
#include "FlowWorldSettings.h"
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

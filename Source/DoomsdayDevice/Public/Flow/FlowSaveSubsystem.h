// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "FlowSubsystem.h"
#include "FlowSaveSubsystem.generated.h"

class UDoomsdaySaveGame;

/**
 * Save/load for the single "Checkpoint" slot. Wraps the Flow plugin's serialization
 * (OnGameSaved / OnGameLoaded) and additionally persists the game-specific item and fact
 * subsystems into a UDoomsdaySaveGame. Being a UGameInstanceSubsystem (via UFlowSubsystem),
 * it survives level travel, so the Continue path can preload here in the menu world.
 */
UCLASS()
class UFlowSaveSubsystem final : public UFlowSubsystem, public FSelfRegisteringExec
{
	GENERATED_BODY()

	UFlowSaveSubsystem();

public:
	static FString CheckpointSlotName;

	// FSelfRegisteringExec
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/** True if the checkpoint slot exists on disk (gates the menu's Continue option). */
	static bool HasSaveGame();

	/** Serialize Flow + item/fact state into the checkpoint slot. Called by CustomCheckpoint. */
	void SaveGame();

	/** In-place reload of the checkpoint into the current world (console/debug path). */
	UFUNCTION(Exec, Category = "SaveSubsystem")
	void LoadGame();

	/**
	 * Load the checkpoint into the (persistent) subsystems without touching the current world.
	 * Item/fact state is restored immediately; Flow state is staged so the next level's
	 * Flow components restore themselves on BeginPlay. Use before travelling from the menu.
	 */
	void PreloadSaveGame();

	/** Delete the checkpoint slot and clear item/fact state (used by New Game). */
	void DeleteSaveGame();

private:
	/** Push a loaded save game's item/fact tables into the live subsystems. */
	void RestoreGameState(const UDoomsdaySaveGame* SaveGame);
};

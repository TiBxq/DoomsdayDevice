// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Engine/DeveloperSettings.h"
#include "Gameplay/ToolSlotDefinition.h"
#include "PlayerSettings.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Player"))
class UPlayerSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UUserWidget> InteractionWidget;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UUserWidget> DialogueWidget;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UUserWidget> ToolSlotsWidget;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UUserWidget> HUDWidget;

	/** Static tool slots; index = hotkey number - 1. Slots unlock when their ToolTag is collected. */
	UPROPERTY(Config, EditAnywhere, Category = "Tools")
	TArray<FToolSlotDefinition> ToolSlots;

	// Contains debug inputs, inactive in Shipping builds
	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftObjectPtr<UInputMappingContext> DebugContext;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftObjectPtr<UInputMappingContext> ExplorationContext;
};

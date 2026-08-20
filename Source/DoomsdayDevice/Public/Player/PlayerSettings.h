// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors
#pragma once

#include "Engine/DeveloperSettings.h"
#include "Gameplay/ToolSlotDefinition.h"
#include "PlayerSettings.generated.h"

class UInputMappingContext;
class UInteractionPromptData;
class UUserWidget;
class UWorld;

/**
 * Project-wide player and UI configuration, surfaced as Project Settings > Player.
 *
 * Config storage only - no loading, no caching, no mutable state. In a packaged build this class's CDO
 * is constructed during module init and lands in the disregard-for-GC pool, whose references the
 * collector never traces, so a UObject pointer parked here is invisible to GC and is a fatal
 * VerifyGCAssumptions error at runtime. Keep asset references soft and let the consumer load them and
 * own the result.
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

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UUserWidget> DialogueHintWidget;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UUserWidget> MainMenuWidget;

	/** Level entered by the Main Menu's New Game / Continue actions. */
	UPROPERTY(Config, EditAnywhere, Category = "Menu")
	TSoftObjectPtr<UWorld> GameplayLevel;

	/** Fade applied when a dialogue voice-over is cut short by the continue press. 0 = hard stop. */
	UPROPERTY(Config, EditAnywhere, Category = "Dialogue", meta = (ClampMin = "0.0"))
	float DialogueVoiceStopFadeSeconds = 0.08f;

	/** Master switch for dialogue auto-skip. When false, lines with Auto Skip wait for the player as usual. */
	UPROPERTY(Config, EditAnywhere, Category = "Dialogue")
	bool bDialogueAutoSkipEnabled = true;

	/**
	 * How long to wait for the dialogue widget to report that its close animation finished before forcing it
	 * shut. Closing runs through a Blueprint animation, so a widget that never calls FinishDialogueClose would
	 * otherwise block every later dialogue. 0 disables the safety net.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dialogue", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float DialogueCloseTimeoutSeconds = 2.f;

	/** Prompt used by interactions that leave their own Prompt unset. Point it at DA_Prompt_Use. Loaded and held by UInteractionComponent. */
	UPROPERTY(Config, EditAnywhere, Category = "Interaction")
	TSoftObjectPtr<UInteractionPromptData> DefaultPrompt;

	/** Blocked prompt naming the missing tool. {Tool} is the matching ToolSlots DisplayName. */
	UPROPERTY(Config, EditAnywhere, Category = "Interaction")
	FText ToolRequiredTextFormat;

	/** Blocked prompt used when the interaction's RequiredToolTag matches no tool slot. */
	UPROPERTY(Config, EditAnywhere, Category = "Interaction")
	FText ToolRequiredText;

	/** Static tool slots; index = hotkey number - 1. Slots unlock when their ToolTag is collected. */
	UPROPERTY(Config, EditAnywhere, Category = "Tools")
	TArray<FToolSlotDefinition> ToolSlots;

	// Contains debug inputs, inactive in Shipping builds
	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftObjectPtr<UInputMappingContext> DebugContext;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftObjectPtr<UInputMappingContext> ExplorationContext;
};

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

	/** Prompt used by interactions that leave their own Prompt unset. Point it at DA_Prompt_Use. */
	UPROPERTY(Config, EditAnywhere, Category = "Interaction")
	TSoftObjectPtr<UInteractionPromptData> DefaultPrompt;

	/** Blocked prompt naming the missing tool. {Tool} is the matching ToolSlots DisplayName. */
	UPROPERTY(Config, EditAnywhere, Category = "Interaction")
	FText ToolRequiredTextFormat;

	/** Blocked prompt used when the interaction's RequiredToolTag matches no tool slot. */
	UPROPERTY(Config, EditAnywhere, Category = "Interaction")
	FText ToolRequiredText;

	/**
	 * Loads DefaultPrompt on first use. The cached hard reference is what keeps the asset alive -
	 * a soft pointer alone would let it be collected and re-loaded on a later frame, and this is
	 * queried every tick while an interaction is targeted.
	 */
	const UInteractionPromptData* GetDefaultPrompt() const;

	/** Static tool slots; index = hotkey number - 1. Slots unlock when their ToolTag is collected. */
	UPROPERTY(Config, EditAnywhere, Category = "Tools")
	TArray<FToolSlotDefinition> ToolSlots;

	// Contains debug inputs, inactive in Shipping builds
	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftObjectPtr<UInputMappingContext> DebugContext;

	UPROPERTY(Config, EditAnywhere, Category = "Widgets")
	TSoftObjectPtr<UInputMappingContext> ExplorationContext;

private:
	/** Resolved DefaultPrompt. Mutable because GetDefault<UPlayerSettings>() hands out a const pointer. */
	UPROPERTY(Transient)
	mutable TObjectPtr<UInteractionPromptData> CachedDefaultPrompt;

	/** Keeps the "DefaultPrompt is not configured" warning to one line per session. */
	mutable bool bWarnedMissingDefaultPrompt = false;
};

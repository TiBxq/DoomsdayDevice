// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Player/PlayerSettings.h"

#include "DoomsdayDevice.h"
#include "Gameplay/InteractionPromptData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerSettings)

#define LOCTEXT_NAMESPACE "PlayerSettings"

UPlayerSettings::UPlayerSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// assigned in the body rather than the init list so this doesn't couple to declaration order.
	// LOCTEXT is safe in a CDO constructor - it builds a text history from namespace/key and touches
	// no assets, unlike FText::FromStringTable.
	ToolRequiredTextFormat = LOCTEXT("ToolRequiredTextFormat", "{Tool} required");
	ToolRequiredText = LOCTEXT("ToolRequiredText", "Tool is required");
}

const UInteractionPromptData* UPlayerSettings::GetDefaultPrompt() const
{
	if (CachedDefaultPrompt)
	{
		return CachedDefaultPrompt;
	}

	if (DefaultPrompt.IsNull())
	{
		if (!bWarnedMissingDefaultPrompt)
		{
			bWarnedMissingDefaultPrompt = true;
			UE_LOG(LogDoomsdayDevice, Warning, TEXT("Project Settings > Player > Interaction > Default Prompt is not set; interactions without their own Prompt asset fall back to the built-in text."));
		}

		return nullptr;
	}

	CachedDefaultPrompt = DefaultPrompt.LoadSynchronous();
	return CachedDefaultPrompt;
}

#undef LOCTEXT_NAMESPACE

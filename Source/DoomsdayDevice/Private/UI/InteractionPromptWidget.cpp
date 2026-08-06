#include "UI/InteractionPromptWidget.h"

#include "DoomsdayDevice.h"
#include "DoomsdayDevicePlayerController.h"

#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionPromptWidget)

void UInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Label)
	{
		UE_LOG(LogDoomsdayDevice, Warning, TEXT("%s has no TextBlock named 'Label'; the interaction prompt text will not update."), *GetClass()->GetName());
	}

	// recreated on every activation, so pull the state the controller has already computed.
	// under the legacy ABasicPlayerController this cast fails and the authored text stands.
	if (const ADoomsdayDevicePlayerController* PlayerController = Cast<ADoomsdayDevicePlayerController>(GetOwningPlayer()))
	{
		if (PlayerController->HasActiveInteraction())
		{
			SetPrompt(PlayerController->GetCurrentInteractionPrompt());
		}
	}
}

void UInteractionPromptWidget::SetPrompt(const FInteractionPrompt& NewPrompt)
{
	if (bHasPrompt && NewPrompt.IsEquivalentTo(Prompt))
	{
		return;
	}

	Prompt = NewPrompt;
	bHasPrompt = true;

	// a misconfigured empty prompt leaves the authored string-table text rather than blanking the widget
	if (Label && !Prompt.PromptText.IsEmpty())
	{
		Label->SetText(Prompt.PromptText);
	}

	BP_PromptChanged(Prompt);
}

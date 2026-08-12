#include "UI/InteractionPromptWidget.h"

#include "DoomsdayDevice.h"
#include "DoomsdayDevicePlayerController.h"
#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"

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

	UnbindAllFromAnimationFinished(ShowAnim);

	FWidgetAnimationDynamicEvent AnimDelegate;
	AnimDelegate.BindDynamic(this, &UInteractionPromptWidget::HandleAppearFinished);
	BindToAnimationFinished(ShowAnim, AnimDelegate);
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

void UInteractionPromptWidget::SetPromptVisible(bool bNew)
{
	if (bWantVisible == bNew) 
		return;

	bWantVisible = bNew;

	// already playing, reverse it
	if (IsAnimationPlaying(ShowAnim))
	{
		ReverseAnimation(ShowAnim);
		bPlayingForward = bWantVisible;
		return;
	}

	if (bWantVisible == bFullyShown) 
		return;

	// playing from start
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bPlayingForward = bWantVisible;
	if (bWantVisible)
	{
		PlayAnimationForward(ShowAnim);
	}
	else
	{
		PlayAnimationReverse(ShowAnim);
	}
}

void UInteractionPromptWidget::HandleAppearFinished()
{
	bFullyShown = bPlayingForward;

	// wanted state changed - reverse it
	if (bWantVisible != bFullyShown)
	{
		bPlayingForward = bWantVisible;
		if (bWantVisible)
		{
			QueuePlayAnimationForward(ShowAnim, 1.f, false);
		}
		else
		{
			QueuePlayAnimationReverse(ShowAnim, 1.f, false);
		}
	}
	// animation finished and widget is hidden
	else if (!bFullyShown)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
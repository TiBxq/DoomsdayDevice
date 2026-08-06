// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUDWidget.h"

#include "DoomsdayDevicePlayerController.h"

#include "Components/Image.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HUDWidget)

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Reticle)
	{
		BaseReticleBrush = Reticle->GetBrush();
	}

	// the HUD outlives individual interactions, but pull anyway so a recreated HUD is never stale
	if (const ADoomsdayDevicePlayerController* PlayerController = Cast<ADoomsdayDevicePlayerController>(GetOwningPlayer()))
	{
		if (PlayerController->HasActiveInteraction())
		{
			ReticleState = PlayerController->GetCurrentInteractionPrompt().bCanUse
				? EInteractionReticleState::Available
				: EInteractionReticleState::Blocked;
		}
	}

	ApplyReticleColor();
}

void UHUDWidget::SetReticleState(const EInteractionReticleState NewState)
{
	if (NewState == ReticleState)
	{
		return;
	}

	ReticleState = NewState;
	ApplyReticleColor();

	BP_ReticleStateChanged(ReticleState);
}

FLinearColor UHUDWidget::GetReticleStateColor(const EInteractionReticleState State) const
{
	switch (State)
	{
	case EInteractionReticleState::Available:
		return ReticleAvailableColor;

	case EInteractionReticleState::Blocked:
		return ReticleBlockedColor;

	default:
		return ReticleIdleColor;
	}
}

void UHUDWidget::ApplyReticleColor()
{
	if (!Reticle)
	{
		return;
	}

	const FLinearColor StateColor = GetReticleStateColor(ReticleState);

	// the fill is brush tint * ColorAndOpacity
	Reticle->SetColorAndOpacity(StateColor);

	// ...but a RoundedBox outline is drawn straight from the brush and ignores ColorAndOpacity
	// (SlateCore DrawElementTypes.cpp passes OutlineSettings.Color through verbatim), so tint it
	// from the authored brush while keeping the alpha it was authored with
	if (bTintReticleOutline)
	{
		FSlateBrush Brush = BaseReticleBrush;
		const float AuthoredAlpha = BaseReticleBrush.OutlineSettings.Color.GetSpecifiedColor().A;
		Brush.OutlineSettings.Color = FSlateColor(StateColor.CopyWithNewOpacity(AuthoredAlpha * StateColor.A));
		Reticle->SetBrush(Brush);
	}
}

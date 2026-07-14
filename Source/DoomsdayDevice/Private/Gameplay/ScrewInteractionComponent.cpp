#include "Gameplay/ScrewInteractionComponent.h"
#include "Gameplay/PanelScrew.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ScrewInteractionComponent)

UScrewInteractionComponent::UScrewInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// screws are interactable by default, no Flow graph enabling required
	bEnabled = true;

	// several screws sit close together on one panel, so require the aim
	// trace to hit this screw's actor instead of ejecting the nearest one
	bPrecise = true;

	ArrowColor = FColor::Orange;
}

void UScrewInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OnUsed.AddDynamic(this, &UScrewInteractionComponent::HandleUsed);
}

void UScrewInteractionComponent::HandleUsed()
{
	if (APanelScrew* Screw = Cast<APanelScrew>(GetOwner()))
	{
		Screw->Eject();
	}
}

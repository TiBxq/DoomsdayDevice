#include "Gameplay/CarryableComponent.h"
#include "Gameplay/ItemSlotComponent.h"
#include "DoomsdayDeviceCharacter.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CarryableComponent)

UCarryableComponent::UCarryableComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CarrySpeedMultiplier(0.5f)
{
	// carryables are interactable by default, no Flow graph enabling required
	bEnabled = true;

	ArrowColor = FColor::Yellow;
}

UItemSlotComponent* UCarryableComponent::GetCurrentSlot() const
{
	return CurrentSlot.Get();
}

void UCarryableComponent::SetCurrentSlot(UItemSlotComponent* Slot)
{
	CurrentSlot = Slot;
}

void UCarryableComponent::BeginPlay()
{
	Super::BeginPlay();

	OnUsed.AddDynamic(this, &UCarryableComponent::HandleUsed);
}

void UCarryableComponent::HandleUsed()
{
	ADoomsdayDeviceCharacter* Character = Cast<ADoomsdayDeviceCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Character || Character->IsCarrying())
	{
		return;
	}

	if (UItemSlotComponent* Slot = CurrentSlot.Get())
	{
		Slot->DisconnectItem();
	}

	Character->StartCarry(this);
}

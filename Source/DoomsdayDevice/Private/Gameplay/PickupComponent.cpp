#include "Gameplay/PickupComponent.h"

#include "Gameplay/InventorySubsystem.h"
#include "Gameplay/ToolSlotLibrary.h"
#include "DoomsdayDeviceCharacter.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PickupComponent)

UPickupComponent::UPickupComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Count(1)
	, bDestroyOwnerOnPickup(true)
{
	// pickups are interactable by default, no Flow graph enabling required
	bEnabled = true;

	ArrowColor = FColor::Green;
}

void UPickupComponent::BeginPlay()
{
	Super::BeginPlay();

	OnUsed.AddDynamic(this, &UPickupComponent::HandlePickedUp);
}

void UPickupComponent::HandlePickedUp()
{
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		if (UInventorySubsystem* Inventory = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
		{
			Inventory->CollectItem(ItemTag, Count);
		}
	}

	if (bAutoEquip)
	{
		if (ADoomsdayDeviceCharacter* PlayerCharacter = Cast<ADoomsdayDeviceCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		{
			const int32 SlotIndex = UToolSlotLibrary::FindToolSlotIndexByTag(ItemTag);
			if (SlotIndex != INDEX_NONE)
			{
				if (PlayerCharacter->GetEquippedToolSlot() != SlotIndex)
				{
					PlayerCharacter->ToggleToolSlot(SlotIndex);
				}
			}
		}
	}

	// Disable() broadcasts OnPlayerExit, so the player controller drops this component
	// from its interaction candidates before the owner is destroyed
	Disable();

	if (bDestroyOwnerOnPickup && GetOwner())
	{
		GetOwner()->Destroy();
	}
}

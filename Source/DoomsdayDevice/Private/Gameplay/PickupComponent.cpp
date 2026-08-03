#include "Gameplay/PickupComponent.h"

#include "Gameplay/InventorySubsystem.h"
#include "DoomsdayDeviceCharacter.h"
#include "Player/PlayerSettings.h"

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
			const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
			int32 SlotIndex = -1;
			int32 Index = 0;
			for (const FToolSlotDefinition& Tool : ToolSlots)
			{
				if (Tool.ToolTag == ItemTag)
				{
					SlotIndex = Index;
				}
				Index++;
			}

			if (SlotIndex >= 0)
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

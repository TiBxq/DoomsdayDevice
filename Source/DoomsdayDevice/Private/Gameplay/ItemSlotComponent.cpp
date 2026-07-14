#include "Gameplay/ItemSlotComponent.h"
#include "Gameplay/CarryableComponent.h"
#include "DoomsdayDeviceCharacter.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemSlotComponent)

UItemSlotComponent::UItemSlotComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// slots are interactable by default, no Flow graph enabling required
	bEnabled = true;

	ArrowColor = FColor::Blue;
}

UCarryableComponent* UItemSlotComponent::GetConnectedItem() const
{
	return ConnectedItem.Get();
}

void UItemSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	OnUsed.AddDynamic(this, &UItemSlotComponent::HandleUsed);
}

void UItemSlotComponent::HandleUsed()
{
	if (IsOccupied())
	{
		return;
	}

	const ADoomsdayDeviceCharacter* Character = Cast<ADoomsdayDeviceCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	UCarryableComponent* Item = Character ? Character->GetCarriedItem() : nullptr;
	if (!Item)
	{
		return;
	}

	if (AcceptedItemTag.IsValid() && !Item->ItemTag.MatchesTag(AcceptedItemTag))
	{
		return;
	}

	ConnectItem(Item);
}

void UItemSlotComponent::ConnectItem(UCarryableComponent* Item)
{
	if (!Item || IsOccupied())
	{
		return;
	}

	AActor* ItemActor = Item->GetOwner();
	if (!ItemActor)
	{
		return;
	}

	if (ADoomsdayDeviceCharacter* Character = Cast<ADoomsdayDeviceCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (Character->GetCarriedItem() == Item)
		{
			Character->ReleaseCarriedItem();
		}
	}

	ItemActor->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	ItemActor->SetActorEnableCollision(true);

	ConnectedItem = Item;
	Item->SetCurrentSlot(this);

	// the slotted item becomes the take-back interaction, the slot goes silent
	Disable();
	Item->Enable();

	OnItemConnected.Broadcast(Item);

	UE_LOG(LogTemp, Log, TEXT("ItemSlot %s: connected %s"), *GetOwner()->GetName(), *Item->ItemTag.ToString());
}

void UItemSlotComponent::DisconnectItem()
{
	UCarryableComponent* Item = ConnectedItem.Get();
	if (!Item)
	{
		return;
	}

	ConnectedItem = nullptr;
	Item->SetCurrentSlot(nullptr);

	if (AActor* ItemActor = Item->GetOwner())
	{
		ItemActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	Enable();

	OnItemDisconnected.Broadcast(Item);

	UE_LOG(LogTemp, Log, TEXT("ItemSlot %s: disconnected %s"), *GetOwner()->GetName(), *Item->ItemTag.ToString());
}

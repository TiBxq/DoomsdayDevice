#include "Gameplay/ItemSlot.h"
#include "Gameplay/ItemSlotComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FlowComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemSlot)

AItemSlot::AItemSlot()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CylinderMesh.Object);
	}
	MeshComponent->SetRelativeScale3D(FVector(0.5f));
	RootComponent = MeshComponent;

	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));

	SlotComponent = CreateDefaultSubobject<UItemSlotComponent>(TEXT("Slot"));
	SlotComponent->SetupAttachment(MeshComponent);
	// the slot transform is where the connected item rests - just above the pedestal
	SlotComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
	SlotComponent->Distance = 150.0f;

	if (FGameplayTag::IsValidGameplayTagString(TEXT("Flow.Slots.Test")))
	{
		const FGameplayTag TestSlotTag = FGameplayTag::RequestGameplayTag(TEXT("Flow.Slots.Test"), /*ErrorIfNotFound*/ false);
		if (TestSlotTag.IsValid())
		{
			FlowComponent->IdentityTags.AddTag(TestSlotTag);
		}
	}

	if (FGameplayTag::IsValidGameplayTagString(TEXT("Flow.Items.Test.HeavyCube")))
	{
		const FGameplayTag TestItemTag = FGameplayTag::RequestGameplayTag(TEXT("Flow.Items.Test.HeavyCube"), /*ErrorIfNotFound*/ false);
		if (TestItemTag.IsValid())
		{
			SlotComponent->AcceptedItemTag = TestItemTag;
		}
	}
}

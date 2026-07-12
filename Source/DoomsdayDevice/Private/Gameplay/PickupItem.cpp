#include "Gameplay/PickupItem.h"
#include "Gameplay/PickupComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FlowComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PickupItem)

APickupItem::APickupItem()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
	}
	MeshComponent->SetRelativeScale3D(FVector(0.25f));
	RootComponent = MeshComponent;

	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));

	PickupComponent = CreateDefaultSubobject<UPickupComponent>(TEXT("Pickup"));
	PickupComponent->SetupAttachment(MeshComponent);
	PickupComponent->Distance = 150.0f;

	if (FGameplayTag::IsValidGameplayTagString(TEXT("Flow.Items.Test.Cube")))
	{
		const FGameplayTag TestItemTag = FGameplayTag::RequestGameplayTag(TEXT("Flow.Items.Test.Cube"), /*ErrorIfNotFound*/ false);
		if (TestItemTag.IsValid())
		{
			FlowComponent->IdentityTags.AddTag(TestItemTag);
			PickupComponent->ItemTag = TestItemTag;
		}
	}
}

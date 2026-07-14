#include "Gameplay/CarryableItem.h"
#include "Gameplay/CarryableComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FlowComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CarryableItem)

ACarryableItem::ACarryableItem()
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

	CarryableComponent = CreateDefaultSubobject<UCarryableComponent>(TEXT("Carryable"));
	CarryableComponent->SetupAttachment(MeshComponent);
	CarryableComponent->Distance = 150.0f;

	if (FGameplayTag::IsValidGameplayTagString(TEXT("Flow.Items.Test.HeavyCube")))
	{
		const FGameplayTag TestItemTag = FGameplayTag::RequestGameplayTag(TEXT("Flow.Items.Test.HeavyCube"), /*ErrorIfNotFound*/ false);
		if (TestItemTag.IsValid())
		{
			FlowComponent->IdentityTags.AddTag(TestItemTag);
			CarryableComponent->ItemTag = TestItemTag;
		}
	}
}

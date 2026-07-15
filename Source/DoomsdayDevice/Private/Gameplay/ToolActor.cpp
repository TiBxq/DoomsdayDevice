#include "Gameplay/ToolActor.h"

#include "Components/StaticMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolActor)

AToolActor::AToolActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(Root);

	// same first-person rendering setup as the character's arms mesh
	MeshComponent->SetCollisionProfileName(FName("NoCollision"));
	MeshComponent->SetOnlyOwnerSee(true);
	MeshComponent->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
}

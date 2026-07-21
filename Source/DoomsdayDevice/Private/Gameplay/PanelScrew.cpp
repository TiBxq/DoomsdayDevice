#include "Gameplay/PanelScrew.h"
#include "Gameplay/ScrewInteractionComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "FlowComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PanelScrew)

APanelScrew::APanelScrew()
	: EjectSpeed(250.0f)
	, EjectSpin(5.0f)
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CylinderMesh.Object);
	}
	MeshComponent->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.08f));
	RootComponent = MeshComponent;

	// while in the panel: visible to the precise interaction trace, but invisible to
	// physics and pawn sweeps so the screw can't shove the dangling panel or block movement
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));

	InteractionComponent = CreateDefaultSubobject<UScrewInteractionComponent>(TEXT("Interaction"));
	InteractionComponent->SetupAttachment(MeshComponent);
	InteractionComponent->Distance = 150.0f;

	EjectedProfileName = UCollisionProfile::PhysicsActor_ProfileName;
}

void APanelScrew::Eject()
{
	if (bEjected)
	{
		return;
	}
	bEjected = true;

	// Disable() broadcasts OnPlayerExit, so the player controller drops this component
	// from its interaction candidates before the screw turns into a physics prop
	InteractionComponent->Disable();

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	MeshComponent->SetCollisionProfileName(EjectedProfileName);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->AddImpulse(GetScrewAxis() * EjectSpeed, NAME_None, /*bVelChange*/ true);
	MeshComponent->AddAngularImpulseInRadians(FMath::VRand() * EjectSpin, NAME_None, /*bVelChange*/ true);

	// the panel reacts last, once the screw is fully free
	OnEjected.Broadcast(this);
}

#include "Gameplay/WallPanel.h"
#include "Gameplay/PanelScrew.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FlowComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WallPanel)

AWallPanel::AWallPanel()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
	}
	MeshComponent->SetRelativeScale3D(FVector(0.06f, 0.8f, 0.8f));
	RootComponent = MeshComponent;

	// make the dangle settle instead of swinging forever
	MeshComponent->SetLinearDamping(0.3f);
	MeshComponent->SetAngularDamping(2.0f);

	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));

	ScrewClass = APanelScrew::StaticClass();

	// four corner slots on the front face of the unit cube (the face plane is at X=50)
	ScrewSlots = {
		FTransform(FVector(50.0f, 40.0f, 40.0f)),
		FTransform(FVector(50.0f, -40.0f, 40.0f)),
		FTransform(FVector(50.0f, 40.0f, -40.0f)),
		FTransform(FVector(50.0f, -40.0f, -40.0f)),
	};

	if (FGameplayTag::IsValidGameplayTagString(TEXT("Flow.Interaction.Panel.Test")))
	{
		const FGameplayTag PanelTag = FGameplayTag::RequestGameplayTag(TEXT("Flow.Interaction.Panel.Test"), /*ErrorIfNotFound*/ false);
		if (PanelTag.IsValid())
		{
			FlowComponent->IdentityTags.AddTag(PanelTag);
		}
	}

	if (FGameplayTag::IsValidGameplayTagString(TEXT("Flow.Interaction.Panel.Screw")))
	{
		const FGameplayTag ScrewTag = FGameplayTag::RequestGameplayTag(TEXT("Flow.Interaction.Panel.Screw"), /*ErrorIfNotFound*/ false);
		if (ScrewTag.IsValid())
		{
			ScrewIdentityTags.AddTag(ScrewTag);
		}
	}
}

void AWallPanel::BeginPlay()
{
	Super::BeginPlay();

	if (!ScrewClass || !GetWorld())
	{
		return;
	}

	// the engine cylinder's long axis is +Z, but slots define "out of the wall" as +X
	const FQuat MeshAxisFix(FRotator(-90.0f, 0.0f, 0.0f));
	const FTransform ActorT = GetActorTransform();

	for (const FTransform& Slot : ScrewSlots)
	{
		// transform the location through the scaled actor transform (matches the edit widget),
		// but spawn with unit scale so the panel's non-uniform scale can't squash the screw
		const FVector Location = ActorT.TransformPosition(Slot.GetLocation());
		const FQuat Rotation = ActorT.GetRotation() * Slot.GetRotation() * MeshAxisFix;
		const FTransform SpawnTransform(Rotation, Location, FVector::OneVector);

		APanelScrew* Screw = GetWorld()->SpawnActorDeferred<APanelScrew>(ScrewClass, SpawnTransform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn, ESpawnActorScaleMethod::MultiplyWithRoot);
		if (!Screw)
		{
			continue;
		}

		// stamp identity before FinishSpawning so the Flow component registers with the right tags
		if (ScrewIdentityTags.IsValid())
		{
			Screw->GetFlowComponent()->IdentityTags = ScrewIdentityTags;
		}
		Screw->FinishSpawning(SpawnTransform, false, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

		Screw->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepWorldTransform);
		Screw->OnEjected.AddDynamic(this, &AWallPanel::HandleScrewEjected);
		SpawnedScrews.Add(Screw);
	}
}

void AWallPanel::EjectNextScrew()
{
	if (!SpawnedScrews.IsEmpty())
	{
		SpawnedScrews.Last()->Eject();
	}
}

void AWallPanel::HandleScrewEjected(APanelScrew* Screw)
{
	SpawnedScrews.Remove(Screw);

	if (bDetached)
	{
		return;
	}

	if (SpawnedScrews.Num() == 1)
	{
		BeginDangle(SpawnedScrews[0]);
	}
	else if (SpawnedScrews.Num() == 0)
	{
		BeginFall();
	}
	// with 2+ screws remaining the panel stays fixed
}

void AWallPanel::BeginDangle(APanelScrew* RemainingScrew)
{
	// read the pivot before physics can move anything
	const FVector PivotLocation = RemainingScrew->GetActorLocation();
	const FVector PivotAxis = RemainingScrew->GetScrewAxis();

	// the body must simulate before the constraint binds it
	MeshComponent->SetSimulatePhysics(true);

	// deliberately not attached to the panel: constraint frames are baked from the
	// component's world transform at init, so keep it out of the moving hierarchy
	HangConstraint = NewObject<UPhysicsConstraintComponent>(this, TEXT("HangConstraint"));
	HangConstraint->RegisterComponent();

	// the twist axis is the constraint frame's X: aim it down the screw axis so the
	// panel can only pivot in the wall plane, hanging crooked off the remaining screw
	HangConstraint->SetWorldLocationAndRotation(PivotLocation, FRotationMatrix::MakeFromX(PivotAxis).Rotator());
	HangConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
	HangConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
	HangConstraint->SetLinearZLimit(LCM_Locked, 0.0f);
	HangConstraint->SetAngularSwing1Limit(ACM_Locked, 0.0f);
	HangConstraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
	HangConstraint->SetAngularTwistLimit(ACM_Free, 0.0f);

	// a null second body constrains the panel to the world (the wall)
	HangConstraint->SetConstrainedComponents(MeshComponent, NAME_None, nullptr, NAME_None);

	MeshComponent->WakeAllRigidBodies();
}

void AWallPanel::BeginFall()
{
	bDetached = true;

	if (HangConstraint)
	{
		HangConstraint->BreakConstraint();
		HangConstraint->DestroyComponent();
		HangConstraint = nullptr;
	}

	// a panel with a single slot goes straight from fixed to falling
	if (!MeshComponent->IsSimulatingPhysics())
	{
		MeshComponent->SetSimulatePhysics(true);
	}
	MeshComponent->WakeAllRigidBodies();

	OnPanelDetached.Broadcast();
}

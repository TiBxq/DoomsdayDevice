// Copyright Epic Games, Inc. All Rights Reserved.

#include "DoomsdayDeviceCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DoomsdayDevice.h"

#include "FlowComponent.h"
#include "Gameplay/CarryableComponent.h"
#include "Player/PlayerTags.h"

ADoomsdayDeviceCharacter::ADoomsdayDeviceCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	FlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComponent"));
	FlowComponent->IdentityTags = FGameplayTagContainer(PlayerTags::Player_Pawn);

	// carried heavy items attach to the capsule (not the camera) so they follow yaw but not pitch
	CarryAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CarryAttachPoint"));
	CarryAttachPoint->SetupAttachment(GetCapsuleComponent());
	CarryAttachPoint->SetRelativeLocation(FVector(75.0f, 0.0f, 10.0f));
}

void ADoomsdayDeviceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ADoomsdayDeviceCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADoomsdayDeviceCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADoomsdayDeviceCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADoomsdayDeviceCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ADoomsdayDeviceCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogDoomsdayDevice, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ADoomsdayDeviceCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ADoomsdayDeviceCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ADoomsdayDeviceCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ADoomsdayDeviceCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ADoomsdayDeviceCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ADoomsdayDeviceCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

FGameplayTagContainer ADoomsdayDeviceCharacter::GetIdentityTags() const
{
	return FlowComponent->IdentityTags;
}

UCarryableComponent* ADoomsdayDeviceCharacter::GetCarriedItem() const
{
	return CarriedItem.Get();
}

void ADoomsdayDeviceCharacter::StartCarry(UCarryableComponent* Item)
{
	if (!Item || CarriedItem.IsValid())
	{
		return;
	}

	AActor* ItemActor = Item->GetOwner();
	if (!ItemActor)
	{
		return;
	}

	// Disable() first: removes the item from the controller's interaction candidates
	// (PlayerTick dereferences them unchecked) and keeps it from hogging ActiveInteraction
	// while glued to the player
	Item->Disable();

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(ItemActor->GetRootComponent()))
	{
		if (RootPrimitive->IsSimulatingPhysics())
		{
			RootPrimitive->SetSimulatePhysics(false);
		}
	}

	// no collision while carried, so the item can't push the player or block
	// the precise-interaction visibility traces (they only ignore the pawn)
	ItemActor->SetActorEnableCollision(false);
	ItemActor->AttachToComponent(CarryAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	CachedWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = CachedWalkSpeed * Item->CarrySpeedMultiplier;

	CarriedItem = Item;
}

void ADoomsdayDeviceCharacter::ReleaseCarriedItem()
{
	if (!CarriedItem.IsValid())
	{
		return;
	}

	if (AActor* ItemActor = CarriedItem->GetOwner())
	{
		ItemActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	GetCharacterMovement()->MaxWalkSpeed = CachedWalkSpeed;
	CarriedItem = nullptr;
}

void ADoomsdayDeviceCharacter::DropCarriedItem()
{
	UCarryableComponent* Item = CarriedItem.Get();
	if (!Item)
	{
		return;
	}

	AActor* ItemActor = Item->GetOwner();
	ReleaseCarriedItem();

	if (ItemActor)
	{
		// settle on the ground below - the attach point floats ~1m above the floor
		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(ItemActor);
		QueryParams.AddIgnoredActor(this);
		const FVector Start = ItemActor->GetActorLocation();
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.0f, 0.0f, 500.0f), ECC_Visibility, QueryParams))
		{
			const FBox Bounds = ItemActor->GetComponentsBoundingBox(/*bNonColliding*/ true);
			ItemActor->SetActorLocation(FVector(Start.X, Start.Y, Hit.ImpactPoint.Z + (Start.Z - Bounds.Min.Z)));
		}

		ItemActor->SetActorEnableCollision(true);
	}

	Item->Enable();
}

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

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

#include "FlowComponent.h"
#include "Gameplay/CarryableComponent.h"
#include "Gameplay/InventorySubsystem.h"
#include "Gameplay/ToolActor.h"
#include "Player/BasicUIManager.h"
#include "Player/PlayerSettings.h"
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

void ADoomsdayDeviceCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UInventorySubsystem* Inventory = GameInstance->GetSubsystem<UInventorySubsystem>())
		{
			Inventory->OnItemCollected.AddUObject(this, &ADoomsdayDeviceCharacter::HandleItemCollected);
		}
	}
}

void ADoomsdayDeviceCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UInventorySubsystem* Inventory = GameInstance->GetSubsystem<UInventorySubsystem>())
		{
			Inventory->OnItemCollected.RemoveAll(this);
		}
	}

	for (AToolActor* Tool : ToolActors)
	{
		if (Tool)
		{
			Tool->Destroy();
		}
	}
	ToolActors.Empty();
	EquippedToolSlot = INDEX_NONE;

	Super::EndPlay(EndPlayReason);
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

	// hands are busy while carrying: stow any equipped tool
	UnequipTool();

	// Disable() first: removes the item from the controller's interaction candidates
	// (PlayerTick dereferences them unchecked) and keeps it from hogging ActiveInteraction
	// while glued to the player
	Item->Disable();
	Item->OnStartCarry.Broadcast();

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
	Item->OnDropped.Broadcast();
}

void ADoomsdayDeviceCharacter::ToggleToolSlot(const int32 SlotIndex)
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	if (!ToolSlots.IsValidIndex(SlotIndex) || IsCarrying() || !IsToolSlotUnlocked(SlotIndex))
	{
		return;
	}

	if (EquippedToolSlot == SlotIndex)
	{
		UnequipTool();
		return;
	}

	AToolActor* Tool = GetOrSpawnToolActor(SlotIndex);
	if (!Tool)
	{
		return;
	}

	// hide the previous tool directly - a single equip notification goes out below
	if (ToolActors.IsValidIndex(EquippedToolSlot) && ToolActors[EquippedToolSlot])
	{
		ToolActors[EquippedToolSlot]->SetActorHiddenInGame(true);
	}

	Tool->SetActorHiddenInGame(false);
	EquippedToolSlot = SlotIndex;

	if (UBasicUIManager* UIManager = GetUIManager())
	{
		UIManager->NotifyEquippedToolChanged(EquippedToolSlot);
	}
}

void ADoomsdayDeviceCharacter::UnequipTool()
{
	if (EquippedToolSlot == INDEX_NONE)
	{
		return;
	}

	if (ToolActors.IsValidIndex(EquippedToolSlot) && ToolActors[EquippedToolSlot])
	{
		ToolActors[EquippedToolSlot]->SetActorHiddenInGame(true);
	}
	EquippedToolSlot = INDEX_NONE;

	if (UBasicUIManager* UIManager = GetUIManager())
	{
		UIManager->NotifyEquippedToolChanged(INDEX_NONE);
	}
}

FGameplayTag ADoomsdayDeviceCharacter::GetEquippedToolTag() const
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	return ToolSlots.IsValidIndex(EquippedToolSlot) ? ToolSlots[EquippedToolSlot].ToolTag : FGameplayTag();
}

bool ADoomsdayDeviceCharacter::IsToolSlotUnlocked(const int32 SlotIndex) const
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	if (!ToolSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UInventorySubsystem* Inventory = GameInstance ? GameInstance->GetSubsystem<UInventorySubsystem>() : nullptr;
	return Inventory && Inventory->HasItem(ToolSlots[SlotIndex].ToolTag);
}

void ADoomsdayDeviceCharacter::HandleItemCollected(const FGameplayTag& ItemTag, int32 /*NewCount*/)
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	for (int32 SlotIndex = 0; SlotIndex < ToolSlots.Num(); ++SlotIndex)
	{
		if (ItemTag.MatchesTag(ToolSlots[SlotIndex].ToolTag))
		{
			if (UBasicUIManager* UIManager = GetUIManager())
			{
				UIManager->NotifyToolSlotUnlocked(SlotIndex);
			}
		}
	}
}

AToolActor* ADoomsdayDeviceCharacter::GetOrSpawnToolActor(const int32 SlotIndex)
{
	const TArray<FToolSlotDefinition>& ToolSlots = GetDefault<UPlayerSettings>()->ToolSlots;
	if (ToolActors.Num() < ToolSlots.Num())
	{
		ToolActors.SetNum(ToolSlots.Num());
	}

	if (ToolActors[SlotIndex])
	{
		return ToolActors[SlotIndex];
	}

	UClass* ToolClass = ToolSlots[SlotIndex].ToolActorClass.LoadSynchronous();
	if (!ToolClass)
	{
		UE_LOG(LogDoomsdayDevice, Warning, TEXT("Tool slot %d has no valid ToolActorClass"), SlotIndex);
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this; // OnlyOwnerSee on the tool mesh keys off the owner chain
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AToolActor* Tool = GetWorld()->SpawnActor<AToolActor>(ToolClass, GetActorTransform(), SpawnParams);
	if (Tool)
	{
		Tool->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ToolHandSocketName);
		Tool->SetActorHiddenInGame(true);
		ToolActors[SlotIndex] = Tool;
	}

	return Tool;
}

UBasicUIManager* ADoomsdayDeviceCharacter::GetUIManager() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UBasicUIManager>() : nullptr;
}

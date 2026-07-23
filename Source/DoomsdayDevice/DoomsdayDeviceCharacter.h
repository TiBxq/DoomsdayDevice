// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "DoomsdayDeviceCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UAnimInstance;
struct FInputActionValue;

class UFlowComponent;
class UCarryableComponent;
class AToolActor;
class UBasicUIManager;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ADoomsdayDeviceCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Components")
	TObjectPtr<UFlowComponent> FlowComponent;

	/** Carried heavy items attach here (chest height, in front of the capsule) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CarryAttachPoint;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Socket on the first person arms mesh that equipped tools attach to */
	UPROPERTY(EditAnywhere, Category = "Tools")
	FName ToolHandSocketName = FName("HandGrip_R");

public:
	ADoomsdayDeviceCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintPure, Category = "MossPawn")
	FGameplayTagContainer GetIdentityTags() const;

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	UFUNCTION(BlueprintPure, Category = "Carry")
	bool IsCarrying() const { return CarriedItem.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Carry")
	UCarryableComponent* GetCarriedItem() const;

	/** Attaches the item to the carry point and slows movement. Disables the item's interaction. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void StartCarry(UCarryableComponent* Item);

	/** Stops carrying without dropping: detach + restore speed + clear state. Callers handle collision/interaction. */
	void ReleaseCarriedItem();

	/** Releases the carried item and settles it on the ground in front of the player */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void DropCarriedItem();

	/** Equips the slot's tool, or unequips it if already equipped. Ignored while carrying or if the slot is locked. */
	UFUNCTION(BlueprintCallable, Category = "Tools")
	void ToggleToolSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Tools")
	void UnequipTool();

	/** Plays the equipped tool's use montage on the first-person arms. No-op with empty hands or no montage. */
	UFUNCTION(BlueprintCallable, Category = "Tools")
	void PlayEquippedToolUseMontage();

	/** Currently equipped slot index; INDEX_NONE for empty hands */
	UFUNCTION(BlueprintPure, Category = "Tools")
	int32 GetEquippedToolSlot() const { return EquippedToolSlot; }

	/** Tag of the tool in hands; empty tag when unequipped */
	UFUNCTION(BlueprintPure, Category = "Tools")
	FGameplayTag GetEquippedToolTag() const;

	/** A slot is unlocked once its tool tag has been collected into the inventory */
	UFUNCTION(BlueprintPure, Category = "Tools")
	bool IsToolSlotUnlocked(int32 SlotIndex) const;

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFlowComponent* GetFlowComponent() const { return FlowComponent; }

private:

	TWeakObjectPtr<UCarryableComponent> CarriedItem;

	/** MaxWalkSpeed before the carry slow was applied; assumes nothing else mutates it mid-carry */
	float CachedWalkSpeed = 0.0f;

	/** Spawned tool actors, parallel to UPlayerSettings::ToolSlots; nullptr until first equipped */
	UPROPERTY()
	TArray<TObjectPtr<AToolActor>> ToolActors;

	int32 EquippedToolSlot = INDEX_NONE;

	/** Arms AnimBP assigned in the Blueprint; captured on BeginPlay and restored when a tool is unequipped */
	UPROPERTY()
	TSubclassOf<UAnimInstance> DefaultFirstPersonAnimClass;

	UPROPERTY()
	TSubclassOf<UAnimInstance> DefaultThirdPersonAnimClass;

	void HandleItemCollected(const FGameplayTag& ItemTag, int32 NewCount);

	AToolActor* GetOrSpawnToolActor(int32 SlotIndex);

	UBasicUIManager* GetUIManager() const;
};


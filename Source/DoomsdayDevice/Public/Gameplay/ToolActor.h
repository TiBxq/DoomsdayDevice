#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ToolActor.generated.h"

class USkeletalMeshComponent;
class UAnimInstance;
class UAnimMontage;

/**
 * Hand-held tool visual. Spawned by the character on first equip and attached to the
 * first-person arms; hidden instead of destroyed on unequip. Mesh and grip offset are
 * configured in Blueprint subclasses (referenced by FToolSlotDefinition::ToolActorClass).
 * The root snaps to the hand socket; the mesh child's relative transform is the grip offset.
 */
UCLASS(Blueprintable)
class DOOMSDAYDEVICE_API AToolActor : public AActor
{
	GENERATED_BODY()

public:
	AToolActor();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetMesh() const { return MeshComponent; }

	/** Returns the third person mesh */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }

	/** Arms AnimBP applied to the character's first-person mesh while this tool is equipped */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const { return FirstPersonAnimInstanceClass; }

	/** Returns the third person anim instance class */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const { return ThirdPersonAnimInstanceClass; }

	/** Montage played on the character's arms when this tool is successfully used */
	UAnimMontage* GetUseMontage() const { return UseMontage; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> ThirdPersonMesh;

	/** Arms AnimBP to apply to the first-person mesh while this tool is equipped */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** AnimInstance class to set for the third person character mesh when this tool is active */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** Montage played on the first-person arms on a successful (tool-gated) use */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> UseMontage;
};

// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Gameplay/InteractionComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionComponent)

FPlayerInInteractionEvent UInteractionComponent::OnPlayerEnter;
FPlayerInInteractionEvent UInteractionComponent::OnPlayerExit;

UInteractionComponent::UInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Distance(100.0f)
{
	bAutoActivate = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetUsingAbsoluteScale(true);
	ArrowColor = FColor::Red;
	ArrowSize = 0.5f;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bEnabled)
	{
		Enable();
	}
}

void UInteractionComponent::Enable()
{
	if (const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		bEnabled = true;

		CameraManager = PlayerController->PlayerCameraManager;
		PrimaryComponentTick.SetTickFunctionEnable(true);
	}
}

void UInteractionComponent::Disable()
{
	if (bCanInteract)
	{
		bCanInteract = false;
		OnPlayerExit.Broadcast(this);
	}

	bEnabled = false;

	PrimaryComponentTick.SetTickFunctionEnable(false);
	CameraManager = nullptr;
}

bool UInteractionComponent::IsToolRequirementMet(const FGameplayTag& EquippedToolTag) const
{
	return !RequiredToolTag.IsValid() || EquippedToolTag.MatchesTag(RequiredToolTag);
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bConditionsMet = false;
	if (CameraManager.IsValid() && GetWorld())
	{
		const FVector DistanceToCamera = GetComponentLocation() - CameraManager->GetCameraLocation();
		bool bCloseEnough = DistanceToCamera.Size() < Distance;

		if (bCloseEnough)
		{
			if (bPrecise)
			{
				AActor* PlayerPawn = Cast<AActor>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

				FHitResult Result;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(PlayerPawn);
				FVector Target = CameraManager->GetCameraLocation() + CameraManager->GetCameraRotation().Vector() * Distance * 2;
				GetWorld()->LineTraceSingleByChannel(Result, CameraManager->GetCameraLocation(), Target, ECollisionChannel::ECC_Visibility, QueryParams);

				//DrawDebugLine(GetWorld(), CameraManager->GetCameraLocation(), Target, FColor(255, 0, 0), false, 5, 1, 3.f);

				bConditionsMet = Result.bBlockingHit && Result.GetActor() == GetOwner();
			}
			else
			{
				bConditionsMet = true;
			}
		}
	}

	if (bConditionsMet)
	{
		if (!bCanInteract)
		{
			bCanInteract = true;
			OnPlayerEnter.Broadcast(this);
		}
	}
	else if (bCanInteract)
	{
		bCanInteract = false;
		OnPlayerExit.Broadcast(this);
	}
}

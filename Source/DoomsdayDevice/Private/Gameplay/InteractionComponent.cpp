// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Gameplay/InteractionComponent.h"

#include "DoomsdayDevice.h"
#include "Gameplay/InteractionPromptData.h"
#include "Gameplay/ToolSlotLibrary.h"
#include "Player/PlayerSettings.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionComponent)

#define LOCTEXT_NAMESPACE "InteractionComponent"

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

	ResolveDefaultPrompt();

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

FInteractionPrompt UInteractionComponent::EvaluatePrompt_Implementation(const FGameplayTag& EquippedToolTag) const
{
	FInteractionPrompt Result;
	Result.bCanUse = IsToolRequirementMet(EquippedToolTag);
	Result.PromptText = Result.bCanUse ? GetResolvedUseText() : GetResolvedBlockedText();

	return Result;
}

void UInteractionComponent::ResolveDefaultPrompt()
{
	if (Prompt || ResolvedDefaultPrompt)
	{
		return;
	}

	// UPlayerSettings is config storage and never loads or caches this itself - see its class comment
	const TSoftObjectPtr<UInteractionPromptData>& DefaultPrompt = GetDefault<UPlayerSettings>()->DefaultPrompt;
	if (DefaultPrompt.IsNull())
	{
		// every interaction without its own Prompt asset lands here, so warn once for the session
		static bool bWarnedMissingDefaultPrompt = false;
		if (!bWarnedMissingDefaultPrompt)
		{
			bWarnedMissingDefaultPrompt = true;
			UE_LOG(LogDoomsdayDevice, Warning, TEXT("Project Settings > Player > Interaction > Default Prompt is not set; interactions without their own Prompt asset fall back to the built-in text."));
		}

		return;
	}

	ResolvedDefaultPrompt = DefaultPrompt.LoadSynchronous();
}

const UInteractionPromptData* UInteractionComponent::GetPromptData() const
{
	return Prompt ? Prompt.Get() : ResolvedDefaultPrompt.Get();
}

FText UInteractionComponent::GetResolvedUseText() const
{
	const UInteractionPromptData* PromptData = GetPromptData();
	if (PromptData && !PromptData->UseText.IsEmpty())
	{
		return PromptData->UseText;
	}

	// last resort, so an unconfigured project shows the old prompt rather than a blank one
	return LOCTEXT("FallbackUseText", "Use");
}

FText UInteractionComponent::GetResolvedBlockedText() const
{
	// deliberately only the interaction's own asset - an assigned prompt with an empty BlockedText
	// wants the generated "<Tool> required", not the default asset's blocked line
	const UInteractionPromptData* PromptData = GetPromptData();
	if (PromptData && !PromptData->BlockedText.IsEmpty())
	{
		return PromptData->BlockedText;
	}

	const UPlayerSettings* Settings = GetDefault<UPlayerSettings>();

	const FText ToolName = UToolSlotLibrary::GetToolDisplayNameForRequirement(RequiredToolTag);
	if (ToolName.IsEmpty())
	{
		return Settings->ToolRequiredText;
	}

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Tool"), ToolName);
	return FText::Format(Settings->ToolRequiredTextFormat, Arguments);
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

#undef LOCTEXT_NAMESPACE

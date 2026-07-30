#include "Player/SaveableComponent.h"

#include "Flow/FlowSaveSubsystem.h"

USaveableComponent::USaveableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USaveableComponent::OnRegister()
{
    Super::OnRegister();

#if WITH_EDITOR
    if (!SaveId.IsValid() && GetWorld() && !GetWorld()->IsGameWorld())
    {
        SaveId = FGuid::NewGuid();
        Modify();
    }
#endif
}

void USaveableComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!SaveId.IsValid())
    {
        SaveId = FGuid::NewGuid();
        bRuntimeSpawned = true;
    }

    if (UFlowSaveSubsystem* SaveSys = GetOwner()->GetGameInstance()->GetSubsystem<UFlowSaveSubsystem>())
    {
        SaveSys->RegisterSaveable(this);
        SaveSys->RestoreActorIfPending(this);
    }
}

void USaveableComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UFlowSaveSubsystem* SaveSys = GetOwner()->GetGameInstance()->GetSubsystem<UFlowSaveSubsystem>())
    {
        SaveSys->UnregisterSaveable(this, Reason == EEndPlayReason::Destroyed);
    }
    Super::EndPlay(Reason);
}
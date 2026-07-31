#include "Player/SaveableComponent.h"

#include "Flow/FlowSaveSubsystem.h"

USaveableComponent::USaveableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USaveableComponent::OnRegister()
{
    Super::OnRegister();
}

void USaveableComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner()->HasAnyFlags(RF_WasLoaded))
    {
        FString Path = GetOwner()->GetPathName();
        Path = UWorld::RemovePIEPrefix(Path);

        SaveId = FGuid::NewDeterministicGuid(Path);
    }
    else
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
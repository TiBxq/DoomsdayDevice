#pragma once

#include "SaveableComponent.generated.h"

UCLASS(ClassGroup = (Save), meta = (BlueprintSpawnableComponent))
class USaveableComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    USaveableComponent();

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Save")
    FGuid SaveId;

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    bool bRuntimeSpawned = false;

    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
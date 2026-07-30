#pragma once

#include "Saveable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class USaveable : public UInterface
{
    GENERATED_BODY()
};

class ISaveable
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save System")
    void OnPreSave();
    virtual void OnPreSave_Implementation() {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save System")
    void OnPostLoadSaved();
    virtual void OnPostLoadSaved_Implementation() {}
};
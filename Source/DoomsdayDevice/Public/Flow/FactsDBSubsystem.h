// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"

#include "FactsDBSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FFactChangedEvent, const FGameplayTag& /*Tag*/, int32 /*NewValue*/);

/**
 *
 */
UCLASS()
class DOOMSDAYDEVICE_API UFactsDBSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Broadcast whenever a fact's value is set or changed (AddFact / IncrementFact). */
	FFactChangedEvent OnFactChanged;

	UFUNCTION(BlueprintCallable)
	void AddFact(const FGameplayTag& Tag, const int32 Value);

	UFUNCTION(BlueprintCallable)
	void IncrementFact(const FGameplayTag& Tag, const int32 Value);

	UFUNCTION(BlueprintCallable)
	void RemoveFact(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable)
	bool DoesFactExist(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintCallable)
	int32 GetFactValue(const FGameplayTag& Tag) const;

private:
	TMap<FGameplayTag, int32> Facts;
};

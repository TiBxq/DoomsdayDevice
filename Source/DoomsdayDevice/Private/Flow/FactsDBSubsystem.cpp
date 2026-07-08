// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/FactsDBSubsystem.h"

void UFactsDBSubsystem::AddFact(const FGameplayTag& Tag, const int32 Value)
{
	Facts.Add(Tag, Value);
}

void UFactsDBSubsystem::IncrementFact(const FGameplayTag& Tag, const int32 Value)
{
	Facts.FindOrAdd(Tag) += Value;
}

void UFactsDBSubsystem::RemoveFact(const FGameplayTag& Tag)
{
	Facts.Remove(Tag);
}

bool UFactsDBSubsystem::DoesFactExist(const FGameplayTag& Tag) const
{
	return Facts.Contains(Tag);
}

int32 UFactsDBSubsystem::GetFactValue(const FGameplayTag& Tag) const
{
	if (const int32* ValueRef = Facts.Find(Tag))
	{
		return *ValueRef;
	}
	return -1;
}
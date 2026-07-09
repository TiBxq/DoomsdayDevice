// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogSpeakerDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class DOOMSDAYDEVICE_API UDialogSpeakerDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;
};

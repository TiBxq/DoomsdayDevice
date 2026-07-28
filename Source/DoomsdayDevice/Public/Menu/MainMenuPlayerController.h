#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UUserWidget;

/**
 * Player controller for the Main Menu level. Creates the menu widget (class from
 * UPlayerSettings::MainMenuWidget) and switches to UI-only input with a visible cursor.
 * Deliberately does NOT add the gameplay input mapping contexts.
 */
UCLASS()
class AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> MainMenuWidget;
};

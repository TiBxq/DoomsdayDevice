#include "Menu/MainMenuPlayerController.h"

#include "Player/PlayerSettings.h"

#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MainMenuPlayerController)

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UClass* WidgetClass = GetDefault<UPlayerSettings>()->MainMenuWidget.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MainMenuPlayerController: MainMenuWidget class is not set in Player settings."));
		return;
	}

	MainMenuWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!MainMenuWidget)
	{
		return;
	}

	MainMenuWidget->AddToViewport();

	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

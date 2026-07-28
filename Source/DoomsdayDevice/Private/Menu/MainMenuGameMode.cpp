#include "Menu/MainMenuGameMode.h"

#include "Menu/MainMenuPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MainMenuGameMode)

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}

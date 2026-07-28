#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * Lightweight game mode for the Main Menu level: no gameplay pawn/HUD, and a UI-only
 * player controller. Assigned via the menu map's World Settings GameModeOverride so it
 * does not affect the global default (BP_FirstPersonGameMode).
 */
UCLASS()
class AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();
};

#include "UI/MainMenuWidget.h"

#include "Flow/FlowSaveSubsystem.h"
#include "Player/PlayerSettings.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MainMenuWidget)

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::ContinueGame);
		ContinueButton->SetVisibility(IsContinueAvailable() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::StartNewGame);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::ExitGame);
	}
}

void UMainMenuWidget::ContinueGame()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlowSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UFlowSaveSubsystem>())
		{
			SaveSubsystem->PreloadSaveGame();
		}
	}

	TravelToGameplayLevel();
}

void UMainMenuWidget::StartNewGame()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlowSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UFlowSaveSubsystem>())
		{
			SaveSubsystem->DeleteSaveGame();
		}
	}

	TravelToGameplayLevel();
}

void UMainMenuWidget::ExitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

bool UMainMenuWidget::IsContinueAvailable() const
{
	return UFlowSaveSubsystem::HasSaveGame();
}

void UMainMenuWidget::TravelToGameplayLevel()
{
	const TSoftObjectPtr<UWorld> GameplayLevel = GetDefault<UPlayerSettings>()->GameplayLevel;
	if (GameplayLevel.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("MainMenuWidget: GameplayLevel is not set in Player settings; cannot travel."));
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameplayLevel);
}

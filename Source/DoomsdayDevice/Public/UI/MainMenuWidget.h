#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;

/**
 * Base for the main menu. Owns the New Game / Continue / Exit actions. Buttons named
 * ContinueButton / NewGameButton / ExitButton in the WBP are auto-bound here and hooked to
 * the actions in NativeConstruct; ContinueButton is hidden when no save exists. The action
 * methods stay BlueprintCallable so a hand-authored WBP can also call them directly.
 */
UCLASS()
class DOOMSDAYDEVICE_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Preload the checkpoint save, then travel into the gameplay level. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ContinueGame();

	/** Delete any existing save, reset game state, then start the gameplay level fresh. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void StartNewGame();

	/** Quit the application. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ExitGame();

	/** True when a checkpoint save exists (drives Continue button visibility). */
	UFUNCTION(BlueprintPure, Category = "Main Menu")
	bool IsContinueAvailable() const;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ExitButton;

private:
	void TravelToGameplayLevel();
};

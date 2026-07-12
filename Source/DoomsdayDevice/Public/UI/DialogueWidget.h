// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class UDialogSpeakerDataAsset;
class URichTextBlock;

/**
 *
 */
UCLASS()
class DOOMSDAYDEVICE_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData);

	void SetupDialogueChoices(const TArray<FText>& ChoiceTexts);

	void ClearDialogueChoices();

	/** Returns true if a reveal was in progress and this call completed it (press consumed). */
	bool SkipReveal();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsRevealing() const { return bIsRevealing; }

	UFUNCTION(BlueprintImplementableEvent)
	void DisplayDialogueLine(const FText& LineText, UDialogSpeakerDataAsset* SpeakerData);

	UFUNCTION(BlueprintImplementableEvent)
	void DisplayDialogueChoices(const TArray<FText>& ChoiceTexts);

	UFUNCTION(BlueprintImplementableEvent)
	void RemoveDialogueChoices();

	/** Fires when the line reveal finishes, both naturally and via SkipReveal. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnLineRevealCompleted();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Dialogue")
	TObjectPtr<URichTextBlock> DialogueText;

	/** Visible characters revealed per second; <= 0 reveals instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	float CharactersPerSecond = 40.f;

private:
	/** One flat run of the line's rich-text markup: plain text, a styled run, or a self-closing decorator. */
	struct FRevealSegment
	{
		FString OpenTag;
		FString Content;
		bool bSelfClosing = false;
	};

	void StartReveal(const FText& LineText);
	void CompleteReveal();
	void ApplyRevealedText(int32 VisibleCount) const;
	static TArray<FRevealSegment> TokenizeMarkup(const FString& Source, int32& OutTotalVisibleChars);

	FText FullLineText;
	TArray<FRevealSegment> Segments;
	int32 TotalVisibleChars = 0;
	int32 RevealedVisibleChars = 0;
	float RevealAccumulator = 0.f;
	bool bIsRevealing = false;
};

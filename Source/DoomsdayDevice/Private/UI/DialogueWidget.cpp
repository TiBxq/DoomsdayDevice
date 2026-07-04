// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DialogueWidget.h"

void UDialogueWidget::AddDialogueLine(const FText& LineText)
{
	DisplayDialogueLine(LineText);
}

void UDialogueWidget::SetupDialogueChoices(const TArray<FText>& ChoiceTexts)
{
	DisplayDialogueChoices(ChoiceTexts);
}

void UDialogueWidget::ClearDialogueChoices()
{
	RemoveDialogueChoices();
}
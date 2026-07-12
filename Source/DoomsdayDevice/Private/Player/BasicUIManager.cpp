// Copyright https://github.com/MothCocoon/FlowGame/graphs/contributors

#include "Player/BasicUIManager.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Player/PlayerSettings.h"
#include "UI/DialogueWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicUIManager)

UBasicUIManager::UBasicUIManager()
{
}

void UBasicUIManager::OpenWidget(const TSoftClassPtr<UUserWidget> SoftClass)
{
	if (SoftClass == nullptr || OpenedWidgets.Contains(SoftClass))
	{
		return;
	}

	UClass* WidgetClass = SoftClass.LoadSynchronous();
	if (WidgetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}

	UUserWidget* Widget = Cast<UUserWidget>(UWidgetBlueprintLibrary::Create(GetWorld(), WidgetClass, GetWorld()->GetFirstPlayerController()));
	OpenedWidgets.Add(SoftClass, Widget);
	Widget->AddToViewport();
}

void UBasicUIManager::CloseWidget(const TSoftClassPtr<UUserWidget> SoftClass)
{
	if (UUserWidget* Widget = OpenedWidgets.FindRef(SoftClass))
	{
		Widget->RemoveFromParent();
		OpenedWidgets.Remove(SoftClass);
	}
}

void UBasicUIManager::ToggleWidget(const TSoftClassPtr<UUserWidget> SoftClass)
{
	if (OpenedWidgets.Contains(SoftClass))
	{
		CloseWidget(SoftClass);
	}
	else
	{
		OpenWidget(SoftClass);
	}
}

void UBasicUIManager::HideWidgets()
{
	for (auto WidgetIt = OpenedWidgets.CreateIterator(); WidgetIt; ++WidgetIt)
	{
		const TPair<TSoftClassPtr<UUserWidget>, UUserWidget*> Widget = *WidgetIt;

		HiddenWidgets.Add(Widget.Key);
		CloseWidget(Widget.Key);
	}
}

void UBasicUIManager::RestoreWidgets()
{
	for (const TSoftClassPtr<UUserWidget>& WidgetClass : HiddenWidgets)
	{
		OpenWidget(WidgetClass);
	}
	HiddenWidgets.Empty();
}

void UBasicUIManager::DisplayDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData)
{
	if (UDialogueWidget* Widget = GetDialogueWidget(true))
	{
		Widget->AddDialogueLine(LineText, SpeakerData);
	}
}

void UBasicUIManager::SetupDialogueChoices(const TArray<FText>& ChoiceTexts)
{
	if (UDialogueWidget* Widget = GetDialogueWidget(true))
	{
		Widget->SetupDialogueChoices(ChoiceTexts);
	}
}

void UBasicUIManager::ClearDialogueChoices()
{
	if (UDialogueWidget* Widget = GetDialogueWidget(false))
	{
		Widget->ClearDialogueChoices();
	}
}

bool UBasicUIManager::SkipDialogueLineReveal()
{
	if (UDialogueWidget* Widget = GetDialogueWidget(false))
	{
		return Widget->SkipReveal();
	}
	return false;
}

UDialogueWidget* UBasicUIManager::GetDialogueWidget(const bool bOpenIfNeeded)
{
	const TSoftClassPtr<UUserWidget> DialogueWidgetClass = GetDefault<UPlayerSettings>()->DialogueWidget;

	if (bOpenIfNeeded && !OpenedWidgets.Contains(DialogueWidgetClass))
	{
		OpenWidget(DialogueWidgetClass);
	}

	return Cast<UDialogueWidget>(OpenedWidgets.FindRef(DialogueWidgetClass));
}
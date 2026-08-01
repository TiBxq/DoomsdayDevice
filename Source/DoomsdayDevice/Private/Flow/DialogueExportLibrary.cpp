// Fill out your copyright notice in the Description page of Project Settings.


#include "Flow/DialogueExportLibrary.h"

#include "DoomsdayDevice.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "Dialogue/DialogSpeakerDataAsset.h"
#include "Flow/Nodes/FlowNode_DialogueLine.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/Package.h"
#endif

#define LOCTEXT_NAMESPACE "DialogueExportLibrary"

namespace DoomsdayDialogueExport
{
	/** RFC 4180: always quote, and double any quote inside. Dialogue text carries rich-text markup, commas and newlines. */
	static FString EscapeCsvField(const FString& Field)
	{
		FString Escaped = Field;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);

		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}
}

void UDialogueExportLibrary::GatherDialogueLines(const FString& RootPath, bool bAssignMissingIds, TArray<FDialogueLineExportRow>& OutRows, int32& OutAssignedIdCount, int32& OutDirtiedAssetCount)
{
	OutRows.Reset();
	OutAssignedIdCount = 0;
	OutDirtiedAssetCount = 0;

#if WITH_EDITOR
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// A scan still in flight would silently return a partial asset list, and a partial scan means missing dialogue.
	AssetRegistry.WaitForCompletion();

	// Flow assets publish no node data as registry tags, so every graph has to be loaded to be inspected.
	FARFilter Filter;
	Filter.ClassPaths.Add(UFlowAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(*(RootPath.IsEmpty() ? TEXT("/Game") : RootPath)));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> FlowAssetDataList;
	AssetRegistry.GetAssets(Filter, FlowAssetDataList);

	// Alphabetical package order keeps the CSV grouped by asset and diffable between runs.
	FlowAssetDataList.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.LexicalLess(B.PackageName);
	});

	FScopedSlowTask SlowTask(static_cast<float>(FlowAssetDataList.Num()), LOCTEXT("ScanningFlowAssets", "Scanning Flow assets for dialogue lines..."));
	SlowTask.MakeDialog();

	for (const FAssetData& FlowAssetData : FlowAssetDataList)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromName(FlowAssetData.AssetName));

		UFlowAsset* FlowAsset = Cast<UFlowAsset>(FlowAssetData.GetAsset());
		if (!IsValid(FlowAsset))
		{
			continue;
		}

		TArray<UFlowNode_DialogueLine*> DialogueNodes;
		for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
		{
			if (UFlowNode_DialogueLine* DialogueNode = Cast<UFlowNode_DialogueLine>(Node.Value))
			{
				DialogueNodes.Add(DialogueNode);
			}
		}

		if (DialogueNodes.Num() == 0)
		{
			continue;
		}

		// GUID order reads arbitrarily but never shifts when nodes are moved or rewired,
		// which is what keeps an already-assigned ID pinned to its line.
		DialogueNodes.Sort([](const UFlowNode_DialogueLine& A, const UFlowNode_DialogueLine& B)
		{
			return A.GetGuid().ToString().Compare(B.GetGuid().ToString(), ESearchCase::CaseSensitive) < 0;
		});

		const FString AssetName = FlowAsset->GetName();
		const FString IdPrefix = AssetName.ToUpper();

		// Reserve hand-authored IDs up front so a generated one can never collide with them.
		TSet<FString> UsedIds;
		for (const UFlowNode_DialogueLine* DialogueNode : DialogueNodes)
		{
			if (!DialogueNode->GetDialogueID().IsEmpty())
			{
				UsedIds.Add(DialogueNode->GetDialogueID());
			}
		}

		int32 NextIdIndex = 0;
		bool bAssignedInThisAsset = false;

		for (UFlowNode_DialogueLine* DialogueNode : DialogueNodes)
		{
			if (bAssignMissingIds && DialogueNode->GetDialogueID().IsEmpty())
			{
				FString NewId;
				do
				{
					NewId = FString::Printf(TEXT("%s_%03d"), *IdPrefix, ++NextIdIndex);
				}
				while (UsedIds.Contains(NewId));

				UsedIds.Add(NewId);

				DialogueNode->Modify();
				DialogueNode->SetDialogueID(NewId);

				bAssignedInThisAsset = true;
				++OutAssignedIdCount;
			}

			const UDialogSpeakerDataAsset* SpeakerData = DialogueNode->GetSpeakerData();

			FDialogueLineExportRow& Row = OutRows.AddDefaulted_GetRef();
			Row.FlowAssetName = AssetName;
			Row.DialogueID = DialogueNode->GetDialogueID();
			Row.SpeakerName = SpeakerData ? SpeakerData->DisplayName.ToString() : FString();
			Row.DialogueLineText = DialogueNode->GetLineText().ToString();
			Row.DevComment = DialogueNode->GetDevComment();
		}

		if (bAssignedInThisAsset)
		{
			FlowAsset->MarkPackageDirty();
			++OutDirtiedAssetCount;
		}
	}
#endif
}

bool UDialogueExportLibrary::ExportDialogueLinesToCsv(const FString& OutputFilePath, const FString& RootPath, bool bAssignMissingIds, FString& OutResultMessage)
{
#if WITH_EDITOR
	const FString ResolvedPath = OutputFilePath.IsEmpty() ? GetDefaultExportFilePath() : OutputFilePath;

	TArray<FDialogueLineExportRow> Rows;
	int32 AssignedIdCount = 0;
	int32 DirtiedAssetCount = 0;
	GatherDialogueLines(RootPath, bAssignMissingIds, Rows, AssignedIdCount, DirtiedAssetCount);

	if (Rows.Num() == 0)
	{
		OutResultMessage = FString::Printf(TEXT("No Dialogue Line nodes found under '%s'."), *RootPath);
		UE_LOG(LogDoomsdayDevice, Warning, TEXT("%s"), *OutResultMessage);

		return false;
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ResolvedPath), true);

	// ForceUTF8 emits the BOM, which is what makes spreadsheet apps read non-ASCII dialogue instead of mangling it.
	if (!FFileHelper::SaveStringToFile(BuildCsv(Rows), *ResolvedPath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		OutResultMessage = FString::Printf(TEXT("Failed to write '%s'."), *ResolvedPath);
		UE_LOG(LogDoomsdayDevice, Error, TEXT("%s"), *OutResultMessage);

		return false;
	}

	OutResultMessage = FString::Printf(TEXT("Exported %d dialogue line(s) to '%s'."), Rows.Num(), *ResolvedPath);
	if (AssignedIdCount > 0)
	{
		OutResultMessage += FString::Printf(TEXT("\nAssigned %d new ID(s) across %d asset(s) - save them with File > Save All."), AssignedIdCount, DirtiedAssetCount);
	}

	UE_LOG(LogDoomsdayDevice, Log, TEXT("%s"), *OutResultMessage);

	return true;
#else
	OutResultMessage = TEXT("Dialogue CSV export is editor-only.");

	return false;
#endif
}

FString UDialogueExportLibrary::GetDefaultExportFilePath()
{
	const FString FileName = FString::Printf(TEXT("DialogueLines_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));

	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueExport"), FileName));
}

FString UDialogueExportLibrary::BuildCsv(const TArray<FDialogueLineExportRow>& Rows)
{
	FString Csv = TEXT("FlowAssetName,DialogueID,SpeakerName,DialogueLineText,DevComment\r\n");

	for (const FDialogueLineExportRow& Row : Rows)
	{
		Csv += DoomsdayDialogueExport::EscapeCsvField(Row.FlowAssetName);
		Csv += TEXT(",");
		Csv += DoomsdayDialogueExport::EscapeCsvField(Row.DialogueID);
		Csv += TEXT(",");
		Csv += DoomsdayDialogueExport::EscapeCsvField(Row.SpeakerName);
		Csv += TEXT(",");
		Csv += DoomsdayDialogueExport::EscapeCsvField(Row.DialogueLineText);
		Csv += TEXT(",");
		Csv += DoomsdayDialogueExport::EscapeCsvField(Row.DevComment);
		Csv += TEXT("\r\n");
	}

	return Csv;
}

#undef LOCTEXT_NAMESPACE

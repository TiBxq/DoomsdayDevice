// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DialogueExportLibrary.generated.h"

/** One exported row: a single Dialogue Line node found in a Flow asset. */
USTRUCT(BlueprintType)
struct FDialogueLineExportRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue Export")
	FString FlowAssetName;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue Export")
	FString DialogueID;

	/** DisplayName of the node's speaker data asset; empty when the line has no speaker assigned. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue Export")
	FString SpeakerName;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue Export")
	FString DialogueLineText;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue Export")
	FString DevComment;
};

/**
 * Editor tooling that scans the project's Flow assets for Dialogue Line nodes and exports them as CSV.
 * Driven from Content/_Doomsaday/EditorTools/EUW_DialogueExport; does nothing in non-editor builds.
 */
UCLASS()
class DOOMSDAYDEVICE_API UDialogueExportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Loads every Flow asset under RootPath and collects its Dialogue Line nodes, ordered by node GUID.
	 * With bAssignMissingIds, empty DialogueIDs are filled in and their packages marked dirty - nothing is saved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doomsday|Dialogue Export")
	static void GatherDialogueLines(const FString& RootPath, bool bAssignMissingIds, TArray<FDialogueLineExportRow>& OutRows, int32& OutAssignedIdCount, int32& OutDirtiedAssetCount);

	/** Gathers, then writes the rows as a UTF-8 CSV. Returns false and explains itself through OutResultMessage on failure. */
	UFUNCTION(BlueprintCallable, Category = "Doomsday|Dialogue Export")
	static bool ExportDialogueLinesToCsv(const FString& OutputFilePath, const FString& RootPath, bool bAssignMissingIds, FString& OutResultMessage);

	/** <Project>/Saved/DialogueExport/DialogueLines_<timestamp>.csv */
	UFUNCTION(BlueprintPure, Category = "Doomsday|Dialogue Export")
	static FString GetDefaultExportFilePath();

	/** Serializes rows into RFC 4180 CSV text, header row included. */
	static FString BuildCsv(const TArray<FDialogueLineExportRow>& Rows);
};

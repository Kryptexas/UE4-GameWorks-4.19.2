// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TemplateProjectDefs.generated.h"

USTRUCT()
struct FTemplateReplacement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FString> Extensions;

	UPROPERTY()
	FString From;

	UPROPERTY()
	FString To;

	UPROPERTY()
	bool bCaseSensitive;
};

USTRUCT()
struct FTemplateFolderRename
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString From;

	UPROPERTY()
	FString To;
};

USTRUCT()
struct FLocalizedTemplateString
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Language;

	UPROPERTY()
	FString Text;
};

UCLASS(config=TemplateDefs)
class UTemplateProjectDefs : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config)
	TArray<FLocalizedTemplateString> LocalizedDisplayNames;

	UPROPERTY(config)
	TArray<FLocalizedTemplateString> LocalizedDescriptions;

	UPROPERTY(config)
	TArray<FString> FoldersToIgnore;

	UPROPERTY(config)
	TArray<FString> FilesToIgnore;

	UPROPERTY(config)
	TArray<FTemplateFolderRename> FolderRenames;

	UPROPERTY(config)
	TArray<FTemplateReplacement> FilenameReplacements;

	UPROPERTY(config)
	TArray<FTemplateReplacement> ReplacementsInFiles;

	/** Fixes up all strings in this definitions object to replace %TEMPLATENAME% with the supplied template name and %PROJECTNAME% with the supplied project name */
	void FixupStrings(const FString& TemplateName, const FString& ProjectName);

	/** Returns the display name for the current culture */
	FText GetDisplayNameText();

	/** Returns the display name for the current culture */
	FText GetLocalizedDescription();

private:
	void FixString(FString& InOutStringToFix, const FString& TemplateName, const FString& ProjectName);
};


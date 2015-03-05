// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ConfigCacheIni.h"
#include "Classes/LocalizationTarget.h"

struct FLocalizationConfigurationScript : public FConfigFile
{
	FConfigSection& CommonSettings()
	{
		return FindOrAdd(TEXT("CommonSettings"));
	}

	FConfigSection& GatherTextStep(const uint32 Index)
	{
		return FindOrAdd( FString::Printf( TEXT("GatherTextStep%u"), Index) );
	}
};

namespace LocalizationConfigurationScript
{
	FString MakePathRelativeToProjectDirectory(const FString& Path);

	FString GetScriptDirectory();
	FString GetDataDirectory(const FLocalizationTargetSettings& Target);
	TArray<FString> GetScriptPaths(const FLocalizationTargetSettings& Target);

	FString GetManifestPath(const FLocalizationTargetSettings& Target);
	FString GetArchivePath(const FLocalizationTargetSettings& Target, const FString& CultureName);
	FString GetDefaultPOFileName(const FLocalizationTargetSettings& Target);
	FString GetDefaultPOPath(const FLocalizationTargetSettings& Target, const FString& CultureName);
	FString GetLocResPath(const FLocalizationTargetSettings& Target, const FString& CultureName);
	FString GetWordCountCSVPath(const FLocalizationTargetSettings& Target);
	FString GetConflictReportPath(const FLocalizationTargetSettings& Target);

	FLocalizationConfigurationScript GenerateGatherScript(const FLocalizationTargetSettings& Target);
	FString GetGatherScriptPath(const FLocalizationTargetSettings& Target);

	FLocalizationConfigurationScript GenerateImportScript(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName = TOptional<FString>(), const TOptional<FString> OutputPathOverride = TOptional<FString>());
	FString GetImportScriptPath(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName = TOptional<FString>());

	FLocalizationConfigurationScript GenerateExportScript(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName = TOptional<FString>(), const TOptional<FString> OutputPathOverride = TOptional<FString>());
	FString GetExportScriptPath(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName = TOptional<FString>());

	FLocalizationConfigurationScript GenerateReportScript(const FLocalizationTargetSettings& Target);
	FString GetReportScriptPath(const FLocalizationTargetSettings& Target);

	FLocalizationConfigurationScript GenerateCompileScript(const FLocalizationTargetSettings& Target);
	FString GetCompileScriptPath(const FLocalizationTargetSettings& Target);
}
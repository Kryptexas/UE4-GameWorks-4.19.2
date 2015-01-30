// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Classes/LocalizationTarget.h"

namespace LocalizationCommandletTasks
{
	bool GatherTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool GatherTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);

	bool ImportTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings, const TOptional<FString> DirectoryPath = TOptional<FString>());
	bool ImportTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const TOptional<FString> DirectoryPath = TOptional<FString>());
	bool ImportCulture(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const FString& CultureName, const TOptional<FString> FilePath = TOptional<FString>());

	bool ExportTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings, const TOptional<FString> DirectoryPath = TOptional<FString>());
	bool ExportTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const TOptional<FString> DirectoryPath = TOptional<FString>());
	bool ExportCulture(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const FString& CultureName, const TOptional<FString> FilePath = TOptional<FString>());

	bool GenerateReportsForTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool GenerateReportsForTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);

	bool CompileTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool CompileTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);
}
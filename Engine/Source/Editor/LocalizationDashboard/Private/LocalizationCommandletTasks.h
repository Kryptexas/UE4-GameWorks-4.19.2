// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Classes/LocalizationTarget.h"

namespace LocalizationCommandletTasks
{
	bool GatherTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool GatherTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);

	bool ImportTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool ImportTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);
	bool ImportCulture(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const FString& CultureName);

	bool ExportTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool ExportTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);
	bool ExportCulture(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const FString& CultureName);

	bool GenerateReportsForTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings);
	bool GenerateReportsForTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings);
}
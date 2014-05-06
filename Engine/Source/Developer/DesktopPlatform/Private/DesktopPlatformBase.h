// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDesktopPlatform.h"
#include "Runtime/Core/Public/Serialization/Json/Json.h"

class FDesktopPlatformBase : public IDesktopPlatform
{
public:
	// IDesktopPlatform Implementation
	virtual FString GetCurrentEngineIdentifier() OVERRIDE;

	virtual void EnumerateLauncherEngineInstallations(TMap<FString, FString> &OutInstallations) OVERRIDE;
	virtual bool GetEngineRootDirFromIdentifier(const FString &Identifier, FString &OutRootDir) OVERRIDE;
	virtual bool GetEngineIdentifierFromRootDir(const FString &RootDir, FString &OutIdentifier) OVERRIDE;

	virtual bool GetDefaultEngineIdentifier(FString &OutIdentifier) OVERRIDE;
	virtual bool GetDefaultEngineRootDir(FString &OutRootDir) OVERRIDE;
	virtual bool IsPreferredEngineIdentifier(const FString &Identifier, const FString &OtherIdentifier);

	virtual bool IsSourceDistribution(const FString &RootDir) OVERRIDE;
	virtual bool IsPerforceBuild(const FString &RootDir) OVERRIDE;
	virtual bool IsValidRootDirectory(const FString &RootDir) OVERRIDE;

	virtual bool SetEngineIdentifierForProject(const FString &ProjectFileName, const FString &Identifier) OVERRIDE;
	virtual bool GetEngineIdentifierForProject(const FString &ProjectFileName, FString &OutIdentifier) OVERRIDE;

private:
	void CheckForLauncherEngineInstallation(const FString &AppId, const FString &Identifier, TMap<FString, FString> &OutInstallations);
	int32 ParseReleaseVersion(const FString &Version);

	TSharedPtr<FJsonObject> LoadProjectFile(const FString &FileName);
	bool SaveProjectFile(const FString &FileName, TSharedPtr<FJsonObject> Object);
};

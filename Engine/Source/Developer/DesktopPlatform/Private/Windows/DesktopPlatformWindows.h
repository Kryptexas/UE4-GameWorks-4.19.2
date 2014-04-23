// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DesktopPlatformBase.h"
#include "WindowsRegistry.h"

class FDesktopPlatformWindows : public FDesktopPlatformBase
{
public:
	// IDesktopPlatform Implementation
	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) OVERRIDE;
	virtual bool SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) OVERRIDE;
	virtual bool OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName) OVERRIDE;
	virtual bool OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags) OVERRIDE;
	virtual bool OpenLauncher(bool Install, const FString& CommandLineParams ) OVERRIDE;

	virtual void EnumerateEngineInstallations(TMap<FString, FString> &OutInstallations) OVERRIDE;

	virtual bool IsSourceDistribution(const FString &RootDir) OVERRIDE;

	virtual bool VerifyFileAssociations() OVERRIDE;
	virtual bool UpdateFileAssociations() OVERRIDE;

private:
	bool FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames);
	void GetRequiredRegistrySettings(TIndirectArray<FRegistryRootedKey> &RootedKeys);
	int32 GetShellIntegrationVersion(const FString &FileName);
};

typedef FDesktopPlatformWindows FDesktopPlatform;

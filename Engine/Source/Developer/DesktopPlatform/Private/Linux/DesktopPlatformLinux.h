// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DesktopPlatformBase.h"

class FDesktopPlatformLinux : public FDesktopPlatformBase
{
public:
	// IDesktopPlatform Implementation
	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) OVERRIDE;
	virtual bool SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) OVERRIDE;
	virtual bool OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName) OVERRIDE;
	virtual bool OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags) OVERRIDE;
	virtual bool OpenLauncher(bool Install, FString CommandLineParams ) OVERRIDE;

	virtual bool RegisterEngineInstallation(const FString &RootDir, FString &OutIdentifier) OVERRIDE;
	virtual void EnumerateEngineInstallations(TMap<FString, FString> &OutInstallations) OVERRIDE;

	virtual bool VerifyFileAssociations() OVERRIDE;
	virtual bool UpdateFileAssociations() OVERRIDE;

	virtual bool RunUnrealBuildTool(const FText& Description, const FString& RootDir, const FString& Arguments, FFeedbackContext* Warn) OVERRIDE;

	virtual FFeedbackContext* GetNativeFeedbackContext() OVERRIDE;

private:
	bool FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames);
};

typedef FDesktopPlatformLinux FDesktopPlatform;

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DesktopPlatformBase.h"

class FDesktopPlatformMac : public FDesktopPlatformBase
{
public:
	// IDesktopPlatform Implementation
	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) override;
	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames, int32& OutFilterIndex) override;
	virtual bool SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) override;
	virtual bool OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName) override;
	virtual bool OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags) override;
	virtual bool OpenLauncher(bool Install, FString CommandLineParams ) override;

	virtual bool RegisterEngineInstallation(const FString &RootDir, FString &OutIdentifier) override;
	virtual void EnumerateEngineInstallations(TMap<FString, FString> &OutInstallations) override;

	virtual bool VerifyFileAssociations() override;
	virtual bool UpdateFileAssociations() override;

	virtual bool RunUnrealBuildTool(const FText& Description, const FString& RootDir, const FString& Arguments, FFeedbackContext* Warn) override;

	virtual FFeedbackContext* GetNativeFeedbackContext() override;

private:
	bool FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames);
};

typedef FDesktopPlatformMac FDesktopPlatform;

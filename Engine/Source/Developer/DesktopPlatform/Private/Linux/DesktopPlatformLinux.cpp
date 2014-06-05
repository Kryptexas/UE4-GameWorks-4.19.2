// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DesktopPlatformPrivatePCH.h"

#define LOCTEXT_NAMESPACE "DesktopPlatform"

bool FDesktopPlatformLinux::OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags)
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::OpenLauncher(bool Install, FString CommandLineParams )
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::RegisterEngineInstallation(const FString &RootDir, FString &OutIdentifier)
{
	return false;
}

void FDesktopPlatformLinux::EnumerateEngineInstallations(TMap<FString, FString> &OutInstallations)
{
	unimplemented();
}

bool FDesktopPlatformLinux::VerifyFileAssociations()
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::UpdateFileAssociations()
{
	unimplemented();
	return false;
}

bool FDesktopPlatformLinux::RunUnrealBuildTool(const FText& Description, const FString& RootDir, const FString& Arguments, FFeedbackContext* Warn)
{
	unimplemented();
	return false;
}

FFeedbackContext* FDesktopPlatformLinux::GetNativeFeedbackContext()
{
	unimplemented();
	return NULL;
}

#undef LOCTEXT_NAMESPACE

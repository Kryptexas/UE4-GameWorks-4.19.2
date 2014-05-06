// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../UnrealVersionSelector.h"
#include "WindowsPlatformInstallation.h"
#include "Runtime/Core/Public/Misc/EngineVersion.h"
#include "AllowWindowsPlatformTypes.h"
#include "Resources/Resource.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Shellapi.h>

struct FEngineLabelSortPredicate
{
	bool operator()(const FString &A, const FString &B) const
	{
		return FDesktopPlatformModule::Get()->IsPreferredEngineIdentifier(A, B);
	}
};

FString GetInstallationDescription(const FString &Id, const FString &RootDir)
{
	// Official release versions just have a version number
	if (Id.Len() > 0 && FChar::IsDigit(Id[0]))
	{
		return Id;
	}

	// Otherwise get the path
	FString PlatformRootDir = RootDir;
	FPaths::MakePlatformFilename(PlatformRootDir);

	// Perforce build
	if (FDesktopPlatformModule::Get()->IsSourceDistribution(RootDir))
	{
		return FString::Printf(TEXT("Source build at %s"), *PlatformRootDir);
	}
	else
	{
		return FString::Printf(TEXT("Binary build at %s"), *PlatformRootDir);
	}
}

struct FSelectBuildDialog
{
	FString Identifier;
	TArray<FString> SortedIdentifiers;
	TMap<FString, FString> Installations;

	FSelectBuildDialog(const FString &InIdentifier)
	{
		Identifier = InIdentifier;
	}

	bool DoModal(HWND hWndParent)
	{
		return DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SELECTBUILD), hWndParent, &DialogProc, (LPARAM)this) != FALSE;
	}

private:
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		FSelectBuildDialog *Dialog = (FSelectBuildDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		switch (Msg)
		{
		case WM_INITDIALOG:
			Dialog = (FSelectBuildDialog*)lParam;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)Dialog);
			Dialog->UpdateInstallations(hWnd);
			return false;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_BROWSE:
				if (Dialog->Browse(hWnd))
				{
					EndDialog(hWnd, 1);
				}
				return false;
			case IDOK:
				Dialog->StoreSelection(hWnd);
				EndDialog(hWnd, 1);
				return false;
			case IDCANCEL:
				EndDialog(hWnd, 0);
				return false;
			}

		}
		return FALSE;
	}

	void StoreSelection(HWND hWnd)
	{
		int32 Idx = SendDlgItemMessage(hWnd, IDC_BUILDLIST, CB_GETCURSEL, 0, 0);
		Identifier = SortedIdentifiers.IsValidIndex(Idx) ? SortedIdentifiers[Idx] : TEXT("");
	}

	void UpdateInstallations(HWND hWnd)
	{
		FDesktopPlatformModule::Get()->EnumerateEngineInstallations(Installations);
		Installations.GetKeys(SortedIdentifiers);
		SortedIdentifiers.Sort<FEngineLabelSortPredicate>(FEngineLabelSortPredicate());

		SendDlgItemMessage(hWnd, IDC_BUILDLIST, CB_RESETCONTENT, 0, 0);

		for(int32 Idx =  0; Idx < SortedIdentifiers.Num(); Idx++)
		{
			const FString &Identifier = SortedIdentifiers[Idx];
			FString Description = GetInstallationDescription(Identifier, Installations[Identifier]);
			SendDlgItemMessage(hWnd, IDC_BUILDLIST, CB_ADDSTRING, 0, (LPARAM)*Description);
		}

		int32 NewIdx = FMath::Max(SortedIdentifiers.IndexOfByKey(Identifier), 0);
		SendDlgItemMessage(hWnd, IDC_BUILDLIST, CB_SETCURSEL, NewIdx, 0);
	}

	bool Browse(HWND hWnd)
	{
		// Get the currently bound engine directory for the project
		const FString *RootDir = Installations.Find(Identifier);
		FString EngineRootDir = (RootDir != NULL)? *RootDir : FString();

		// Browse for a new directory
		FString NewEngineRootDir;
		if (!FDesktopPlatformModule::Get()->OpenDirectoryDialog(hWnd, TEXT("Select the Unreal Engine installation to use for this project"), EngineRootDir, NewEngineRootDir))
		{
			return false;
		}

		// Check it's a valid directory
		if (!FPlatformInstallation::NormalizeEngineRootDir(NewEngineRootDir))
		{
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("The selected directory is not a valid engine installation."), TEXT("Error"));
			return false;
		}

		// Check that it's a registered engine directory
		FString NewIdentifier;
		if (!FDesktopPlatformModule::Get()->GetEngineIdentifierFromRootDir(NewEngineRootDir, NewIdentifier))
		{
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Couldn't register engine installation."), TEXT("Error"));
			return false;
		}

		// Update the identifier and return
		Identifier = NewIdentifier;
		return true;
	}
};

bool FWindowsPlatformInstallation::LaunchEditor(const FString &RootDirName, const FString &Arguments)
{
	FString EditorFileName = RootDirName / TEXT("Engine/Binaries/Win64/UE4Editor.exe");
	return FPlatformProcess::ExecProcess(*EditorFileName, *Arguments, NULL, NULL, NULL);
}

bool FWindowsPlatformInstallation::GenerateProjectFiles(const FString &RootDirName, const FString &Arguments)
{
	// Get the path to the batch file
	FString BatchFileName;
	if (FDesktopPlatformModule::Get()->IsSourceDistribution(RootDirName))
	{
		BatchFileName = RootDirName / TEXT("Engine/Build/BatchFiles/GenerateProjectFiles.bat");
	}
	else
	{
		BatchFileName = RootDirName / TEXT("Engine/Build/BatchFiles/RocketGenerateProjectFiles.bat");
	}

	// Check it exists
	if (IFileManager::Get().FileSize(*BatchFileName) < 0)
	{
		return false;
	}

	// Build the full argument list
	FString AllArguments = FString::Printf(TEXT("/c \"\"%s\" %s\""), *BatchFileName, *Arguments);

	// Run the batch file through cmd.exe. 
	FProcHandle Handle = FPlatformProcess::CreateProc(TEXT("cmd.exe"), *AllArguments, false, false, false, NULL, 0, NULL, NULL);
	if (!Handle.IsValid()) return false;

	FPlatformProcess::WaitForProc(Handle);
	return true;
}

bool FWindowsPlatformInstallation::SelectEngineInstallation(FString &Identifier)
{
	FSelectBuildDialog Dialog(Identifier);
	if(!Dialog.DoModal(NULL))
	{
		return false;
	}

	Identifier = Dialog.Identifier;
	return true;
}

#include "HideWindowsPlatformTypes.h"

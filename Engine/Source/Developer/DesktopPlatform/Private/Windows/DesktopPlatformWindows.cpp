// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DesktopPlatformPrivatePCH.h"

#include "AllowWindowsPlatformTypes.h"
	#include <commdlg.h>
	#include <shlobj.h>
#include "HideWindowsPlatformTypes.h"

#define LOCTEXT_NAMESPACE "DesktopPlatform"
#define MAX_FILETYPES_STR 4096
#define MAX_FILENAME_STR 65536 // This buffer has to be big enough to contain the names of all the selected files as well as the null characters between them and the null character at the end

bool FDesktopPlatformWindows::OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	return FileDialogShared(false, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
}

bool FDesktopPlatformWindows::SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	return FileDialogShared(true, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
}

static ::INT CALLBACK BrowseCallbackProc(HWND hwnd, ::UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	// Set the path to the start path upon initialization.
	switch (uMsg)
	{
		case BFFM_INITIALIZED:
		if ( lpData )
		{
			SendMessage(hwnd, BFFM_SETSELECTION, true, lpData);
		}
		break;
	}

	return 0;
}

bool FDesktopPlatformWindows::OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
{
	FScopedSystemModalMode SystemModalScope;

	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));

	TCHAR FolderName[MAX_PATH];
	ZeroMemory(FolderName, sizeof(TCHAR) * MAX_PATH);

	const FString PathToSelect = DefaultPath.Replace(TEXT("/"), TEXT("\\"));

	bi.hwndOwner = (HWND)ParentWindowHandle;
	bi.pszDisplayName = FolderName;
	bi.lpszTitle = *DialogTitle;
	bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_SHAREABLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)(*PathToSelect);
	bool bSuccess = false;
	LPCITEMIDLIST Folder = ::SHBrowseForFolder(&bi);
	if (Folder) 
	{
		bSuccess = ( ::SHGetPathFromIDList(Folder, FolderName) ? true : false );
		if ( bSuccess )
		{
			OutFolderName = FolderName;
			FPaths::NormalizeFilename(OutFolderName);
		}
		else
		{
			UE_LOG(LogDesktopPlatform, Warning, TEXT("Error converting the folder path to a string."));
		}
	}
	else
	{
		UE_LOG(LogDesktopPlatform, Warning, TEXT("Error reading results of folder dialog."));
	}

	return bSuccess;
}

bool FDesktopPlatformWindows::OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags)
{	
	FScopedSystemModalMode SystemModalScope;

	CHOOSEFONT cf;
	ZeroMemory(&cf, sizeof(CHOOSEFONT));

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(LOGFONT));

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = (HWND)ParentWindowHandle;
	cf.lpLogFont = &lf;
	cf.Flags = CF_EFFECTS | CF_SCREENFONTS;
	bool bSuccess = !!::ChooseFont(&cf);
	if ( bSuccess )
	{
		HDC DC = ::GetDC( cf.hwndOwner ); 
		const float LogicalPixelsY = static_cast<float>(GetDeviceCaps(DC, LOGPIXELSY));
		const int32 PixelHeight = static_cast<int32>(-lf.lfHeight * ( 72.0f / LogicalPixelsY ));	// Always target 72 DPI
		uint32 FontFlags = EFontImportFlags::None;
		if ( lf.lfUnderline )
		{
			FontFlags |= EFontImportFlags::EnableUnderline;
		}
		if ( lf.lfItalic )
		{
			FontFlags |= EFontImportFlags::EnableItalic;
		}
		if ( lf.lfWeight == FW_BOLD )
		{
			FontFlags |= EFontImportFlags::EnableBold;
		}

		OutFontName = (const TCHAR*)lf.lfFaceName;
		OutHeight = PixelHeight;
		OutFlags = static_cast<EFontImportFlags::Type>(FontFlags);

		::ReleaseDC( cf.hwndOwner, DC ); 
	}
	else
	{
		UE_LOG(LogDesktopPlatform, Warning, TEXT("Error reading results of font dialog."));
	}

	return bSuccess;
}

bool FDesktopPlatformWindows::OpenLauncher(bool Install, const FString& CommandLineParams )
{
	FString LaunchPath;

	if (FParse::Param(FCommandLine::Get(), TEXT("Dev")))
	{
		LaunchPath = FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries"), TEXT("Win64"), TEXT("UnrealEngineLauncher-Win64-Debug.exe"));
	}
	else
	{
		LaunchPath = FPaths::Combine(*FPaths::EngineDir(), TEXT("../../Launcher/Engine/Binaries/Win64/UnrealEngineLauncher.exe"));
	}

	// if the Launcher doesn't exist locally...
	if (!FPaths::FileExists(LaunchPath))
	{
		FString InstallDir;

		// ... locate it in the Registry...
		if (FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("Software\\EpicGames\\Unreal Engine"), TEXT("InstallDir"), InstallDir) && (InstallDir.Len() > 0))
		{
			LaunchPath = FPaths::Combine(*InstallDir, TEXT("Launcher/Engine/Binaries/Win64/UnrealEngineLauncher.exe"));
		}
		else
		{
			LaunchPath.Empty();
		}

		if (LaunchPath.IsEmpty() || !FPaths::FileExists(LaunchPath))
		{
			if (Install)
			{
				// ... or run the installer if desired
				LaunchPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(*FPaths::EngineDir(), TEXT("Extras/UnrealEngineLauncher/UnrealEngineInstaller.msi")));

				if (FPaths::FileExists(LaunchPath))
				{
					FPlatformProcess::LaunchFileInDefaultExternalApplication(*LaunchPath);

					return true;
				}
			}

			return false;
		}
	}

	return FPlatformProcess::CreateProc(*LaunchPath, *CommandLineParams, true, false, false, NULL, 0, *FPaths::GetPath(LaunchPath), NULL).IsValid();
}

bool FDesktopPlatformWindows::FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	FScopedSystemModalMode SystemModalScope;

	WCHAR Filename[MAX_FILENAME_STR];
	FCString::Strcpy(Filename, MAX_FILENAME_STR, *(DefaultFile.Replace(TEXT("/"), TEXT("\\"))));

	// Convert the forward slashes in the path name to backslashes, otherwise it'll be ignored as invalid and use whatever is cached in the registry
	WCHAR Pathname[MAX_FILENAME_STR];
	FCString::Strcpy(Pathname, MAX_FILENAME_STR, *(FPaths::ConvertRelativePathToFull(DefaultPath).Replace(TEXT("/"), TEXT("\\"))));

	// Convert the "|" delimited list of filetypes to NULL delimited then add a second NULL character to indicate the end of the list
	WCHAR FileTypeStr[MAX_FILETYPES_STR];
	WCHAR* FileTypesPtr = NULL;
	const int32 FileTypesLen = FileTypes.Len();

	// Nicely formatted file types for lookup later and suitable to append to filenames without extensions
	TArray<FString> CleanExtensionList;

	// The strings must be in pairs for windows.
	// It is formatted as follows: Pair1String1|Pair1String2|Pair2String1|Pair2String2
	// where the second string in the pair is the extension.  To get the clean extensions we only care about the second string in the pair
	TArray<FString> UnformattedExtensions;
	FileTypes.ParseIntoArray( &UnformattedExtensions, TEXT("|"), true );
	for( int32 ExtensionIndex = 1; ExtensionIndex < UnformattedExtensions.Num(); ExtensionIndex += 2)
	{
		const FString& Extension = UnformattedExtensions[ExtensionIndex];
		// Assume the user typed in an extension or doesnt want one when using the *.* extension. We can't determine what extension they wan't in that case
		if( Extension != TEXT("*.*") )
		{
			// Add to the clean extension list, first removing the * wildcard from the extension
			int32 WildCardIndex = Extension.Find( TEXT("*") );
			CleanExtensionList.Add( WildCardIndex != INDEX_NONE ? Extension.RightChop( WildCardIndex+1 ) : Extension );
		}
	}

	if (FileTypesLen > 0 && FileTypesLen - 1 < MAX_FILETYPES_STR)
	{
		FileTypesPtr = FileTypeStr;
		FCString::Strcpy(FileTypeStr, MAX_FILETYPES_STR, *FileTypes);

		TCHAR* Pos = FileTypeStr;
		while( Pos[0] != 0 )
		{
			if ( Pos[0] == '|' )
			{
				Pos[0] = 0;
			}

			Pos++;
		}

		// Add two trailing NULL characters to indicate the end of the list
		FileTypeStr[FileTypesLen] = 0;
		FileTypeStr[FileTypesLen + 1] = 0;
	}

	OPENFILENAME ofn;
	FMemory::Memzero(&ofn, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = (HWND)ParentWindowHandle;
	ofn.lpstrFilter = FileTypesPtr;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = Filename;
	ofn.nMaxFile = MAX_FILENAME_STR;
	ofn.lpstrInitialDir = Pathname;
	ofn.lpstrTitle = *DialogTitle;
	if(FileTypesLen > 0)
	{
		ofn.lpstrDefExt = &FileTypeStr[0];
	}

	ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_EXPLORER;

	if ( bSave )
	{
		ofn.Flags |= OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOVALIDATE;
	}
	else
	{
		ofn.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	}

	if ( Flags & EFileDialogFlags::Multiple )
	{
		ofn.Flags |= OFN_ALLOWMULTISELECT;
	}

	bool bSuccess;
	if ( bSave )
	{
		bSuccess = !!::GetSaveFileName(&ofn);
	}
	else
	{
		bSuccess = !!::GetOpenFileName(&ofn);
	}

	if ( bSuccess )
	{
		// GetOpenFileName/GetSaveFileName changes the CWD on success. Change it back immediately.
		FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();

		if ( Flags & EFileDialogFlags::Multiple )
		{
			// When selecting multiple files, the returned string is a NULL delimited list
			// where the first element is the directory and all remaining elements are filenames.
			// There is an extra NULL character to indicate the end of the list.
			FString DirectoryOrSingleFileName = FString(Filename);
			TCHAR* Pos = Filename + DirectoryOrSingleFileName.Len() + 1;

			if ( Pos[0] == 0 )
			{
				// One item selected. There was an extra trailing NULL character.
				OutFilenames.Add(DirectoryOrSingleFileName);
			}
			else
			{
				// Multiple items selected. Keep adding filenames until two NULL characters.
				FString SelectedFile;
				do
				{
					SelectedFile = FString(Pos);
					new(OutFilenames) FString(DirectoryOrSingleFileName / SelectedFile);
					Pos += SelectedFile.Len() + 1;
				}
				while (Pos[0] != 0);
			}
		}
		else
		{
			new(OutFilenames) FString(Filename);
		}

		// The index of the filter in OPENFILENAME starts at 1.
		const uint32 FilterIndex = ofn.nFilterIndex - 1;

		// Get the extension to add to the filename (if one doesnt already exist)
		const FString Extension = CleanExtensionList.IsValidIndex( FilterIndex ) ? CleanExtensionList[FilterIndex] : TEXT("");

		// Make sure all filenames gathered have their paths normalized and proper extensions added
		for ( auto FilenameIt = OutFilenames.CreateIterator(); FilenameIt; ++FilenameIt )
		{
			FString& Filename = *FilenameIt;
			
			Filename = IFileManager::Get().ConvertToRelativePath(*Filename);

			if( FPaths::GetExtension(Filename).IsEmpty() && !Extension.IsEmpty() )
			{
				// filename does not have an extension. Add an extension based on the filter that the user chose in the dialog
				Filename += Extension;
			}

			FPaths::NormalizeFilename(Filename);
		}
	}
	else
	{
		uint32 Error = ::CommDlgExtendedError();
		if ( Error != ERROR_SUCCESS )
		{
			UE_LOG(LogDesktopPlatform, Warning, TEXT("Error reading results of file dialog. Error: 0x%04X"), Error);
		}
	}

	return bSuccess;
}

#undef LOCTEXT_NAMESPACE

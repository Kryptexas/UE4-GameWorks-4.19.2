// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// Core includes.
#include "CorePrivate.h"
#include "Runtime/Launch/Resources/Version.h" 
#include "EngineVersion.h"

DEFINE_LOG_CATEGORY_STATIC(LogPaths, Log, All);


/*-----------------------------------------------------------------------------
	Path helpers for retrieving game dir, engine dir, etc.
-----------------------------------------------------------------------------*/

bool FPaths::ShouldSaveToUserDir()
{
	static bool bShouldSaveToUserDir = FApp::IsInstalled() || FParse::Param(FCommandLine::Get(), TEXT("SaveToUserDir"));
	return bShouldSaveToUserDir;
}

FString FPaths::EngineDir()
{
	return FString(FPlatformMisc::EngineDir());
}

FString FPaths::EngineContentDir()
{
	return FPaths::EngineDir() + TEXT("Content/");
}

FString FPaths::EngineConfigDir()
{
	return FPaths::EngineDir() + TEXT("Config/");
}

FString FPaths::EngineIntermediateDir()
{
	return FPaths::EngineDir() + TEXT("Intermediate/");
}

FString FPaths::EngineSavedDir()
{
	if (ShouldSaveToUserDir() || FApp::IsEngineInstalled())
	{
		return FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT(EPIC_PRODUCT_IDENTIFIER), *GEngineVersion.ToString(EVersionComponent::Minor)) + TEXT("/");
	}

	return FPaths::EngineDir() + TEXT("Saved/");
}

FString FPaths::EnginePluginsDir()
{
	return FPaths::EngineDir() + TEXT("Plugins/");
}

FString FPaths::RootDir()
{
	return FString(FPlatformMisc::RootDir());
}

FString FPaths::GameDir()
{
	return FString(FPlatformMisc::GameDir());
}

FString FPaths::GameContentDir()
{
	return FPaths::GameDir() + TEXT("Content/");
}

FString FPaths::GameSavedDir()
{
	if (ShouldSaveToUserDir())
	{
		return FPaths::Combine(FPlatformProcess::UserSettingsDir(), GGameName, TEXT("Saved/"));
	}

	return FPaths::GameDir() + TEXT("Saved/");
}

FString FPaths::GameIntermediateDir()
{
	return FPaths::GameDir() + TEXT("Intermediate/");
}

FString FPaths::GamePluginsDir()
{
	return FPaths::GameDir() + TEXT("Plugins/");
}

FString FPaths::SourceConfigDir()
{
	return FPaths::GameDir() + TEXT("Config/");
}

FString FPaths::GeneratedConfigDir()
{
	return FPaths::GameSavedDir() + TEXT("Config/");
}

FString FPaths::SandboxesDir()
{
	return FPaths::GameDir() + TEXT("Saved/Sandboxes");
}

FString FPaths::ProfilingDir()
{
	return FPaths::GameSavedDir() + TEXT("Profiling/");
}

FString FPaths::ScreenShotDir()
{
	return FPaths::GameSavedDir() + TEXT("Screenshots/") + FPlatformProperties::PlatformName() + TEXT("/");
}

FString FPaths::BugItDir()
{
	return FPaths::GameSavedDir() + TEXT("BugIt/") + FPlatformProperties::PlatformName() + TEXT("/");
}

FString FPaths::VideoCaptureDir()
{
	return FPaths::GameSavedDir() + TEXT("VideoCaptures/");
}

FString FPaths::GameLogDir()
{
	return FPaths::GameSavedDir() + TEXT("Logs/");
}

FString FPaths::AutomationDir()
{
	return FPaths::GameSavedDir() + TEXT("Automation/");
}

FString FPaths::AutomationTransientDir()
{
	return FPaths::AutomationDir() + TEXT("Transient/");
}

FString FPaths::CloudDir()
{
	return FPaths::GameSavedDir() + TEXT("Cloud/");
}

FString FPaths::GameDevelopersDir()
{
	return FPaths::GameContentDir() + TEXT("Developers/");
}

FString FPaths::GameUserDeveloperDir()
{
	static FString UserFolder;

	if ( UserFolder.Len() == 0 )
	{
		// The user folder is the user name without any invalid characters
		const FString InvalidChars = INVALID_LONGPACKAGE_CHARACTERS;
		const FString& UserName = FPlatformProcess::UserName();
		
		UserFolder = UserName;
		
		for (int32 CharIdx = 0; CharIdx < InvalidChars.Len(); ++CharIdx)
		{
			const FString Char = InvalidChars.Mid(CharIdx, 1);
			UserFolder = UserFolder.Replace(*Char, TEXT("_"));
		}
	}

	return FPaths::GameDevelopersDir() + UserFolder + TEXT("/");
}

FString FPaths::DiffDir()
{
	return FPaths::GameSavedDir() + TEXT("Diff/");
}

const TArray<FString>& FPaths::GetEngineLocalizationPaths()
{
	static TArray<FString> Results;
	static bool HasInitialized = false;

	if(!HasInitialized)
	{
		if(GConfig && GConfig->IsReadyForUse())
		{
			GConfig->GetArray( TEXT("Internationalization"), TEXT("LocalizationPaths"), Results, GEngineIni );
			if(!Results.Num())
			{
				UE_LOG(LogInit, Warning, TEXT("We appear to have no engine localization data?"));
			}
			HasInitialized = true;
		}
		else
		{
			Results.AddUnique(TEXT("../../../Engine/Content/Localization/Engine")); // Hardcoded convention.
		}
	}

	return Results;
}

const TArray<FString>& FPaths::GetEditorLocalizationPaths()
{
	static TArray<FString> Results;
	static bool HasInitialized = false;

	if(!HasInitialized)
	{
		if(GConfig && GConfig->IsReadyForUse())
		{
			GConfig->GetArray( TEXT("Internationalization"), TEXT("LocalizationPaths"), Results, GEditorIni );
			if(!Results.Num())
			{
				UE_LOG(LogInit, Warning, TEXT("We appear to have no editor localization data? Editor can't run."));
			}
			HasInitialized = true;
		}
		else
		{
			Results.AddUnique(TEXT("../../../Engine/Content/Localization/Editor")); // Hardcoded convention.
		}
	}

	return Results;
}

const TArray<FString>& FPaths::GetGameLocalizationPaths()
{
	static TArray<FString> Results;
	static bool HasInitialized = false;

	if(!HasInitialized)
	{
		if(GConfig && GConfig->IsReadyForUse())
		{
			GConfig->GetArray( TEXT("Internationalization"), TEXT("LocalizationPaths"), Results, GGameIni );
			if(!Results.Num()) // Failed to find localization path.
			{
				UE_LOG(LogPaths, Warning, TEXT("We appear to have no game localization data? Game can't run."));
			}
			HasInitialized = true;
		}
	}


	return Results;
}

FString FPaths::GameAgnosticSavedDir()
{
	return EngineSavedDir();
}

FString FPaths::GameSourceDir()
{
	return FPaths::GameDir() + TEXT("Source/");
}

FString FPaths::StarterContentDir()
{
	return FPaths::RootDir() + TEXT("Samples/StarterContent/");
}

bool FPaths::IsProjectFilePathSet()
{
	FScopeLock Lock(GameProjectFilePathLock());
	return !GameProjectFilePath.IsEmpty();
}

const FString& FPaths::GetProjectFilePath()
{
	FScopeLock Lock(GameProjectFilePathLock());
	return GameProjectFilePath;
}

void FPaths::SetProjectFilePath( const FString& NewGameProjectFilePath )
{
	FScopeLock Lock(GameProjectFilePathLock());
	GameProjectFilePath = NewGameProjectFilePath;
	FPaths::NormalizeFilename(GameProjectFilePath);
}

FString FPaths::GetExtension( const FString& InPath, bool bIncludeDot )
{
	const FString Filename = GetCleanFilename(InPath);
	int32 DotPos = Filename.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (DotPos != INDEX_NONE)
	{
		return Filename.Mid(DotPos + (bIncludeDot ? 0 : 1));
	}

	return TEXT("");
}

FString FPaths::GetCleanFilename(const FString& InPath)
{
	int32 Pos = InPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	// in case we are using backslashes on a platform that doesn't use backslashes
	Pos = FMath::Max(Pos, InPath.Find(TEXT("\\"), ESearchCase::CaseSensitive, ESearchDir::FromEnd));

	if ( Pos != INDEX_NONE)
	{
		// if it was a trailing one, cut it (account for trailing whitespace?) and try removing path again
		if (Pos == InPath.Len() - 1)
		{
			return GetCleanFilename(InPath.Left(Pos));
		}

		return InPath.Mid(Pos + 1);
	}

	return InPath;
}

FString FPaths::GetBaseFilename( const FString& InPath, bool bRemovePath )
{
	FString Wk = bRemovePath ? GetCleanFilename(InPath) : InPath;

	// remove the extension
	int32 Pos = Wk.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if ( Pos != INDEX_NONE )
	{
		return Wk.Left(Pos);
	}

	return Wk;
}

FString FPaths::GetPath(const FString& InPath)
{
	int32 Pos = InPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	// in case we are using backslashes on a platform that doesn't use backslashes
	Pos = FMath::Max(Pos, InPath.Find(TEXT("\\"), ESearchCase::CaseSensitive, ESearchDir::FromEnd));
	if ( Pos != INDEX_NONE )
	{
		return InPath.Left(Pos);
	}

	return TEXT("");
}

bool FPaths::FileExists(const FString& InPath)
{
	return IFileManager::Get().GetTimeStamp(*InPath) > FDateTime::MinValue();
}

bool FPaths::IsDrive(const FString& InPath)
{
	FString ConvertedPathString = InPath;

	ConvertedPathString = ConvertedPathString.Replace(TEXT("/"), TEXT("\\"));
	const TCHAR* ConvertedPath= *ConvertedPathString;

	// Does Path refer to a drive letter or BNC path?
	if( FCString::Stricmp(ConvertedPath,TEXT(""))==0 )
	{
		return true;
	}
	else if( FChar::ToUpper(ConvertedPath[0])!=FChar::ToLower(ConvertedPath[0]) && ConvertedPath[1]==':' && ConvertedPath[2]==0 )
	{
		return true;
	}
	else if( FCString::Stricmp(ConvertedPath,TEXT("\\"))==0 )
	{
		return true;
	}
	else if( FCString::Stricmp(ConvertedPath,TEXT("\\\\"))==0 )
	{
		return true;
	}
	else if( ConvertedPath[0]=='\\' && ConvertedPath[1]=='\\' && !FCString::Strchr(ConvertedPath+2,'\\') )
	{
		return true;
	}
	else
	{
		// Need to handle cases such as X:\A\B\..\..\C\..
		// This assumes there is no actual filename in the path (ie, not c:\DIR\File.ext)!
		FString TempPath(ConvertedPath);
		// Make sure there is a '\' at the end of the path
		if (TempPath.Find(TEXT("\\"), ESearchCase::CaseSensitive, ESearchDir::FromEnd) != (TempPath.Len() - 1))
		{
			TempPath += TEXT("\\");
		}

		FString CheckPath = TEXT("");
		int32 ColonSlashIndex = TempPath.Find(TEXT(":\\"));
		if (ColonSlashIndex != INDEX_NONE)
		{
			// Remove the 'X:\' from the start
			CheckPath = TempPath.Right(TempPath.Len() - ColonSlashIndex - 2);
		}
		else
		{
			// See if the first two characters are '\\' to handle \\Server\Foo\Bar cases
			if (TempPath.StartsWith(TEXT("\\\\")) == true)
			{
				CheckPath = TempPath.Right(TempPath.Len() - 2);
				// Find the next slash
				int32 SlashIndex = CheckPath.Find(TEXT("\\"));
				if (SlashIndex != INDEX_NONE)
				{
					CheckPath = CheckPath.Right(CheckPath.Len() - SlashIndex  - 1);
				}
				else
				{
					CheckPath = TEXT("");
				}
			}
		}

		if (CheckPath.Len() > 0)
		{
			// Replace any remaining '\\' instances with '\'
			CheckPath.Replace(TEXT("\\\\"), TEXT("\\"));

			int32 CheckCount = 0;
			int32 SlashIndex = CheckPath.Find(TEXT("\\"));
			while (SlashIndex != INDEX_NONE)
			{
				FString FolderName = CheckPath.Left(SlashIndex);
				if (FolderName == TEXT(".."))
				{
					// It's a relative path, so subtract one from the count
					CheckCount--;
				}
				else
				{
					// It's a real folder, so add one to the count
					CheckCount++;
				}
				CheckPath = CheckPath.Right(CheckPath.Len() - SlashIndex  - 1);
				SlashIndex = CheckPath.Find(TEXT("\\"));
			}

			if (CheckCount <= 0)
			{
				// If there were the same number or greater relative to real folders, it's the root dir
				return true;
			}
		}
	}

	// It's not a drive...
	return false;
}

bool FPaths::IsRelative(const FString& InPath)
{
	return
		InPath.StartsWith( TEXT("./") ) ||
		InPath.StartsWith( TEXT(".\\") ) || 
		InPath.StartsWith( TEXT("../") ) ||
		InPath.StartsWith( TEXT("..\\") ) || 
		InPath.IsEmpty() ||
		!(InPath.Contains("/")
		|| InPath.Contains("\\"));
}

void FPaths::NormalizeFilename(FString& InPath)
{
	InPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	FPlatformMisc::NormalizePath(InPath);
}

void FPaths::NormalizeDirectoryName(FString& InPath)
{
	InPath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (InPath.EndsWith(TEXT("/")) && !InPath.EndsWith(TEXT("//")) && !InPath.EndsWith(TEXT(":/")))
	{
		// overwrite trailing slash with terminator
		InPath.GetCharArray()[InPath.Len() - 1] = 0;
		// shrink down
		InPath.TrimToNullTerminator();
	}

	FPlatformMisc::NormalizePath(InPath);
}

bool FPaths::CollapseRelativeDirectories(FString& InPath)
{
	const TCHAR ParentDir[] = TEXT("/..");
	const int32 ParentDirLength = ARRAY_COUNT( ParentDir ) - 1; // To avoid hardcoded values

	for (;;)
	{
		// An empty path is finished
		if (InPath.IsEmpty())
			break;

		// Consider empty paths or paths which start with .. or /.. as invalid
		if (InPath.StartsWith(TEXT("..")) || InPath.StartsWith(ParentDir))
			return false;

		// If there are no "/.."s left then we're done
		const int32 Index = InPath.Find(ParentDir);
		if (Index == -1)
			break;

		int32 PreviousSeparatorIndex = Index;
		for (;;)
		{
			// Find the previous slash
			PreviousSeparatorIndex = FMath::Max(0, InPath.Find( TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, PreviousSeparatorIndex - 1));

			// Stop if we've hit the start of the string
			if (PreviousSeparatorIndex == 0)
				break;

			// Stop if we've found a directory that isn't "/./"
			if ((Index - PreviousSeparatorIndex) > 1 && (InPath[PreviousSeparatorIndex + 1] != TEXT('.') || InPath[PreviousSeparatorIndex + 2] != TEXT('/')))
				break;
		}

		// If we're attempting to remove the drive letter, that's illegal
		int32 Colon = InPath.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart, PreviousSeparatorIndex);
		if (Colon >= 0 && Colon < Index)
			return false;

		InPath.RemoveAt(PreviousSeparatorIndex, Index - PreviousSeparatorIndex + ParentDirLength, false);
	}

	InPath.ReplaceInline(TEXT("./"), TEXT(""));

	InPath.TrimToNullTerminator();

	return true;
}

void FPaths::RemoveDuplicateSlashes(FString& InPath)
{
	while (InPath.Contains(TEXT("//")))
	{
		InPath = InPath.Replace(TEXT("//"), TEXT("/"));
	}
}

void FPaths::MakeStandardFilename(FString& InPath)
{
	// if this is an empty path, use the relative base dir
	if (InPath.Len() == 0)
	{
#if !PLATFORM_HTML5
		InPath = FPlatformProcess::BaseDir();
		FPaths::MakeStandardFilename(InPath);
#else
		// @todo: revisit this as needed
//		InPath = TEXT("/");
#endif
		return;
	}

	FString WithSlashes = InPath.Replace(TEXT("\\"), TEXT("/"));

	FString RootDirectory = FPaths::ConvertRelativePathToFull(FPaths::RootDir());

	// look for paths that cannot be made relative, and are therefore left alone
	// UNC (windows) network path
	bool bCannotBeStandardized = InPath.StartsWith(TEXT("\\\\"));
	// windows drive letter path that doesn't start with base dir
	bCannotBeStandardized |= (InPath[1] == ':' && !WithSlashes.StartsWith(RootDirectory));
	// Unix style absolute path that doesn't start with base dir
	bCannotBeStandardized |= (WithSlashes.GetCharArray()[0] == '/' && !WithSlashes.StartsWith(RootDirectory));

	// if it can't be standardized, just return itself
	if (bCannotBeStandardized)
	{
		return;
	}

	// make an absolute path
	
	FString Standardized = FPaths::ConvertRelativePathToFull(InPath);

	// fix separators and remove any trailing slashes
	FPaths::NormalizeDirectoryName(Standardized);
	
	// remove inner ..'s
	FPaths::CollapseRelativeDirectories(Standardized);

	// remove duplicate slashes
	FPaths::RemoveDuplicateSlashes(Standardized);

	// make it relative to Engine\Binaries\Platform
	InPath = Standardized.Replace(*RootDirectory, *FPaths::GetRelativePathToRoot());;
}

bool FPaths::MakePathRelativeTo( FString& InPath, const TCHAR* InRelativeTo )
{
	FString Target = FPaths::ConvertRelativePathToFull(InPath);
	FString Source = FPaths::ConvertRelativePathToFull(InRelativeTo);
	
	Source = FPaths::GetPath(Source);
	Source.ReplaceInline(TEXT("\\"), TEXT("/"));
	Target.ReplaceInline(TEXT("\\"), TEXT("/"));

	TArray<FString> TargetArray;
	Target.ParseIntoArray(&TargetArray, TEXT("/"), true);
	TArray<FString> SourceArray;
	Source.ParseIntoArray(&SourceArray, TEXT("/"), true);

	if (TargetArray.Num() && SourceArray.Num())
	{
		// Check for being on different drives
		if ((TargetArray[0][1] == TEXT(':')) && (SourceArray[0][1] == TEXT(':')))
		{
			if (FChar::ToUpper(TargetArray[0][0]) != FChar::ToUpper(SourceArray[0][0]))
			{
				// The Target and Source are on different drives... No relative path available.
				return false;
			}
		}
	}

	while (TargetArray.Num() && SourceArray.Num() && TargetArray[0] == SourceArray[0])
	{
		TargetArray.RemoveAt(0);
		SourceArray.RemoveAt(0);
	}
	FString Result;
	for (int32 Index = 0; Index < SourceArray.Num(); Index++)
	{
		Result += TEXT("../");
	}
	for (int32 Index = 0; Index < TargetArray.Num(); Index++)
	{
		Result += TargetArray[Index];
		if (Index + 1 < TargetArray.Num())
		{
			Result += TEXT("/");
		}
	}
	
	InPath = Result;
	return true;
}

FString FPaths::ConvertRelativePathToFull(const FString& InPath)
{
	return FPaths::ConvertRelativePathToFull(FString(FPlatformProcess::BaseDir()), InPath);
}

FString FPaths::ConvertRelativePathToFull(const FString& BasePath, const FString& InPath)
{
	FString FullyPathed;
	if ( FPaths::IsRelative(InPath) )
	{
		FullyPathed = BasePath;
	}
	
	FullyPathed /= InPath;

	FPaths::NormalizeFilename(FullyPathed);
	FPaths::CollapseRelativeDirectories(FullyPathed);
	return FullyPathed;
}

FString FPaths::ConvertToSandboxPath( const FString& InPath, const TCHAR* InSandboxName )
{
	FString SandboxDirectory = FPaths::SandboxesDir() / InSandboxName;
	FPaths::NormalizeFilename(SandboxDirectory);
	
	FString RootDirectory = FPaths::RootDir();
	FPaths::CollapseRelativeDirectories(RootDirectory);
	FPaths::NormalizeFilename(RootDirectory);

	FString SandboxPath = FPaths::ConvertRelativePathToFull(InPath);
	if (!SandboxPath.StartsWith(RootDirectory))
	{
		UE_LOG(LogInit, Fatal, TEXT("%s does not start with %s so this is not a valid sandbox path."), *SandboxPath, *RootDirectory);
	}
	check(SandboxPath.StartsWith(RootDirectory));
	SandboxPath.ReplaceInline(*RootDirectory, *SandboxDirectory);

	return SandboxPath;
}

FString FPaths::ConvertFromSandboxPath( const FString& InPath, const TCHAR* InSandboxName )
{
	FString SandboxDirectory =  FPaths::SandboxesDir() / InSandboxName;
	FPaths::NormalizeFilename(SandboxDirectory);
	FString RootDirectory = FPaths::RootDir();
	
	FString SandboxPath(InPath);
	check(SandboxPath.StartsWith(SandboxDirectory));
	SandboxPath.ReplaceInline(*SandboxDirectory, *RootDirectory);
	return SandboxPath;
}

FString FPaths::CreateTempFilename( const TCHAR* Path, const TCHAR* Prefix, const TCHAR* Extension )
{
	FString UniqueFilename;
	do
	{
		const int32 PathLen = FCString::Strlen( Path );
		if( PathLen > 0 && Path[ PathLen - 1 ] != TEXT('/') )
		{
			UniqueFilename = FString::Printf( TEXT("%s/%s%s%s"), Path, Prefix, *FGuid::NewGuid().ToString(), Extension );
		}
		else
		{
			UniqueFilename = FString::Printf( TEXT("%s/%s%s%s"), Path, Prefix, *FGuid::NewGuid().ToString(), Extension );
		}
	}
	while( IFileManager::Get().FileSize( *UniqueFilename ) >= 0 );
	
	return UniqueFilename;
}

void FPaths::Split( const FString& InPath, FString& PathPart, FString& FilenamePart, FString& ExtensionPart )
{
	PathPart = GetPath(InPath);
	FilenamePart = GetBaseFilename(InPath);
	ExtensionPart = GetExtension(InPath);
}

const FString& FPaths::GetRelativePathToRoot()
{
	static FString RelativePathToRoot;

	// initialize static paths if needed
	if (RelativePathToRoot.Len() == 0)
	{
		FString RootDirectory = FPaths::RootDir();
		FString BaseDirectory = FPlatformProcess::BaseDir();

		// this is how to go from the base dir back to the root
		RelativePathToRoot = RootDirectory;
		FPaths::MakePathRelativeTo(RelativePathToRoot, *BaseDirectory);

		// Ensure that the path ends w/ '/'
		if ((RelativePathToRoot.Len() > 0) && (RelativePathToRoot.EndsWith(TEXT("/")) == false) && (RelativePathToRoot.EndsWith(TEXT("\\")) == false))
		{
			RelativePathToRoot += TEXT("/");
		}
	}
	return RelativePathToRoot;
}

void FPaths::CombineInternal(FString& OutPath, const TCHAR** Pathes, int32 NumPathes)
{
	check(Pathes != NULL && NumPathes > 0);

	int32 OutStringSize = 0;

	for (int32 i=0; i < NumPathes; ++i)
	{
		OutStringSize += FCString::Strlen(Pathes[i]) + 1;
	}

	OutPath.Empty(OutStringSize);
	OutPath += Pathes[0];
	
	for (int32 i=1; i < NumPathes; ++i)
	{
		OutPath /= Pathes[i];
	}
}
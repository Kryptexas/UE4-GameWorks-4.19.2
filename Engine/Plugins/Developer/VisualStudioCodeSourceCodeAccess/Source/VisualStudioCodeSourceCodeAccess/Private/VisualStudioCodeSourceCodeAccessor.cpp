// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VisualStudioCodeSourceCodeAccessor.h"
#include "VisualStudioCodeSourceCodeAccessModule.h"
#include "ISourceCodeAccessModule.h"
#include "ModuleManager.h"
#include "DesktopPlatformModule.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY_STATIC(LogVSCodeAccessor, Log, All);

#define LOCTEXT_NAMESPACE "VisualStudioCodeSourceCodeAccessor"

FString FVisualStudioCodeSourceCodeAccessor::GetSolutionPath() const
{
	FScopeLock Lock(&CachedSolutionPathCriticalSection);
	if (IsInGameThread())
	{
		FString SolutionPath;
		if (FDesktopPlatformModule::Get()->GetSolutionPath(SolutionPath))
		{
			CachedSolutionPath = FPaths::ConvertRelativePathToFull(SolutionPath);
		}
	}
	return CachedSolutionPath;
}

/** save all open documents in visual studio, when recompiling */
void OnModuleCompileStarted(bool bIsAsyncCompile)
{
	FVisualStudioCodeSourceCodeAccessModule& VisualStudioCodeSourceCodeAccessModule = FModuleManager::LoadModuleChecked<FVisualStudioCodeSourceCodeAccessModule>(TEXT("VisualStudioCodeSourceCodeAccess"));
	VisualStudioCodeSourceCodeAccessModule.GetAccessor().SaveAllOpenDocuments();
}

void FVisualStudioCodeSourceCodeAccessor::Startup()
{
	GetSolutionPath();
	RefreshAvailability();
}

void FVisualStudioCodeSourceCodeAccessor::RefreshAvailability()
{
	FString IDEPath;
#if PLATFORM_WINDOWS
	if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command\\"), TEXT(""), IDEPath))
	{
		FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command\\"), TEXT(""), IDEPath);
	}

	FString PatternString(TEXT("\"(.*)\" \".*\""));
	FRegexPattern Pattern(PatternString);
	FRegexMatcher Matcher(Pattern, IDEPath);
	if (Matcher.FindNext())
	{
		IDEPath = Matcher.GetCaptureGroup(1);
	}
#endif

	if (IDEPath.Len() > 0 && FPaths::FileExists(IDEPath))
	{
		Location = IDEPath;
	}
	else
	{
		Location = TEXT("");
	}
}

void FVisualStudioCodeSourceCodeAccessor::Shutdown()
{
}

bool FVisualStudioCodeSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths)
{
	if (Location.Len() > 0)
	{
		FString Args;

		for (const FString& SourcePath : AbsoluteSourcePaths)
		{
			Args += TEXT("\"") + SourcePath + TEXT("\" ");
		}

		return FPlatformProcess::ExecProcess(*Location, *Args, nullptr, nullptr, nullptr);
	}

	return false;
}

bool FVisualStudioCodeSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
	return false;
}

bool FVisualStudioCodeSourceCodeAccessor::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	return false;
}

bool FVisualStudioCodeSourceCodeAccessor::CanAccessSourceCode() const
{
	// True if we have any versions of VS installed
	return Location.Len() > 0;
}

FName FVisualStudioCodeSourceCodeAccessor::GetFName() const
{
	return FName("VisualStudioCodeSourceCodeAccessor");
}

FText FVisualStudioCodeSourceCodeAccessor::GetNameText() const
{
	return LOCTEXT("VisualStudioDisplayName", "Visual Studio Code");
}

FText FVisualStudioCodeSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("VisualStudioDisplayDesc", "Open source code files in Visual Studio Code");
}

void FVisualStudioCodeSourceCodeAccessor::Tick(const float DeltaTime)
{
}

bool FVisualStudioCodeSourceCodeAccessor::OpenSolution()
{
	if (Location.Len() > 0)
	{
		FString SolutionDir = FPaths::GetPath(GetSolutionPath());
		uint32 ProcessID;
		FProcHandle hProcess = FPlatformProcess::CreateProc(*Location, *SolutionDir, true, false, false, &ProcessID, 0, nullptr, nullptr, nullptr);
		return hProcess.IsValid();
	}

	return false;
}

bool FVisualStudioCodeSourceCodeAccessor::SaveAllOpenDocuments() const
{
	return false;
}


#undef LOCTEXT_NAMESPACE
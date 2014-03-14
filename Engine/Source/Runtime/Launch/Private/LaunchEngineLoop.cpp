// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LaunchEngineLoop.cpp: Implements the main engine loop.
=============================================================================*/

#include "LaunchPrivatePCH.h"
#include "Internationalization/Internationalization.h"
#include "Ticker.h"
#include "ConsoleManager.h"
#include "ExceptionHandling.h"
#include "FileManagerGeneric.h"
#include "TaskGraphInterfaces.h"

#include "Projects.h"
#include "UProjectInfo.h"
#include "EngineVersion.h"

#if WITH_EDITOR
	#include "EditorStyle.h"
	#include "AutomationController.h"
	#include "ProfilerClient.h"
	#include "RemoteConfigIni.h"

	#if PLATFORM_WINDOWS
		#include "AllowWindowsPlatformTypes.h"
			#include <objbase.h>
		#include "HideWindowsPlatformTypes.h"
	#endif
#endif

#if WITH_ENGINE
	#include "Database.h"
	#include "DerivedDataCacheInterface.h"
	#include "RenderCore.h"
	#include "ShaderCompiler.h"
	#include "GlobalShader.h"
	#include "TaskGraphInterfaces.h"
	#include "ParticleHelper.h"
	#include "Online.h"
	#include "PlatformFeatures.h"
#if !UE_SERVER
	#include "SlateRHIRendererModule.h"
#endif

	#include "MoviePlayer.h"

#if !UE_BUILD_SHIPPING
	#include "STaskGraph.h"
	#include "ProfilerService.h"
#endif

#if WITH_AUTOMATION_WORKER
	#include "AutomationWorker.h"
#endif

	/** 
	 *	Function to free up the resources in GPrevPerBoneMotionBlur
	 *	Should only be called at application exit
	 */
	ENGINE_API void MotionBlur_Free();

#endif  //WITH_ENGINE

#if WITH_EDITOR
	#include "FeedbackContextEditor.h"
	static FFeedbackContextEditor UnrealEdWarn;
#endif	// WITH_EDITOR

#define LOCTEXT_NAMESPACE "LaunchEngineLoop"

#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
	#include <ObjBase.h>
	#include "HideWindowsPlatformTypes.h"
#endif

// Pipe output to std output
// This enables UBT to collect the output for it's own use
class FOutputDeviceStdOutput : public FOutputDevice
{
public:

	FOutputDeviceStdOutput()
	{
		bAllowLogVerbosity = FParse::Param(FCommandLine::Get(), TEXT("AllowStdOutLogVerbosity"));
	}

	virtual ~FOutputDeviceStdOutput()
	{
	}

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE
	{
		if ( (bAllowLogVerbosity && Verbosity <= ELogVerbosity::Log) || (Verbosity <= ELogVerbosity::Display) )
		{
#if PLATFORM_USE_LS_SPEC_FOR_WIDECHAR
			printf("\n%ls", *FOutputDevice::FormatLogLine(Verbosity, Category, V, GPrintLogTimes));
#else
			wprintf(TEXT("\n%s"), *FOutputDevice::FormatLogLine(Verbosity, Category, V, GPrintLogTimes));
#endif
			fflush(stdout);
		}
	}

private:
	bool bAllowLogVerbosity;
};

static TScopedPointer<FOutputDeviceConsole>	GScopedLogConsole;
static TScopedPointer<FOutputDeviceStdOutput> GScopedStdOut;

bool ParseGameProjectFromCommandLine(const TCHAR* InCmdLine, FString& OutProjectFilePath, FString& OutGameName)
{
	const TCHAR *CmdLine = InCmdLine;
	FString FirstCommandLineToken = FParse::Token(CmdLine, 0);

	// trim any whitespace at edges of string - this can happen if the token was quoted with leading or trailing whitespace
	// VC++ tends to do this in its "external tools" config
	FirstCommandLineToken = FirstCommandLineToken.Trim();

	// 
	OutProjectFilePath = TEXT("");
	OutGameName = TEXT("");

	if ( FirstCommandLineToken.Len() && !FirstCommandLineToken.StartsWith(TEXT("-")) )
	{
		// The first command line argument could be the project file if it exists or the game name if not launching with a project file
		const FString ProjectFilePath = FString(FirstCommandLineToken);
		if ( FPaths::GetExtension(ProjectFilePath) == IProjectManager::GetProjectFileExtension() )
		{
			OutProjectFilePath = FirstCommandLineToken;
			// Here we derive the game name from the project file
			OutGameName = FPaths::GetBaseFilename(OutProjectFilePath);
			return true;
		}
		else if (FPlatformProperties::IsMonolithicBuild() == false)
		{
			// Full game name is assumed to be the first token
			OutGameName = MoveTemp(FirstCommandLineToken);
			// Derive the project path from the game name. All games must have a uproject file, even if they are in the root folder.
			OutProjectFilePath = FPaths::Combine(*FPaths::RootDir(), *OutGameName, *FString(OutGameName + TEXT(".") + IProjectManager::GetProjectFileExtension()));
			return true;
		}
	}
	return false;
}

bool LaunchSetGameName(const TCHAR *InCmdLine)
{
	if (GIsGameAgnosticExe)
	{
		// Initialize GGameName to an empty string. Populate it below.
		GGameName[0] = 0;

		FString ProjFilePath;
		FString LocalGameName;
		if (ParseGameProjectFromCommandLine(InCmdLine, ProjFilePath, LocalGameName) == true)
		{
			// Only set the game name if this is NOT a program...
			if (FPlatformProperties::IsProgram() == false)
			{
				FCString::Strncpy(GGameName, *LocalGameName, ARRAY_COUNT(GGameName));
			}
			FPaths::SetProjectFilePath(ProjFilePath);
		}
#if UE_GAME
		else
		{
			// Try to use the executable name as the game name.
			LocalGameName = FPlatformProcess::ExecutableName();
			FCString::Strncpy(GGameName, *LocalGameName, ARRAY_COUNT(GGameName));

			// Check it's not UE4Game, otherwise assume a uproject file relative to the game project directory
			if (LocalGameName != TEXT("UE4Game"))
			{
				ProjFilePath = FPaths::Combine(TEXT(".."), TEXT(".."), TEXT(".."), *LocalGameName, *FString(LocalGameName + TEXT(".") + IProjectManager::GetProjectFileExtension()));
				FPaths::SetProjectFilePath(ProjFilePath);
			}
		}
#endif

		static bool bPrinted = false;
		if (!bPrinted)
		{
			bPrinted = true;
			if (FApp::HasGameName())
			{
				UE_LOG(LogInit, Display, TEXT("Running engine for game: %s"), GGameName);
			}
			else
			{
				if (FPlatformProperties::RequiresCookedData())
				{
					UE_LOG(LogInit, Fatal, TEXT("Non-agnostic games on cooked platforms require a uproject file be specified."));
				}
				else
				{
					UE_LOG(LogInit, Display, TEXT("Running engine without a game"));
				}
			}
		}
	}
	else
	{
		FString ProjFilePath;
		FString LocalGameName;
		if (ParseGameProjectFromCommandLine(InCmdLine, ProjFilePath, LocalGameName) == true)
		{
			if (FPlatformProperties::RequiresCookedData())
			{
				// Non-agnostic exes that require cooked data cannot load projects, so make sure that the LocalGameName is the GGameName
				if (LocalGameName != GGameName)
				{
					UE_LOG(LogInit, Fatal, TEXT("Non-agnostic games cannot load projects on cooked platforms - try running UE4Game."));
				}
			}
			// Only set the game name if this is NOT a program...
			if (FPlatformProperties::IsProgram() == false)
			{
				FCString::Strncpy(GGameName, *LocalGameName, ARRAY_COUNT(GGameName));
			}
			FPaths::SetProjectFilePath(ProjFilePath);
		}

		// In a non-game agnostic exe, the game name should already be assigned by now.
		if (!FApp::HasGameName())
		{
			UE_LOG(LogInit, Fatal,TEXT("Could not set game name!"));
		}
	}

	return true;
}


static IPlatformFile* ConditionallyCreateFileWrapper(const TCHAR* Name, IPlatformFile* CurrentPlatformFile, const TCHAR* CommandLine, bool* OutFailedToInitialize = NULL)
{
	if (OutFailedToInitialize)
	{
		*OutFailedToInitialize = false;
	}
	IPlatformFile* WrapperFile = FPlatformFileManager::Get().GetPlatformFile(Name);
	if (WrapperFile != NULL && WrapperFile->ShouldBeUsed(CurrentPlatformFile, CommandLine))
	{
		if (WrapperFile->Initialize(CurrentPlatformFile, CommandLine) == false)
		{
			if (OutFailedToInitialize)
			{
				*OutFailedToInitialize = true;
			}
			// Don't delete the platform file. It will be automatically deleted by its module.
			WrapperFile = NULL;
		}
	}
	else
	{
		// Make sure it won't be used.
		WrapperFile = NULL;
	}
	return WrapperFile;
}

/**
 * Look for any file overrides on the command line (i.e. network connection file handler)
 */
bool LaunchCheckForFileOverride(const TCHAR* CmdLine, bool& OutFileOverrideFound)
{
	OutFileOverrideFound = false;

	// Get the physical platform file.
	IPlatformFile* CurrentPlatformFile = &FPlatformFileManager::Get().GetPlatformFile();

	// Try to create pak file wrapper
	{
		IPlatformFile* PlatformFile = ConditionallyCreateFileWrapper(TEXT("PakFile"), CurrentPlatformFile, CmdLine);
		if (PlatformFile)
		{
			CurrentPlatformFile = PlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}
	}

	// Try to create sandbox wrapper
	{
		IPlatformFile* PlatformFile = ConditionallyCreateFileWrapper(TEXT("SandboxFile"), CurrentPlatformFile, CmdLine);
		if (PlatformFile)
		{
			CurrentPlatformFile = PlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}
	}
	
#if !UE_BUILD_SHIPPING // UFS clients are not available in shipping builds.	
	// Streaming network wrapper (it has a priority over normal network wrapper)
	bool bNetworkFailedToInitialize = false;
	do
	{
		IPlatformFile* NetworkPlatformFile = ConditionallyCreateFileWrapper(TEXT("StreamingFile"), CurrentPlatformFile, CmdLine, &bNetworkFailedToInitialize);
		if (NetworkPlatformFile)
		{
			CurrentPlatformFile = NetworkPlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}

		// Network file wrapper (only create if the streaming wrapper hasn't been created)
		if (!NetworkPlatformFile)
		{
			NetworkPlatformFile = ConditionallyCreateFileWrapper(TEXT("NetworkFile"), CurrentPlatformFile, CmdLine, &bNetworkFailedToInitialize);
			if (NetworkPlatformFile)
			{
				CurrentPlatformFile = NetworkPlatformFile;
				FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
			}
		}

		if (bNetworkFailedToInitialize)
		{
			FString HostIpString;
			FParse::Value(CmdLine, TEXT("-FileHostIP="), HostIpString);
#if PLATFORM_REQUIRES_FILESERVER
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Failed to connect to file server at %s. EXITING.\n"), *HostIpString);
			uint32 Result = 2;
#else	//PLATFORM_REQUIRES_FILESERVER
			uint32 Result = FMessageDialog::Open( EAppMsgType::YesNoCancel, FText::Format( NSLOCTEXT("Engine", "FailedToConnectToServer", "Failed to connect to any of the following file servers:\n\n    {0}\n\nWould you like to try again? No will fallback to local disk files, Cancel will quit."), FText::FromString( HostIpString.Replace( TEXT("+"), TEXT("\n    ") ) ) ) );
#endif	//PLATFORM_REQUIRES_FILESERVER

			if (Result == EAppReturnType::No)
			{
				break;
			}
			else if (Result == EAppReturnType::Cancel)
			{
				// Cancel - return a failure, and quit
				return false;
			}
		}
	}
	while (bNetworkFailedToInitialize);
#endif

#if !UE_BUILD_SHIPPING
	// Try to create file profiling wrapper
	{
		IPlatformFile* PlatformFile = ConditionallyCreateFileWrapper(TEXT("ProfileFile"), CurrentPlatformFile, CmdLine);
		if (PlatformFile)
		{
			CurrentPlatformFile = PlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}
	}
	{
		IPlatformFile* PlatformFile = ConditionallyCreateFileWrapper(TEXT("SimpleProfileFile"), CurrentPlatformFile, CmdLine);
		if (PlatformFile)
		{
			CurrentPlatformFile = PlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}
	}
	{
		IPlatformFile* PlatformFile = ConditionallyCreateFileWrapper(TEXT("FileReadStats"), CurrentPlatformFile, CmdLine);
		if (PlatformFile)
		{
			CurrentPlatformFile = PlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}
	}
#endif	//#if !UE_BUILD_SHIPPING

	// Wrap the above in a file logging singleton if requested
	{
		IPlatformFile* PlatformFile = ConditionallyCreateFileWrapper(TEXT("LogFile"), CurrentPlatformFile, CmdLine);
		if (PlatformFile)
		{
			CurrentPlatformFile = PlatformFile;
			FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);
		}
	}

	// If our platform file is different than it was when we started, then an override was used
	OutFileOverrideFound = (CurrentPlatformFile != &FPlatformFileManager::Get().GetPlatformFile());

	return true;
}

bool LaunchHasIncompleteGameName()
{
	if ( FApp::HasGameName() && !FPaths::IsProjectFilePathSet() )
	{
		// Verify this is a legitimate game name
		// Launched with a game name. See if the <GameName> folder exists. If it doesn't, it could instead be <GameName>Game
		const FString NonSuffixedGameFolder = FPaths::RootDir() / GGameName;
		if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*NonSuffixedGameFolder) == false)
		{
			const FString SuffixedGameFolder = NonSuffixedGameFolder + TEXT("Game");
			if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*SuffixedGameFolder))
			{
				return true;
			}
		}
	}

	return false;
}

void LaunchUpdateMostRecentProjectFile()
{
	// If we are launching without a game name or project file, we should use the last used project file, if it exists
	const FString& AutoLoadProjectFileName = IProjectManager::Get().GetAutoLoadProjectFileName();
	FString RecentProjectFileContents;
	if ( FFileHelper::LoadFileToString(RecentProjectFileContents, *AutoLoadProjectFileName) )
	{
		if ( RecentProjectFileContents.Len() )
		{
			const FString AutoLoadInProgressFilename = AutoLoadProjectFileName + TEXT(".InProgress");
			if ( FPlatformFileManager::Get().GetPlatformFile().FileExists(*AutoLoadInProgressFilename) )
			{
				// We attempted to auto-load a project but the last run did not make it to UEditorEngine::InitEditor.
				// This indicates that there was a problem loading the project.
				// Do not auto-load the project, instead load normally until the next time the editor starts successfully.
				UE_LOG(LogInit, Display, TEXT("There was a problem auto-loading %s. Auto-load will be disabled until the editor successfully starts up with a project."), *RecentProjectFileContents);
			}
			else if ( FPlatformFileManager::Get().GetPlatformFile().FileExists(*RecentProjectFileContents) )
			{
				// The previously loaded project file was found. Change the game name here and update the project file path
				FCString::Strncpy(GGameName, *FPaths::GetBaseFilename(RecentProjectFileContents), ARRAY_COUNT(GGameName));
				FPaths::SetProjectFilePath(RecentProjectFileContents);
				UE_LOG(LogInit, Display, TEXT("Loading recent project file: %s"), *RecentProjectFileContents);

				// Write a file indicating that we are trying to auto-load a project.
				// This file prevents auto-loading of projects for as long as it exists. It is a detection system for failed auto-loads.
				// The file is deleted in UEditorEngine::InitEditor, thus if the load does not make it that far then the project will not be loaded again.
				FFileHelper::SaveStringToFile(TEXT(""), *AutoLoadInProgressFilename);
			}
		}
	}
}


/*-----------------------------------------------------------------------------
	FEngineLoop implementation.
-----------------------------------------------------------------------------*/

FEngineLoop::FEngineLoop()
#if WITH_ENGINE
	: EngineService(NULL)
#endif
{ }


int32 FEngineLoop::PreInit(int32 ArgC, ANSICHAR* ArgV[], const TCHAR* AdditionalCommandline)
{
	FString CmdLine;

	// loop over the parameters, skipping the first one (which is the executable name)
	for (int32 Arg = 1; Arg < ArgC; Arg++)
	{
		FString ThisArg = ArgV[Arg];
		if (ThisArg.Contains(TEXT(" ")) && !ThisArg.Contains(TEXT("\"")))
		{
			int32 EqualsAt = ThisArg.Find(TEXT("="));
			if (EqualsAt > 0 && ThisArg.Find(TEXT(" ")) > EqualsAt)
			{
				ThisArg = ThisArg.Left(EqualsAt + 1) + FString("\"") + ThisArg.RightChop(EqualsAt + 1) + FString("\"");

			}
			else
			{
				ThisArg = FString("\"") + ThisArg + FString("\"");
			}
		}

		CmdLine += ThisArg;
		// put a space between each argument (not needed after the end)
		if (Arg + 1 < ArgC)
		{
			CmdLine += TEXT(" ");
		}
	}

	// append the additional extra command line
	if (AdditionalCommandline)
	{
		CmdLine += TEXT(" ");
		CmdLine += AdditionalCommandline;
	}

	// send the command line without the exe name
	return GEngineLoop.PreInit(*CmdLine);
}

#if WITH_ENGINE
bool IsServerDelegateForOSS()
{
	return IsRunningDedicatedServer() || (GEngine && (GEngine->GetNetModeForOnlineSubsystems() == NM_DedicatedServer || GEngine->GetNetModeForOnlineSubsystems() == NM_ListenServer));
}
#endif

int32 FEngineLoop::PreInit( const TCHAR* CmdLine )
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Engine Pre-Initialized"), STAT_PreInit, STATGROUP_LoadTime);

	// Switch into executable's directory.
	FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();

#if WITH_ENGINE
	extern ENGINE_API void InitializeRenderingCVarsCaching();
	InitializeRenderingCVarsCaching();
#endif

	// Initialize log console here to avoid statics initialization issues when launched from the command line.
	GScopedLogConsole = FPlatformOutputDevices::GetLogConsole();

#if !UE_BUILD_SHIPPING
	// Check for the '-' that normal ones get converted to in Outlook
	if (StringHasBadDashes(CmdLine))
	{
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format( NSLOCTEXT("Engine", "ComdLineHasInvalidChar", "Error: Command-line contains an invalid '-' character, likely pasted from an email.\nCmdline = {0}"), FText::FromString( CmdLine ) ) );
		return -1;
	}
#endif

	// Always enable the backlog so we get all messages, we will disable and clear it in the game
	// as soon as we determine whether GIsEditor == false
	GLog->EnableBacklog(true);

	// this is set later with shorter command lines, but we want to make sure it is set ASAP as some subsystems will do the tests themselves...
	// also realize that command lines can be pulled from the network at a slightly later time.
	FCommandLine::Set(CmdLine); 

	// Set GGameName, based on the command line
	if (LaunchSetGameName(CmdLine) == false)
	{
		// If it failed, do not continue
		return 1;
	}

	// Switch into executable's directory (may be required by some of the platform file overrides)
	FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();

	// allow the command line to override the platform file singleton
	bool bFileOverrideFound = false;
	if (LaunchCheckForFileOverride(CmdLine, bFileOverrideFound) == false)
	{
		// if it failed, we cannot continue
		return 1;
	}

	// Initialize file manager
	IFileManager::Get().ProcessCommandLineOptions();

	if (FPlatformProperties::IsProgram() == false)
	{
		if (FPaths::IsProjectFilePathSet())
		{
			FString ProjPath = FPaths::GetProjectFilePath();
			if (FPaths::FileExists(ProjPath) == false)
			{
				UE_LOG(LogInit, Display, TEXT("Project file not found: %s"), *ProjPath);
				UE_LOG(LogInit, Display, TEXT("\tAttempting to find via project info helper."));
				// Use the uprojectdirs
				FUProjectInfoHelper::FillProjectInfo();
				FString GameProjectFile = FUProjectInfoHelper::GetProjectForGame(GGameName);
				if (GameProjectFile.IsEmpty() == false)
				{
					UE_LOG(LogInit, Display, TEXT("\tFound project file %s."), *GameProjectFile);
					FPaths::SetProjectFilePath(GameProjectFile);
				}
			}
		}
	}

	if( GIsGameAgnosticExe )
	{
		// If we launched without a project file, but with a game name that is incomplete, warn about the improper use of a Game suffix
		if ( LaunchHasIncompleteGameName() )
		{
			// We did not find a non-suffixed folder and we DID find the suffixed one.
			// The engine MUST be launched with <GameName>Game.
			const FText GameNameText = FText::FromString( GGameName );
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format( LOCTEXT("RequiresGamePrefix", "Error: UE4Editor does not append 'Game' to the passed in game name.\nYou must use the full name.\nYou specified '{0}', use '{0}Game'."), GameNameText ) );
			return 1;
		}
	}

	// remember thread id of the main thread
	GGameThreadId = FPlatformTLS::GetCurrentThreadId();
	GIsGameThreadIdInitialized = true;

#if PLATFORM_XBOXONE
	HANDLE proc = ::GetCurrentProcess();
	DWORD64 dwProcessAffinity, dwSystemAffinity;
	if (GetProcessAffinityMask( proc, &dwProcessAffinity, &dwSystemAffinity))
	{
		UE_LOG(LogInit, Log, TEXT("Process affinity %ld System affinity %ld."), dwProcessAffinity , dwSystemAffinity);
	}
	uint64 GameThreadAffinity = AffinityManagerGetAffinity( TEXT("MainGame"));
	::SetThreadAffinityMask(::GetCurrentThread(), (DWORD_PTR)GameThreadAffinity);
	UE_LOG(LogInit, Log, TEXT("Runnable thread Main Thread is on Process %d."), static_cast<uint32>(::GetCurrentProcessorNumber()) );	
#endif // PLATFORM_XBOXONE

	// Figure out whether we're the editor, ucc or the game.
	const SIZE_T CommandLineSize = FCString::Strlen(CmdLine)+1;
	TCHAR* CommandLineCopy			= new TCHAR[ CommandLineSize ];
	FCString::Strcpy( CommandLineCopy, CommandLineSize, CmdLine );
	const TCHAR* ParsedCmdLine	= CommandLineCopy;
	FString Token				= FParse::Token( ParsedCmdLine, 0);

	// trim any whitespace at edges of string - this can happen if the token was quoted with leading or trailing whitespace
	// VC++ tends to do this in its "external tools" config
	Token = Token.Trim();

	const bool bFirstTokenIsGameName = (FApp::HasGameName() && Token == GGameName);
	const bool bFirstTokenIsGameProjectFilePath = (FPaths::IsProjectFilePathSet() && Token.Replace(TEXT("\\"), TEXT("/")) == FPaths::GetProjectFilePath());

	if (bFirstTokenIsGameName || bFirstTokenIsGameProjectFilePath)
	{
		// first item on command line was the game name, remove it in all cases
		FString RemainingCommandline = ParsedCmdLine;
		FCString::Strcpy( CommandLineCopy, CommandLineSize, *RemainingCommandline );
		ParsedCmdLine	= CommandLineCopy;

		// Set a new command-line that doesn't include the game name as the first argument
		FCommandLine::Set(ParsedCmdLine); 

		Token = FParse::Token( ParsedCmdLine, 0);
		Token = Token.Trim();

		if (bFirstTokenIsGameProjectFilePath)
		{
			// Convert it to relative if possible...
			FString RelativeGameProjectFilePath = FFileManagerGeneric::DefaultConvertToRelativePath(*FPaths::GetProjectFilePath());
			if (RelativeGameProjectFilePath != FPaths::GetProjectFilePath())
			{
				FPaths::SetProjectFilePath(RelativeGameProjectFilePath);
			}
		}
	}

	// look early for the editor token
	bool bHasEditorToken = false;

#if UE_EDITOR
	// Check each token for '-game', '-server' or '-run='
	bool bIsNotEditor = false;

	// This isn't necessarily pretty, but many requests have been made to allow
	//   UE4Editor.exe <GAMENAME> -game <map>
	// or
	//   UE4Editor.exe <GAMENAME> -game 127.0.0.0
	// We don't want to remove the -game from the commandline just yet in case 
	// we need it for something later. So, just move it to the end for now...
	const bool bFirstTokenIsGame = (Token == TEXT("-GAME"));
	const bool bFirstTokenIsServer = (Token == TEXT("-SERVER"));
	const bool bFirstTokenIsCommandlet = Token.StartsWith(TEXT("-RUN=")) || Token.EndsWith(TEXT("Commandlet"));
	const bool bFirstTokenIsModeOverride = bFirstTokenIsGame || bFirstTokenIsServer || bFirstTokenIsCommandlet;
	const TCHAR* CommandletCommandLine = NULL;
	if (bFirstTokenIsModeOverride)
	{
		bIsNotEditor = true;
		if (bFirstTokenIsGame || bFirstTokenIsServer)
		{
			// Move the token to the end of the list...
			FString RemainingCommandline = ParsedCmdLine;
			RemainingCommandline = RemainingCommandline.Trim();
			RemainingCommandline += FString::Printf(TEXT(" %s"), *Token);
			FCommandLine::Set(*RemainingCommandline); 
		}
		if (bFirstTokenIsCommandlet)
		{
#if STATS
			FThreadStats::MasterDisableForever();
#endif
			if (Token.StartsWith(TEXT("-run=")))
			{
				Token = Token.RightChop(5);
				if (!Token.EndsWith(TEXT("Commandlet")))
				{
					Token += TEXT("Commandlet");
				}
			}
			CommandletCommandLine = ParsedCmdLine;
		}

	}

	if( !bIsNotEditor && GIsGameAgnosticExe )
	{
		// If we launched without a game name or project name, try to load the most recently loaded project file.
		// We can not do this if we are using a FilePlatform override since the game directory may already be established.
		const bool bIsBuildMachine = FParse::Param(FCommandLine::Get(), TEXT("BUILDMACHINE"));
		const bool bLoadMostRecentProjectFileIfItExists = !FApp::HasGameName() && !bFileOverrideFound && !bIsBuildMachine && !FParse::Param( CmdLine, TEXT("norecentproject") );
		if (bLoadMostRecentProjectFileIfItExists )
		{
			LaunchUpdateMostRecentProjectFile();
		}
	}

#if !UE_BUILD_SHIPPING
	// Benchmarking.
	GIsBenchmarking	= FParse::Param(FCommandLine::Get(),TEXT("BENCHMARK"));
#else
	GIsBenchmarking = false;
#endif // !UE_BUILD_SHIPPING

	// Initialize random number generator.
	if( GIsBenchmarking || FParse::Param(FCommandLine::Get(),TEXT("FIXEDSEED")) )
	{
		FMath::RandInit( 0 );
		FMath::SRandInit( 0 );
	}
	else
	{
		FMath::RandInit( FPlatformTime::Cycles() );
		FMath::SRandInit( FPlatformTime::Cycles() );
	}


	FString CheckToken = Token;
	bool bFoundValidToken = false;
	while (!bFoundValidToken && (CheckToken.Len() > 0))
	{
		if (!bIsNotEditor)
		{
			bool bHasNonEditorToken = (CheckToken == TEXT("-GAME")) || (CheckToken == TEXT("-SERVER")) || (CheckToken.StartsWith(TEXT("-RUN="))) || CheckToken.EndsWith(TEXT("Commandlet"));
			if (bHasNonEditorToken)
			{
				bIsNotEditor = true;
				bFoundValidToken = true;
			}
		}
		
		CheckToken = FParse::Token(ParsedCmdLine, 0);
	}

	bHasEditorToken = !bIsNotEditor;
#else	//UE_EDITOR
#if WITH_EDITOR && WITH_EDITORONLY_DATA
	// If a non-editor target build w/ WITH_EDITOR and WITH_EDITORONLY_DATA, use the old token check...
	//@todo. Is this something we need to support?
	bHasEditorToken = Token == TEXT("EDITOR");
#else
	// Game, server and commandlets never set the editor token
	bHasEditorToken = false;
#endif
#endif	//UE_EDITOR

#if !IS_PROGRAM
	if ( !GIsGameAgnosticExe && FApp::HasGameName() && !FPaths::IsProjectFilePathSet() )
	{
		// If we are using a non-agnostic exe where a name was specified but we did not specify a project path. Assemble one based on the game name.
		// @todo uproject This should not be necessary when uproject files are always specified.
		const FString ProjectFilePath = FPaths::Combine(*FPaths::GameDir(), *FString::Printf(TEXT("%s.%s"), FApp::GetGameName(), *IProjectManager::GetProjectFileExtension()));
		FPaths::SetProjectFilePath(ProjectFilePath);
	}

	// Now verify the project file if we have one
	if ( FPaths::IsProjectFilePathSet() )
	{
		if ( !IProjectManager::Get().LoadProjectFile(FPaths::GetProjectFilePath()) )
		{
			// The project file was invalid or saved with a newer version of the engine. Exit.
			return 1;
		}
	}

	if( FApp::HasGameName() )
	{
		// Tell the module manager what the game binaries folder is
		const FString GameBinariesDirectory = FPaths::Combine( FPlatformMisc::GameDir(), TEXT( "Binaries" ), FPlatformProcess::GetBinariesSubdirectory() );
		FModuleManager::Get().SetGameBinariesDirectory(*GameBinariesDirectory);
	}
#endif

	bool bTokenDoesNotHaveDash = Token.Len() && FCString::Strnicmp(*Token, TEXT("-"), 1) != 0;

#if WITH_EDITOR
	// If we're running as an game but don't have a project, inform the user and exit.
	if (bHasEditorToken == false && bFirstTokenIsCommandlet == false)
	{
		if ( !FPaths::IsProjectFilePathSet() )
		{
			//@todo this is too early to localize
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("Engine", "UE4RequiresProjectFiles", "UE4 games require a project file as the first parameter."));
			return 1;
		}
	}

	if (!bHasEditorToken)
	{
		if ((Token == TEXT("MAKE") || Token == TEXT("MAKECOMMANDLET")))
		{
			//@todo this is too early to localize
			const FText Message = NSLOCTEXT("Engine", "MakeRunNoLongerUsed", "The make/run commandlet/arguments are no longer used.");
			if (!FParse::Param(CmdLine,TEXT("BUILDMACHINE")))
			{
				FMessageDialog::Open( EAppMsgType::Ok, Message );
			}
			else
			{
				UE_LOG(LogInit, Fatal, TEXT("%s"), *Message.ToString());
			}
			GGameName[0] = 0; // this disables part of the crash reporter to avoid writing log files to a bogus directory
			exit(0);
		}
	}

	if (GIsUCCMakeStandaloneHeaderGenerator)
	{
		// Rebuilding script requires some hacks in the engine so we flag that.
		PRIVATE_GIsRunningCommandlet = true;
	}
#endif //WITH_EDITOR

	if (FPlatformProcess::SupportsMultithreading())
	{
		GThreadPool	= FQueuedThreadPool::Allocate();
		int32 NumThreadsInThreadPool = FPlatformMisc::NumberOfWorkerThreadsToSpawn();

		// we are only going to give dedicated servers one pool thread
		if (FPlatformProperties::IsServerOnly())
		{
			NumThreadsInThreadPool = 1;
		}
		verify(GThreadPool->Create(NumThreadsInThreadPool));
	}

	// Get a pointer to the log output device
	GLogConsole = GScopedLogConsole.GetOwnedPointer();

	LoadPreInitModules();

	AppInit();
	
#if WITH_ENGINE
	// Initialize system settings before anyone tries to use it...
	GSystemSettings.Initialize( bHasEditorToken );

	// Apply renderer settings from console variables stored in the INI.
	ApplyCVarSettingsFromIni(TEXT("/Script/Engine.RendererSettings"),*GEngineIni);

	// As early as possible to avoid expensive re-init of subsystems, 
	// after SystemSettings.ini file loading so we get the right state,
	// before ConsoleVariables.ini so the local developer can always override.
	// before InitializeCVarsForActiveDeviceProfile() so the platform can override user settings
	Scalability::LoadState(bHasEditorToken ? GEditorUserSettingsIni : GGameUserSettingsIni);

	// Set all CVars which have been setup in the device profiles.
	UDeviceProfileManager::InitializeCVarsForActiveDeviceProfile();

	if (FApp::ShouldUseThreadingForPerformance())
	{
		GUseThreadedRendering = true;
	}
#endif

	FConfigCacheIni::LoadConsoleVariablesFromINI();

	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Platform Initialization"), STAT_PlatformInit, STATGROUP_LoadTime);

		// platform specific initialization now that the SystemSettings are loaded
		FPlatformMisc::PlatformInit();
		FPlatformMemory::Init();
	}

	// Let LogConsole know what ini file it should use to save its setting on exit.
	// We can't use GGameIni inside log console because it's destroyed in the global
	// scoped pointer and at that moment GGameIni may already be gone.
	if( GLogConsole != NULL )
	{
		GLogConsole->SetIniFilename(*GGameIni);
	}


#if CHECK_PUREVIRTUALS
	FMessageDialog::Open( EAppMsgType::Ok, *NSLOCTEXT("Engine", "Error_PureVirtualsEnabled", "The game cannot run with CHECK_PUREVIRTUALS enabled.  Please disable CHECK_PUREVIRTUALS and rebuild the executable.").ToString() );
	FPlatformMisc::RequestExit(false);
#endif

#if WITH_ENGINE
	// allow for game explorer processing (including parental controls) and firewalls installation
	if (!FPlatformMisc::CommandLineCommands())
	{
		FPlatformMisc::RequestExit(false);
	}
	
	bool bIsSeekFreeDedicatedServer = false;	
	bool bIsRegularClient = false;

	if (!bHasEditorToken)
	{
		// See whether the first token on the command line is a commandlet.

		//@hack: We need to set these before calling StaticLoadClass so all required data gets loaded for the commandlets.
		GIsClient = true;
		GIsServer = true;
#if WITH_EDITOR
		GIsEditor = true;
		PRIVATE_GIsRunningCommandlet = true;
#endif	//WITH_EDITOR

		// We need to disregard the empty token as we try finding Token + "Commandlet" which would result in finding the
		// UCommandlet class if Token is empty.
		bool bDefinitelyCommandlet = (bTokenDoesNotHaveDash && Token.EndsWith(TEXT("Commandlet")));
		if (!bTokenDoesNotHaveDash)
		{
			if (Token.StartsWith(TEXT("-run=")))
			{
				Token = Token.RightChop(5);
				bDefinitelyCommandlet = true;
				if (!Token.EndsWith(TEXT("Commandlet")))
				{
					Token += TEXT("Commandlet");
				}
			}
		}
		else 
		{
			if (!bDefinitelyCommandlet)
			{
				UClass* TempCommandletClass = FindObject<UClass>(ANY_PACKAGE, *(Token+TEXT("Commandlet")), false);

				if (TempCommandletClass)
				{
					check(TempCommandletClass->IsChildOf(UCommandlet::StaticClass())); // ok so you have a class that ends with commandlet that is not a commandlet
					
					Token += TEXT("Commandlet");
					bDefinitelyCommandlet = true;
				}
			}
		}

		if (!bDefinitelyCommandlet)
		{
			bIsRegularClient = true;
			GIsClient = true;
			GIsServer = false;
#if WITH_EDITORONLY_DATA
			GIsEditor = false;
			PRIVATE_GIsRunningCommandlet = false;
#endif
		}
	}

	if (IsRunningDedicatedServer())
	{
		GIsClient = false;
		GIsServer = true;
#if WITH_EDITOR
		PRIVATE_GIsRunningCommandlet = false;
		GIsEditor = false;
#endif
		bIsSeekFreeDedicatedServer = FPlatformProperties::RequiresCookedData();
	}

	// Initialize the RHI.
	RHIInit(bHasEditorToken);

	if (!FPlatformProperties::RequiresCookedData())
	{
		check(!GShaderCompilingManager);
		GShaderCompilingManager = new FShaderCompilingManager();
	}

	FIOSystem::Get(); // force it to be created if it isn't already

	// allow the platform to start up any features it may need
	IPlatformFeaturesModule::Get();

	// Init physics engine before loading anything, in case we want to do things like cook during post-load.
	InitGamePhys();

	// Delete temporary files in cache.
	FPlatformProcess::CleanFileCache();



#if !UE_BUILD_SHIPPING
	GIsDemoMode = FParse::Param( FCommandLine::Get(), TEXT( "DEMOMODE" ) );
#endif

	if (bHasEditorToken)
	{	
#if WITH_EDITOR

		// We're the editor.
		GIsClient = true;
		GIsServer = true;
		GIsEditor = true;
		PRIVATE_GIsRunningCommandlet = false;

		GWarn = &UnrealEdWarn;

#else	
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("Engine", "EditorNotSupported", "Editor not supported in this mode."));
		FPlatformMisc::RequestExit(false);
		return 1;
#endif //WITH_EDITOR
 	}

	// If we're not in the editor stop collecting the backlog now that we know
	if (!GIsEditor)
	{
		GLog->EnableBacklog( false );
	}

	EndInitTextLocalization();

	GStreamingManager = new FStreamingManagerCollection();

	if (FPlatformProcess::SupportsMultithreading() && !IsRunningDedicatedServer() && (bIsRegularClient || bHasEditorToken))
	{
		FPlatformSplash::Show();
	}

	if ( !IsRunningDedicatedServer() )
	{
		if ( bHasEditorToken || bIsRegularClient )
		{
			// Init platform application
			FSlateApplication::Create();
		}
		else
		{
			FCoreStyle::ResetToDefault();
		}
	}
	else
	{
		FCoreStyle::ResetToDefault();
	}

	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Initial UObject load"), STAT_InitialUObjectLoad, STATGROUP_LoadTime);

		// Initialize shader types before loading any shaders
		InitializeShaderTypes();

		// Load the global shaders.
		if (GetGlobalShaderMap() == NULL && GIsRequestingExit)
		{
			// This means we can't continue without the global shader map.
			return 1;
		}

		// In order to be able to use short script package names get all script
		// package names from ini files and register them with FPackageName system.
		FPackageName::RegisterShortPackageNamesForUObjectModules();


		// Make sure all UObject classes are registered and default properties have been initialized
		ProcessNewlyLoadedUObjects();

		// Default materials may have been loaded due to dependencies when loading
		// classes and class default objects. If not, do so now.
		UMaterialInterface::InitDefaultMaterials();
		UMaterialInterface::AssertDefaultMaterialsExist();
		UMaterialInterface::AssertDefaultMaterialsPostLoaded();
	}

	// Tell the module manager is may now process newly-loaded UObjects when new C++ modules are loaded
	FModuleManager::Get().StartProcessingNewlyLoadedObjects();

	// load up the seek-free startup packages
	if ( !FStartupPackages::LoadAll() )
	{
		// At least one startup package failed to load, return 1 to indicate an error
		return 1;
	}

	// Setup GC optimizations
	if (bIsSeekFreeDedicatedServer || bHasEditorToken)
	{
		GUObjectArray.DisableDisregardForGC();
	}
	
	if ( !LoadStartupCoreModules() )
	{
		// At least one startup module failed to load, return 1 to indicate an error
		return 1;
	}

#if !UE_SERVER// && !UE_EDITOR
	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		TSharedRef<FSlateRenderer> SlateRenderer = FModuleManager::Get().GetModuleChecked<FSlateRHIRendererModule>("SlateRHIRenderer").CreateSlateRHIRenderer();

		// If Slate is being used, initialize the renderer after RHIInit 
		FSlateApplication& CurrentSlateApp = FSlateApplication::Get();
		CurrentSlateApp.InitializeRenderer( SlateRenderer );
		
		GetMoviePlayer()->SetSlateRenderer(SlateRenderer);
	}
#endif

	// Load up all modules that need to hook into the loading screen
	if (!IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PreLoadingScreen))
	{
		return 1;
	}

	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreLoadingScreen);
	
	
#if !UE_SERVER
	if ( !IsRunningDedicatedServer() )
	{
		if (!IsRunningCommandlet())
		{
			// Note: It is critical that resolution settings are loaded before the movie starts playing so that the window size and fullscreen state is known
			UGameUserSettings::PreloadResolutionSettings();
		}

		// @todo ps4: If a loading movie starts earlier, which it probably should, then please see PS4's PlatformPostInit() implementation!

		// allow the movie player to load a sequence from the .inis (a PreLoadingScreen module could have already initialized a sequence, in which case
		// it wouldn't have anything in it's .ini file)
		GetMoviePlayer()->SetupLoadingScreenFromIni();

		GetMoviePlayer()->Initialize();
		GetMoviePlayer()->PlayMovie();
	}
#endif

	// do any post appInit processing, before the render thread is started.
	FPlatformMisc::PlatformPostInit();

	if (GUseThreadedRendering)
	{
		StartRenderingThread();
	}
	
	if ( !LoadStartupModules() )
	{
		// At least one startup module failed to load, return 1 to indicate an error
		return 1;
	}

	MarkObjectsToDisregardForGC(); 
	GUObjectArray.CloseDisregardForGC();

	if (FParse::Param(FCommandLine::Get(), TEXT("stdout")) || (!bHasEditorToken && !bIsRegularClient && !IsRunningDedicatedServer()))
	{
		GScopedStdOut = new FOutputDeviceStdOutput(); 
		GLog->AddOutputDevice( GScopedStdOut.GetOwnedPointer() );
	}

#if WITH_ENGINE
	SetIsServerForOnlineSubsystemsDelegate(FQueryIsRunningServer::CreateStatic(&IsServerDelegateForOSS));
#endif

#if WITH_EDITOR
	if (!bHasEditorToken)
	{
		UClass* CommandletClass = NULL;

		if (!bIsRegularClient)
		{
			CommandletClass = FindObject<UClass>(ANY_PACKAGE,*Token,false);
			if (!CommandletClass)
			{
				UE_LOG(LogInit, Fatal,TEXT("%s looked like a commandlet, but we could not find the class."), *Token);
			}

			extern bool GIsConsoleExecutable;
			if (GIsConsoleExecutable)
			{
				if (GLogConsole != NULL && GLogConsole->IsAttached())
				{
					GLog->RemoveOutputDevice(GLogConsole);
				}
				// Setup Ctrl-C handler for console application
				FPlatformMisc::SetGracefulTerminationHandler();
			}
			else
			{
				// Bring up console unless we're a silent build.
				if( GLogConsole && !GIsSilent )
				{
					GLogConsole->Show( true );
				}
			}

			// print output immediately
			setvbuf(stdout, NULL, _IONBF, 0);

			UE_LOG(LogInit, Log,  TEXT("Executing %s"), *CommandletClass->GetFullName() );

			// Allow commandlets to individually override those settings.
			UCommandlet* Default = CastChecked<UCommandlet>(CommandletClass->GetDefaultObject());

			if ( GIsRequestingExit )
			{
				// commandlet set GIsRequestingExit during construction
				return 1;
			}

			GIsClient = Default->IsClient;
			GIsServer = Default->IsServer;
			GIsEditor = Default->IsEditor;
			PRIVATE_GIsRunningCommandlet = true;
			// Reset aux log if we don't want to log to the console window.
			if( !Default->LogToConsole )
			{
				GLog->RemoveOutputDevice( GLogConsole );
			}

			GIsRequestingExit = true;	// so CTRL-C will exit immediately
			
			// allow the commandlet the opportunity to create a custom engine
			CommandletClass->GetDefaultObject<UCommandlet>()->CreateCustomEngine(CommandletCommandLine);
			if ( GEngine == NULL )
			{
				if ( GIsEditor )
				{
					UClass* EditorEngineClass = StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("engine-ini:/Script/Engine.Engine.EditorEngine"), NULL, LOAD_None, NULL );

					GEngine = GEditor = ConstructObject<UEditorEngine>( EditorEngineClass );

					UE_LOG(LogInit, Log, TEXT("Initializing Editor Engine..."));
					GEditor->InitEditor(this);
					UE_LOG(LogInit, Log, TEXT("Initializing Editor Engine Completed"));
				}
				else
				{
					UClass* EngineClass = StaticLoadClass( UEngine::StaticClass(), NULL, TEXT("engine-ini:/Script/Engine.Engine.GameEngine"), NULL, LOAD_None, NULL );

					// must do this here so that the engine object that we create on the next line receives the correct property values
					GEngine = ConstructObject<UEngine>( EngineClass );
					check(GEngine);

					GEngine->ParseCommandline();

					UE_LOG(LogInit, Log, TEXT("Initializing Game Engine..."));
					GEngine->Init(this);
					UE_LOG(LogInit, Log, TEXT("Initializing Game Engine Completed"));
				}
			}

			//run automation smoke tests now that the commandlet has had a chance to override the above flags and GEngine is available
#if !PLATFORM_HTML5 && !PLATFORM_HTML5_WIN32 
			FAutomationTestFramework::GetInstance().RunSmokeTests();
#endif 
	
			UCommandlet* Commandlet = ConstructObject<UCommandlet>( CommandletClass );
			check(Commandlet);
			Commandlet->AddToRoot();			

			// Execute the commandlet.
			double CommandletExecutionStartTime = FPlatformTime::Seconds();

			// Commandlets don't always handle -run= properly in the commandline so we'll provide them
			// with a custom version that doesn't have it.
			Commandlet->ParseParms( CommandletCommandLine );
			int32 ErrorLevel = Commandlet->Main( CommandletCommandLine );

			// Log warning/ error summary.
			if( Commandlet->ShowErrorCount )
			{
				if( GWarn->Errors.Num() || GWarn->Warnings.Num() )
				{
					SET_WARN_COLOR(COLOR_WHITE);
					UE_LOG(LogInit, Display, TEXT(""));
					UE_LOG(LogInit, Display, TEXT("Warning/Error Summary"));
					UE_LOG(LogInit, Display, TEXT("---------------------"));

					static const int32 MaxMessagesToShow = 50;
					TSet<FString> ShownMessages;
					
					SET_WARN_COLOR(COLOR_RED);
					ShownMessages.Empty(MaxMessagesToShow);
					for(auto It = GWarn->Errors.CreateConstIterator(); It; ++It)
					{
						bool bAlreadyShown = false;
						ShownMessages.Add(*It, &bAlreadyShown);

						if (!bAlreadyShown)
						{
							if (ShownMessages.Num() > MaxMessagesToShow)
							{
								SET_WARN_COLOR(COLOR_WHITE);
								UE_LOG(LogInit, Display, TEXT("NOTE: Only first %d errors displayed."), MaxMessagesToShow);
								break;
							}

							UE_LOG(LogInit, Display, TEXT("%s"), **It);
						}
					}

					SET_WARN_COLOR(COLOR_YELLOW);
					ShownMessages.Empty(MaxMessagesToShow);
					for(auto It = GWarn->Warnings.CreateConstIterator(); It; ++It)
					{
						bool bAlreadyShown = false;
						ShownMessages.Add(*It, &bAlreadyShown);

						if (!bAlreadyShown)
						{
							if (ShownMessages.Num() > MaxMessagesToShow)
							{
								SET_WARN_COLOR(COLOR_WHITE);
								UE_LOG(LogInit, Display, TEXT("NOTE: Only first %d warnings displayed."), MaxMessagesToShow);
								break;
							}

							UE_LOG(LogInit, Display, TEXT("%s"), **It);
						}
					}
				}

				UE_LOG(LogInit, Display, TEXT(""));

				if( ErrorLevel != 0 )
				{
					SET_WARN_COLOR(COLOR_RED);
					UE_LOG(LogInit, Display, TEXT("Commandlet->Main return this error code: %d"), ErrorLevel );
					UE_LOG(LogInit, Display, TEXT("With %d error(s), %d warning(s)"), GWarn->Errors.Num(), GWarn->Warnings.Num() );
				}
				else if( ( GWarn->Errors.Num() == 0 ) )
				{
					SET_WARN_COLOR(GWarn->Warnings.Num() ? COLOR_YELLOW : COLOR_GREEN);
					UE_LOG(LogInit, Display, TEXT("Success - %d error(s), %d warning(s)"), GWarn->Errors.Num(), GWarn->Warnings.Num() );
				}
				else
				{
					SET_WARN_COLOR(COLOR_RED);
					UE_LOG(LogInit, Display, TEXT("Failure - %d error(s), %d warning(s)"), GWarn->Errors.Num(), GWarn->Warnings.Num() );
					ErrorLevel = 1;
				}
				CLEAR_WARN_COLOR();
			}
			else
			{
				UE_LOG(LogInit, Display, TEXT("Finished.") );
			}
		
			double CommandletExecutionTime = FPlatformTime::Seconds() - CommandletExecutionStartTime;
			UE_LOG(LogInit, Display, LINE_TERMINATOR TEXT( "Execution of commandlet took:  %.2f seconds"), CommandletExecutionTime );

			// We're ready to exit!
			return ErrorLevel;
		}
		else
		{
			// We're a regular client.
			check(bIsRegularClient);

			if (bTokenDoesNotHaveDash)
			{
				// here we give people a reasonable warning if they tried to use the short name of a commandlet
				UClass* TempCommandletClass = FindObject<UClass>(ANY_PACKAGE,*(Token+TEXT("Commandlet")),false);
				if (TempCommandletClass)
				{
					UE_LOG(LogInit, Fatal, TEXT("You probably meant to call a commandlet. Please use the full name %s."), *(Token+TEXT("Commandlet")));
				}
			}
		}
	}

#endif	//WITH_EDITOR

	// exit if wanted.
	if( GIsRequestingExit )
	{
		if ( GEngine != NULL )
		{
			GEngine->PreExit();
		}
		AppPreExit();
		// appExit is called outside guarded block.
		return 1;
	}

	FString MatineeName;

	if(FParse::Param(FCommandLine::Get(),TEXT("DUMPMOVIE")) || FParse::Value(FCommandLine::Get(), TEXT("-MATINEESSCAPTURE="), MatineeName))
	{
		// -1: remain on
		GIsDumpingMovie = -1;
	}

	// If dumping movie then we do NOT want on-screen messages
	GAreScreenMessagesEnabled = !GIsDumpingMovie && !GIsDemoMode;

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(),TEXT("NOSCREENMESSAGES")))
	{
		GAreScreenMessagesEnabled = false;
	}

	// Don't update INI files if benchmarking or -noini
	if( GIsBenchmarking || FParse::Param(FCommandLine::Get(),TEXT("NOINI")))
	{
		GConfig->Detach( GEngineIni );
		GConfig->Detach( GInputIni );
		GConfig->Detach( GGameIni );
		GConfig->Detach( GEditorIni );
	}
#endif // !UE_BUILD_SHIPPING

	delete [] CommandLineCopy;

	// initialize the pointer, as it is deleted before being assigned in the first frame
	PendingCleanupObjects = NULL;

	// Initialize profile visualizers.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FModuleManager::Get().LoadModule(TEXT("TaskGraph"));
	if (FPlatformProcess::SupportsMultithreading())
	{
		FModuleManager::Get().LoadModule(TEXT("ProfilerService"));
		FModuleManager::Get().GetModuleChecked<IProfilerServiceModule>("ProfilerService").CreateProfilerServiceManager();
	}
#endif

#endif
	
	//run automation smoke tests now that everything is setup to run
	FAutomationTestFramework::GetInstance().RunSmokeTests();
	
	return 0;
}


void FEngineLoop::LoadPreInitModules()
{	
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading PreInit Modules"), STAT_PreInitModules, STATGROUP_LoadTime);

	// GGetMapNameDelegate is initialized here

	// Always attempt to load CoreUObject. It requires additional pre init which is called from its module's StartupModule method.
#if WITH_COREUOBJECT
	FModuleManager::Get().LoadModule(TEXT("CoreUObject"));
#endif

#if WITH_ENGINE
	FModuleManager::Get().LoadModule(TEXT("Engine"));

	FModuleManager::Get().LoadModule(TEXT("Renderer"));

	
#if !UE_SERVER
	if (!IsRunningDedicatedServer() )
	{
		// This needs to be loaded before InitializeShaderTypes is called
		FModuleManager::Get().LoadModuleChecked<FSlateRHIRendererModule>("SlateRHIRenderer");
	}
#endif

	FPlatformMisc::LoadPreInitModules();
	// Initialize ShaderCore before loading or compiling any shaders,
	// But after Renderer and any other modules which implement shader types.
	FModuleManager::Get().LoadModule(TEXT("ShaderCore"));

#if WITH_EDITORONLY_DATA
	// Load the texture compressor module before any textures load. They may
	// compress asynchronously and that can lead to a race condition.
	FModuleManager::Get().LoadModule(TEXT("TextureCompressor"));
#endif
#endif // WITH_ENGINE
}

#if WITH_ENGINE

bool FEngineLoop::LoadStartupCoreModules()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Startup Modules"), STAT_StartupModules, STATGROUP_LoadTime);

	bool bSuccess = true;

	// Load all Runtime modules
	{
		FModuleManager::Get().LoadModule(TEXT("Core"));
		FModuleManager::Get().LoadModule(TEXT("Networking"));
	}

	FPlatformMisc::LoadStartupModules();

	// initialize messaging
	if (FPlatformProcess::SupportsMultithreading())
	{
		FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");

		if (!IsRunningCommandlet())
		{
			SessionService = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices").GetSessionService();
			SessionService->Start();
		}

		EngineService = new FEngineService();
	}

#if WITH_EDITOR
		FModuleManager::LoadModuleChecked<IEditorStyleModule>("EditorStyle");
#endif //WITH_EDITOR

	// Load all Development modules
	if (!IsRunningDedicatedServer())
	{
		FModuleManager::Get().LoadModule("Slate");

#if WITH_UNREAL_DEVELOPER_TOOLS
		FModuleManager::Get().LoadModule("MessageLog");
		FModuleManager::Get().LoadModule("CollisionAnalyzer");
#endif	//WITH_UNREAL_DEVELOPER_TOOLS
	}

#if WITH_UNREAL_DEVELOPER_TOOLS
		FModuleManager::Get().LoadModule("FunctionalTesting");
#endif	//WITH_UNREAL_DEVELOPER_TOOLS

#if (WITH_EDITOR && !(UE_BUILD_SHIPPING || UE_BUILD_TEST))
	// HACK: load BT editor as early as possible for statically initialized assets (non cooked BT assets needs it)
	// cooking needs this module too
	bool bBehaviorTreeEditorEnabled=false;
	GConfig->GetBool(TEXT("BehaviorTreesEd"), TEXT("BehaviorTreeEditorEnabled"), bBehaviorTreeEditorEnabled, GEngineIni);

	//we can override config settings from EpicLabs
	bool bBehaviorTreeEditorEnabledFromUserSettings=false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.EditorExperimentalSettings"), TEXT("bBehaviorTreeEditor"), bBehaviorTreeEditorEnabledFromUserSettings, GEditorUserSettingsIni);

	if (bBehaviorTreeEditorEnabled || bBehaviorTreeEditorEnabledFromUserSettings)
	{
		//let's load BT editor even for Rocket users (ShooterGame needs it but other projects should have it disabled in config file). 
		FModuleManager::Get().LoadModule(TEXT("BehaviorTreeEditor"));
	}

	// HACK: load EQS editor as early as possible for statically initialized assets (non cooked EQS assets needs it)
	// cooking needs this module too
	bool bEnvironmentQueryEditor = false;
	GConfig->GetBool(TEXT("EnvironmentQueryEd"), TEXT("EnableEnvironmentQueryEd"), bEnvironmentQueryEditor, GEngineIni);
	if (bEnvironmentQueryEditor)
	{
		FModuleManager::Get().LoadModule(TEXT("EnvironmentQueryEditor"));
	}

#endif //(WITH_EDITOR && !(UE_BUILD_SHIPPING || UE_BUILD_TEST))

	return bSuccess;
}


bool FEngineLoop::LoadStartupModules()
{
	// Load any modules that want to be loaded before default modules are loaded up.
	if (!IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PreDefault))
	{
		return false;
	}

	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreDefault);

	// Load modules that are configured to load in the default phase
	if (!IProjectManager::Get().LoadModulesForProject(ELoadingPhase::Default))
	{
		return false;
	}

	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::Default);

	// Load any modules that want to be loaded after default modules are loaded up.
	if (!IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostDefault))
	{
		return false;
	}

	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostDefault);

	return true;
}


void FEngineLoop::InitTime()
{
	// Init variables used for benchmarking and ticking.
	GCurrentTime				= FPlatformTime::Seconds();
	MaxFrameCounter				= 0;
	MaxTickTime					= 0;
	TotalTickTime				= 0;
	LastFrameCycles				= FPlatformTime::Cycles();

	float FloatMaxTickTime		= 0;
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(),TEXT("SECONDS="),FloatMaxTickTime);
	MaxTickTime					= FloatMaxTickTime;

	// look of a version of seconds that only is applied if GIsBenchmarking is set. This makes it easier on
	// say, iOS, where we have a toggle setting to enable benchmarking, but don't want to have to make user
	// also disable the seconds setting as well. -seconds= will exit the app after time even if benchmarking
	// is not enabled
	// NOTE: This will override -seconds= if it's specified
	if (GIsBenchmarking)
	{
		if (FParse::Value(FCommandLine::Get(),TEXT("BENCHMARKSECONDS="),FloatMaxTickTime) && FloatMaxTickTime)
		{
			MaxTickTime			= FloatMaxTickTime;
		}
	}

	// Use -FPS=X to override fixed tick rate if e.g. -BENCHMARK is used.
	float FixedFPS = 0;
	FParse::Value(FCommandLine::Get(),TEXT("FPS="),FixedFPS);
	if( FixedFPS > 0 )
	{
		GEngine->MatineeCaptureFPS = (int32)FixedFPS;
		GFixedDeltaTime = 1 / FixedFPS;
	}
#endif // !UE_BUILD_SHIPPING

	// convert FloatMaxTickTime into number of frames (using 1 / GFixedDeltaTime to convert fps to seconds )
	MaxFrameCounter				= FMath::Trunc( MaxTickTime / GFixedDeltaTime );
}

void FlushStatsFrame(bool bDiscardCallstack)
{
	check(IsInGameThread()); 
	static int32 MasterDisableChangeTagStartFrame = -1;
	static int64 StatsFrame = 1;
	StatsFrame++;
#if STATS
	int64 Frame = StatsFrame;
	if (bDiscardCallstack)
	{
		FThreadStats::FrameDataIsIncomplete(); // we won't collect call stack stats this frame
	}
	if (MasterDisableChangeTagStartFrame == -1)
	{
		MasterDisableChangeTagStartFrame = FThreadStats::MasterDisableChangeTag();
	}
	if (!FThreadStats::IsCollectingData() ||  MasterDisableChangeTagStartFrame != FThreadStats::MasterDisableChangeTag())
	{
		Frame = -StatsFrame; // mark this as a bad frame
	}
	static FStatNameAndInfo Adv(NAME_AdvanceFrame, "", TEXT(""), EStatDataType::ST_int64, true, false);
	FThreadStats::AddMessage(Adv.GetEncodedName(), EStatOperation::AdvanceFrameEventGameThread, Frame); // we need to flush here if we aren't collecting stats to make sure the meta data is up to date
	if (FPlatformProperties::IsServerOnly())
	{
		FThreadStats::AddMessage(Adv.GetEncodedName(), EStatOperation::AdvanceFrameEventRenderThread, Frame); // we need to flush here if we aren't collecting stats to make sure the meta data is up to date
	}
#endif
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( 
		RenderingThreadTickCommand, 
		int64, SentStatsFrame, StatsFrame,
		int32, SentMasterDisableChangeTagStartFrame, MasterDisableChangeTagStartFrame,
	{ 
		RenderingThreadTick(SentStatsFrame, SentMasterDisableChangeTagStartFrame); 
	} 
	);
	if (bDiscardCallstack)
	{
		// we need to flush the rendering thread here, otherwise it can get behind and then the stats will get behind.
		FlushRenderingCommands();
	}

#if STATS
	FThreadStats::ExplicitFlush(bDiscardCallstack);
	FThreadStats::WaitForStats();
	MasterDisableChangeTagStartFrame = FThreadStats::MasterDisableChangeTag();
#endif

}

//called via FCoreDelegates::StarvedGameLoop
void GameLoopIsStarved()
{
	FlushStatsFrame(true);
}


int32 FEngineLoop::Init()
{
	// Figure out which UEngine variant to use.
	UClass* EngineClass = NULL;
	if( !GIsEditor )
	{
		// We're the game.
		EngineClass = StaticLoadClass( UGameEngine::StaticClass(), NULL, TEXT("engine-ini:/Script/Engine.Engine.GameEngine"), NULL, LOAD_None, NULL );
		GEngine = ConstructObject<UEngine>( EngineClass );
	}
	else
	{
#if WITH_EDITOR
		// We're UnrealEd.
		EngineClass = StaticLoadClass( UUnrealEdEngine::StaticClass(), NULL, TEXT("engine-ini:/Script/Engine.Engine.UnrealEdEngine"), NULL, LOAD_None, NULL );
		GEngine = GEditor = GUnrealEd = ConstructObject<UUnrealEdEngine>( EngineClass );
#else
		check(0);
#endif
	}

	check( GEngine );
	
	GetMoviePlayer()->PassLoadingScreenWindowBackToGame();

	GEngine->ParseCommandline();

	InitTime();

	GEngine->Init(this);

	GetMoviePlayer()->WaitForMovieToFinish();

	// initialize automation worker
#if WITH_AUTOMATION_WORKER
	FModuleManager::Get().LoadModule("AutomationWorker");
#endif

#if WITH_EDITOR
	if( GIsEditor )
	{
		FModuleManager::GetModuleChecked<IAutomationControllerModule>("AutomationController").Init();
		FModuleManager::Get().LoadModule(TEXT("ProfilerClient"));
	}
#endif

	GIsRunning = true;

	// let the game script code run any special code for initial boot-up (this is a one time call ever)
	for (TObjectIterator<UWorld> ObjIt;  ObjIt; ++ObjIt)
	{
		UWorld* const EachWorld = CastChecked<UWorld>(*ObjIt);
		if (EachWorld)
		{
			AGameMode* const GameMode = EachWorld->GetAuthGameMode();
			if (GameMode)
			{
				GameMode->OnEngineHasLoaded();
			}
		}
	}

	if( !GIsEditor)
	{
		// hide a couple frames worth of rendering
		FViewport::SetGameRenderingEnabled(true, 3);
	}

	// Begin the async platform hardware survey
	GEngine->InitHardwareSurvey();

	FCoreDelegates::StarvedGameLoop.BindStatic(&GameLoopIsStarved);

	return 0;
}


void FEngineLoop::Exit()
{
	GIsRunning	= 0;
	GLogConsole	= NULL;

	// Make sure we're not in the middle of loading something.
	FlushAsyncLoading();

	// Block till all outstanding resource streaming requests are fulfilled.
	if (GStreamingManager != NULL)
	{
		UTexture2D::CancelPendingTextureStreaming();
		GStreamingManager->BlockTillAllRequestsFinished();
	}

#if WITH_ENGINE
	// shut down messaging
	delete EngineService;
	EngineService = NULL;

	if (SessionService.IsValid())
	{
		SessionService->Stop();
		SessionService.Reset();
	}
#endif // WITH_ENGINE

	MALLOC_PROFILER( GEngine->Exec( NULL, TEXT( "MPROF STOP" ) ); )

	if ( GEngine != NULL )
	{
		GEngine->ShutdownAudioDevice();
	}

	// close all windows
	FSlateApplication::Shutdown();


	if ( GEngine != NULL )
	{
		GEngine->PreExit();
	}

	AppPreExit();

	// Shutdown and unregister all online subsystems
	Online::ShutdownOnlineSubsystem();

	TermGamePhys();
	ParticleVertexFactoryPool_FreePool();
	MotionBlur_Free();

	// Stop the rendering thread.
	StopRenderingThread();

	// Tear down the RHI.
	RHIExit();

	// Unload all modules.  Note that this doesn't actually unload the module DLLs (that happens at
	// process exit by the OS), but it does call ShutdownModule() on all loaded modules in the reverse
	// order they were loaded in, so that systems can unregister and perform general clean up.
	FModuleManager::Get().UnloadModulesAtShutdown();

#if STATS
	FThreadStats::StopThread();
#endif

	FTaskGraphInterface::Shutdown();

	delete GStreamingManager;
	GStreamingManager	= NULL;

	FIOSystem::Shutdown();
}

bool FEngineLoop::ShouldUseIdleMode() const
{
	static const auto CVarIdleWhenNotForeground = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("t.IdleWhenNotForeground"));
	bool bIdleMode = false;

	// Yield and prevent ticking and rendering when desired
	if (FApp::IsGame() 
		&& CVarIdleWhenNotForeground->GetValueOnGameThread()
		&& FPlatformProperties::SupportsWindowedMode()
		&& !FPlatformProcess::IsThisApplicationForeground())
	{
		bIdleMode = true;

		const TArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();

		for (int32 WorldIndex = 0; WorldIndex < WorldContexts.Num(); ++WorldIndex)
		{
			if (!WorldContexts[WorldIndex].World()->AreAlwaysLoadedLevelsLoaded())
			{
				bIdleMode = false;
				break;
			}
		}
	}

	return bIdleMode;
}

void FEngineLoop::Tick()
{
	// If a movie that is blocking the game thread has been playing,
	// wait for it to finish before we tick again, otherwise we'll get
	// multiple threads ticking simultaneously, which is bad
	GetMoviePlayer()->WaitForMovieToFinish();

	// early in the Tick() to get the callbacks for cvar changes called
	IConsoleManager::Get().CallAllConsoleVariableSinks();

	{ 
		SCOPE_CYCLE_COUNTER( STAT_FrameTime );	

		ENQUEUE_UNIQUE_RENDER_COMMAND(
				BeginFrame,
			{
				GFrameNumberRenderThread++;
				GDynamicRHI->PushEvent(*FString::Printf(TEXT("Frame%d"),GFrameNumberRenderThread));
				RHIBeginFrame();
			});

		// Flush debug output which has been buffered by other threads.
		GLog->FlushThreadedLogs();

		// Exit if frame limit is reached in benchmark mode.
		if( (GIsBenchmarking && MaxFrameCounter && (GFrameCounter > MaxFrameCounter))
			// or time limit is reached if set.
			|| (MaxTickTime && (TotalTickTime > MaxTickTime)) )
		{
			FPlatformMisc::RequestExit(0);
		}

		// Set GCurrentTime, GDeltaTime and potentially wait to enforce max tick rate.
		GEngine->UpdateTimeAndHandleMaxTickRate();

		GEngine->TickFPSChart( GDeltaTime );

#if STATS	// Can't do this within the malloc classes as they get initialized before the stats

		// @todo FPlatformMemory::UpdateStats();
		FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		SET_DWORD_STAT(STAT_VirtualAllocSize,MemoryStats.PagefileUsage);
		SET_DWORD_STAT(STAT_PhysicalAllocSize,MemoryStats.WorkingSetSize);

		static uint64 LastMallocCalls		= 0;
		static uint64 LastReallocCalls		= 0;
		static uint64 LastFreeCalls			= 0;

		uint32 CurrentFrameMallocCalls		= FMalloc::TotalMallocCalls - LastMallocCalls;
		uint32 CurrentFrameReallocCalls		= FMalloc::TotalReallocCalls - LastReallocCalls;
		uint32 CurrentFrameFreeCalls			= FMalloc::TotalFreeCalls - LastFreeCalls;
		uint32 CurrentFrameAllocatorCalls	= CurrentFrameMallocCalls + CurrentFrameReallocCalls + CurrentFrameFreeCalls;

		SET_DWORD_STAT( STAT_MallocCalls, CurrentFrameMallocCalls );
		SET_DWORD_STAT( STAT_ReallocCalls, CurrentFrameReallocCalls );
		SET_DWORD_STAT( STAT_FreeCalls, CurrentFrameFreeCalls );
		SET_DWORD_STAT( STAT_TotalAllocatorCalls, CurrentFrameAllocatorCalls );

		LastMallocCalls			= FMalloc::TotalMallocCalls;
		LastReallocCalls		= FMalloc::TotalReallocCalls;
		LastFreeCalls			= FMalloc::TotalFreeCalls;
#endif
	} 

	FlushStatsFrame(false);

	{ 
		SCOPE_CYCLE_COUNTER( STAT_FrameTime );
		
		STAT(extern ENGINE_API void BeginOneFrameParticleStats());
		STAT(BeginOneFrameParticleStats());

		// Calculates average FPS/MS (outside STATS on purpose)
		CalculateFPSTimings();

		// Note the start of a new frame
		MALLOC_PROFILER(GMalloc->Exec( NULL, *FString::Printf(TEXT("SNAPSHOTMEMORYFRAME")),*GLog));

		// handle some per-frame tasks on the rendering thread
		ENQUEUE_UNIQUE_RENDER_COMMAND(
			ResetDeferredUpdates,
			{
				FDeferredUpdateResource::ResetNeedsUpdate();
			});
		
		{
#if WITH_ENGINE
			SCOPE_CYCLE_COUNTER( STAT_PlatformMessageTime );
#endif
			SCOPE_CYCLE_COUNTER(STAT_PumpMessages);
			FPlatformMisc::PumpMessages(true);
		}

		const bool bIdleMode = ShouldUseIdleMode();

		if (bIdleMode)
		{
			// Yield CPU time
			FPlatformProcess::Sleep(.1f);
		}

		if (FSlateApplication::IsInitialized() && !bIdleMode)
		{
			FSlateApplication::Get().PollGameDeviceState();
		}

		GEngine->Tick( GDeltaTime, bIdleMode );

		if (GShaderCompilingManager)
		{
			// Process any asynchronous shader compile results that are ready, limit execution time
			GShaderCompilingManager->ProcessAsyncResults(true, false);
		}

		if (FSlateApplication::IsInitialized() && !bIdleMode)
		{
			check(!IsRunningDedicatedServer());

			// Tick Slate application
			FSlateApplication::Get().Tick();
		}

#if WITH_EDITOR
		if( GIsEditor )
		{
			//Check if module loaded to support the change to allow this to be hot compilable.
			if (FModuleManager::Get().IsModuleLoaded(TEXT("AutomationController")))
			{
				FModuleManager::GetModuleChecked<IAutomationControllerModule>("AutomationController").Tick();
			}
		}
#endif

#if WITH_ENGINE
#if WITH_AUTOMATION_WORKER
		//Check if module loaded to support the change to allow this to be hot compilable.
		static const FName AutomationWorkerModuleName = TEXT("AutomationWorker");
		if (FModuleManager::Get().IsModuleLoaded(AutomationWorkerModuleName))
		{
			FModuleManager::GetModuleChecked<IAutomationWorkerModule>(AutomationWorkerModuleName).Tick();
		}
#endif
#endif //WITH_ENGINE

		{			
#if WITH_ENGINE
			SCOPE_CYCLE_COUNTER(STAT_RHITickTime);
#endif
			RHITick( GDeltaTime ); // Update RHI.
		}

		// Increment global frame counter. Once for each engine tick.
		GFrameCounter++;

		// Disregard first few ticks for total tick time as it includes loading and such.
		if( GFrameCounter > 6 )
		{
			TotalTickTime+=GDeltaTime;
		}


		// Find the objects which need to be cleaned up the next frame.
		FPendingCleanupObjects* PreviousPendingCleanupObjects = PendingCleanupObjects;
		PendingCleanupObjects = GetPendingCleanupObjects();

		{
#if WITH_ENGINE
			SCOPE_CYCLE_COUNTER( STAT_FrameSyncTime );
#endif
			// this could be perhaps moved down to get greater parallelizm
			// Sync game and render thread. Either total sync or allowing one frame lag.
			static FFrameEndSync FrameEndSync;
			static auto CVarAllowOneFrameThreadLag = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.OneFrameThreadLag"));
			FrameEndSync.Sync( CVarAllowOneFrameThreadLag->GetValueOnGameThread() != 0 );
		}

		{
#if WITH_ENGINE
			SCOPE_CYCLE_COUNTER( STAT_DeferredTickTime );
#endif

			// Delete the objects which were enqueued for deferred cleanup before the previous frame.
			delete PreviousPendingCleanupObjects;

			FTicker::GetCoreTicker().Tick(GDeltaTime);

			FSingleThreadManager::Get().Tick();

			GEngine->TickDeferredCommands();		
		}

		{
			STAT(extern ENGINE_API void FinishOneFrameParticleStats());
			STAT(FinishOneFrameParticleStats());
		} 

		ENQUEUE_UNIQUE_RENDER_COMMAND(
			EndFrame,
		{
			RHIEndFrame();
			GDynamicRHI->PopEvent();
		});
	} 

	// Check for async platform hardware survey results
	GEngine->TickHardwareSurvey();

	// Set CPU utilization stats.
	const FCPUTime CPUTime = FPlatformTime::GetCPUTime();
	SET_FLOAT_STAT(STAT_CPUTimePct,CPUTime.CPUTimePct);
	SET_FLOAT_STAT(STAT_CPUTimePctRelative,CPUTime.CPUTimePctRelative);
}

void FEngineLoop::ClearPendingCleanupObjects()
{
	delete PendingCleanupObjects;
	PendingCleanupObjects = NULL;
}

#if PLATFORM_XBOXONE

void FEngineLoop::OnResuming(_In_ Platform::Object^ Sender, _In_ Platform::Object^ Args)
{
	// Make the call down to the RHI to Resume the GPU state
	RHIResumeRendering();
}

void FEngineLoop::OnSuspending(_In_ Platform::Object^ Sender, _In_ Windows::ApplicationModel::SuspendingEventArgs^ Args)
{
	// Get the Suspending Event
	Windows::ApplicationModel::SuspendingDeferral^ SuspendingEvent = Args->SuspendingOperation->GetDeferral();
	
	// @TODO ASync Call to Save the Game - This will run in Parallel to the Flush

	// Flush the RenderingThread
	FlushRenderingCommands();

	// Make the call down to the RHI to Suspend the GPU state
	RHISuspendRendering();

	// Tell the callback that we are done
	SuspendingEvent->Complete();
}

void FEngineLoop::OnResourceAvailabilityChanged( _In_ Platform::Object^ Sender, _In_ Platform::Object^ Args )
{
	// @TODO Implement as required? May be game specific
// 	// Check to see what has changed
// 	switch ( CoreApplication::ResourceAvailability )
// 	{
// 		case CoreApplication::ResourceAvailability::Constrained:
// 		{
// 		}
// 		break;
// 
// 		case CoreApplication::ResourceAvailability::Full:
// 		{
// 		}
// 		break;
// 
// 		default:
// 		{
// 			// Unknown State
// 			check(0);
// 		}
// 		break;
//	}
}

#endif // PLATFORM_XBOXONE

#endif // WITH_ENGINE


/* FEngineLoop static interface
 *****************************************************************************/

void FEngineLoop::AppInit( )
{
	// Output devices.
	GError = FPlatformOutputDevices::GetError();
	GWarn = FPlatformOutputDevices::GetWarn();

	BeginInitTextLocalization();
	FInternationalization::Initialize();

	// Avoiding potential exploits by not exposing command line overrides in the shipping games.
#if !UE_BUILD_SHIPPING && !WITH_EDITORONLY_DATA
	// 8192 is the maximum length of the command line on Windows XP.
	TCHAR CmdLineEnv[8192];

	// Retrieve additional command line arguments from environment variable.
	FPlatformMisc::GetEnvironmentVariable(TEXT("UE-CmdLineArgs"), CmdLineEnv,ARRAY_COUNT(CmdLineEnv));
	
	// Manually NULL terminate just in case. The NULL string is returned above in the error case so
	// we don't have to worry about that.
	CmdLineEnv[ARRAY_COUNT(CmdLineEnv)-1] = 0;
	FString Env = FString(CmdLineEnv).Trim();

	if (Env.Len())
	{
		// Append the command line environment after inserting a space as we can't set it in the 
		// environment. Note that any code accessing GCmdLine before appInit obviously won't 
		// respect the command line environment additions.
		FCommandLine::Append(TEXT(" -EnvAfterHere "));
		FCommandLine::Append(CmdLineEnv);
	}
#endif

	// Error history.
	FCString::Strcpy(GErrorHist, TEXT("Fatal error!" LINE_TERMINATOR LINE_TERMINATOR));

	// Platform specific pre-init.
	FPlatformMisc::PlatformPreInit();

	// Keep track of start time.
	GSystemStartTime = FDateTime::Now().ToString();

	// Switch into executable's directory.
	FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();

	// Now finish initializing the file manager after the command line is set up
	IFileManager::Get().ProcessCommandLineOptions();

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("BUILDMACHINE")))
	{
		GIsBuildMachine = true;
	}
#endif // !UE_BUILD_SHIPPING

#if PLATFORM_WINDOWS
	// update the mini dump filename now that we have enough info to point it to the log folder even in installed builds
	FCString::Strcpy(MiniDumpFilenameW, *IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FString::Printf(TEXT("%sunreal-v%i-%s.dmp"), *FPaths::GameLogDir(), GEngineVersion.GetChangelist(), *FDateTime::Now().ToString())));
#endif

	// Init logging to disk
	GLog->AddOutputDevice(FPlatformOutputDevices::GetLog());

	if (!FParse::Param(FCommandLine::Get(),TEXT("NOCONSOLE")))
	{
		GLog->AddOutputDevice(GLogConsole);
	}

	// Only create debug output device if a debugger is attached or we're running on a console or build machine
	if (!FPlatformProperties::SupportsWindowedMode() || FPlatformMisc::IsDebuggerPresent() || GIsBuildMachine)
	{
		GLog->AddOutputDevice( new FOutputDeviceDebug() );
	}

	GLog->AddOutputDevice(FPlatformOutputDevices::GetEventLog());

	// init config system
	FConfigCacheIni::InitializeConfigSystem();

	// Load "pre-init" plugin modules
	if (IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostConfigInit))
	{
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostConfigInit);
	}

	// Put the command line and config info into the suppression system
	FLogSuppressionInterface::Get().ProcessConfigAndCommandLine();

	// after the above has run we now have the REQUIRED set of engine .INIs  (all of the other .INIs)
	// that are gotten from .h files' config() are not requires and are dynamically loaded when the .u files are loaded

	// Set culture according to configuration now that configs are available.
#if ENABLE_LOC_TESTING
	if( FCommandLine::IsInitialized() && FParse::Param(FCommandLine::Get(), TEXT("LEET")) )
	{
		FInternationalization::SetCurrentCulture(TEXT("LEET"));
	}
	else
#endif
	{
		FString CultureName;
#if !UE_BUILD_SHIPPING
		// Use culture override specified on commandline.
		if (FParse::Value(FCommandLine::Get(), TEXT("CULTURE="), CultureName))
		{
			//UE_LOG(LogInit, Log, TEXT("Overriding culture %s w/ command-line option".), *CultureName);
		}
		// Use culture specified in engine configuration.
		else
#endif // !UE_BUILD_SHIPPING
		if(GConfig->GetString( TEXT("Internationalization"), TEXT("Culture"), CultureName, GEngineIni ))
		{
			//UE_LOG(LogInit, Log, TEXT("Overriding culture %s w/ engine configuration."), *CultureName);
		}
		else
		{
			CultureName = FInternationalization::GetDefaultCulture()->GetName();
		}

		FInternationalization::SetCurrentCulture(CultureName);
	}

#if !UE_BUILD_SHIPPING
	// Prompt the user for remote debugging?
	bool bPromptForRemoteDebug = false;
	GConfig->GetBool(TEXT("Engine.ErrorHandling"), TEXT("bPromptForRemoteDebugging"), bPromptForRemoteDebug, GEngineIni);
	bool bPromptForRemoteDebugOnEnsure = false;
	GConfig->GetBool(TEXT("Engine.ErrorHandling"), TEXT("bPromptForRemoteDebugOnEnsure"), bPromptForRemoteDebugOnEnsure, GEngineIni);

	if (FParse::Param(FCommandLine::Get(), TEXT("PROMPTREMOTEDEBUG")))
	{
		bPromptForRemoteDebug = true;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("PROMPTREMOTEDEBUGENSURE")))
	{
		bPromptForRemoteDebug = true;
		bPromptForRemoteDebugOnEnsure = true;
	}

	FPlatformMisc::SetShouldPromptForRemoteDebugging(bPromptForRemoteDebug);
	FPlatformMisc::SetShouldPromptForRemoteDebugOnEnsure(bPromptForRemoteDebugOnEnsure);

	// Feedback context.
	if (FParse::Param(FCommandLine::Get(), TEXT("WARNINGSASERRORS")))
	{
		GWarn->TreatWarningsAsErrors = true;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("SILENT")))
	{
		GIsSilent = true;
	}

	// Show log if wanted.
	if (GLogConsole && FParse::Param(FCommandLine::Get(), TEXT("LOG")))
	{
		GLogConsole->Show(true);
	}

#endif // !UE_BUILD_SHIPPING

	//// Command line.
	UE_LOG(LogInit, Log, TEXT("Version: %s"), *GEngineVersion.ToString());

#if PLATFORM_64BITS
	UE_LOG(LogInit, Log, TEXT("Compiled (64-bit): %s %s"), ANSI_TO_TCHAR(__DATE__), ANSI_TO_TCHAR(__TIME__));
#else
	UE_LOG(LogInit, Log, TEXT("Compiled (32-bit): %s %s"), ANSI_TO_TCHAR(__DATE__), ANSI_TO_TCHAR(__TIME__));
#endif

	UE_LOG(LogInit, Log, TEXT("Build Configuration: %s"), EBuildConfigurations::ToString(FApp::GetBuildConfiguration()));
	UE_LOG(LogInit, Log, TEXT("Branch Name: %s"), *FApp::GetBranchName() );
	UE_LOG(LogInit, Log, TEXT("Command line: %s"), FCommandLine::Get() );
	UE_LOG(LogInit, Log, TEXT("Base directory: %s"), FPlatformProcess::BaseDir() );
	//UE_LOG(LogInit, Log, TEXT("Character set: %s"), sizeof(TCHAR)==1 ? TEXT("ANSI") : TEXT("Unicode") );

	GPrintLogTimes = ELogTimes::None;

	// Determine whether to override the default setting for including timestamps in the log.
	FString LogTimes;
	if (GConfig->GetString(TEXT("LogFiles"), TEXT("LogTimes"), LogTimes, GEngineIni))
	{
		if (LogTimes == TEXT("SinceStart"))
		{
			GPrintLogTimes = ELogTimes::SinceGStartTime;
		}
		// Assume this is a bool for backward compatibility
		else if (FCString::ToBool(*LogTimes))
		{
			GPrintLogTimes = ELogTimes::UTC;
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("LOGTIMES")))
	{
		GPrintLogTimes = ELogTimes::UTC;
	}
	else if(FParse::Param(FCommandLine::Get(), TEXT("NOLOGTIMES")))
	{
		GPrintLogTimes = ELogTimes::None;
	}
	else if(FParse::Param(FCommandLine::Get(), TEXT("LOGTIMESINCESTART")))
	{
		GPrintLogTimes = ELogTimes::SinceGStartTime;
	}

	// if a logging build, clear out old log files
#if !NO_LOGGING && !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FMaintenance::DeleteOldLogs();
#endif

#if !UE_BUILD_SHIPPING
	FApp::InitializeSession();
#endif

	// initialize task graph sub-system with potential multiple threads
	FTaskGraphInterface::Startup(FPlatformMisc::NumberOfCores());
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::GameThread);

#if STATS
	FThreadStats::StartThread();
	if (FThreadStats::WillEverCollectData())
	{
		FThreadStats::ExplicitFlush(); // flush the stats and set update teh scope so we don't flush again until a frame update, this helps prevent fragmentation
	}
#endif

#if WITH_ENGINE
	// Earliest place to init the online subsystems
	// Code needs GConfigFile to be valid
	// Must be after FThreadStats::StartThread();
	// Must be before Render/RHI subsystem D3DCreate()
	// For platform services that need D3D hooks like Steam
	FModuleManager::Get().LoadModule(TEXT("OnlineSubsystem"));
	FModuleManager::Get().LoadModule(TEXT("OnlineSubsystemUtils"));
#endif

	// Checks.
	check(sizeof(uint8) == 1);
	check(sizeof(int8) == 1);
	check(sizeof(uint16) == 2);
	check(sizeof(uint32) == 4);
	check(sizeof(uint64) == 8);
	check(sizeof(ANSICHAR) == 1);

#if PLATFORM_TCHAR_IS_4_BYTES
	check(sizeof(TCHAR) == 4);
#else
	check(sizeof(TCHAR) == 2);
#endif

	check(sizeof(int16) == 2);
	check(sizeof(int32) == 4);
	check(sizeof(int64) == 8);
	check(sizeof(bool) == 1);
	check(sizeof(float) == 4);
	check(sizeof(double) == 8);
	check(GEngineNetVersion == 0 || GEngineNetVersion >= GEngineMinNetVersion);

	// Culture.
	TCHAR CookerCulture[8];
	
	if (FParse::Value(FCommandLine::Get(), TEXT("CULTUREFORCOOKING="), CookerCulture, ARRAY_COUNT(CookerCulture)))
	{
		FInternationalization::SetCurrentCulture( CookerCulture );

		// Write the culture passed in if first install...
		if (FParse::Param(FCommandLine::Get(), TEXT("firstinstall")))
		{
			GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), CookerCulture, GEngineIni);
		}
	}

	// Init list of common colors.
	GColorList.CreateColorMap();

	// Init other systems.
	FCoreDelegates::OnInit.Broadcast();
}


void FEngineLoop::AppPreExit( )
{
	UE_LOG(LogExit, Log, TEXT("Preparing to exit.") );

	FCoreDelegates::OnPreExit.Broadcast();

#if WITH_ENGINE
	if (FString(FCommandLine::Get()).Contains(TEXT("CreatePak")) && GetDerivedDataCache())
	{
		// if we are creating a Pak, we need to make sure everything is done and written before we exit
		UE_LOG(LogInit, Display, TEXT("Closing DDC Pak File."));
		GetDerivedDataCacheRef().WaitForQuiescence(true);
	}
#endif

#if WITH_EDITOR
	FRemoteConfig::Flush();
#endif

	FCoreDelegates::OnExit.Broadcast();

	// Clean up the thread pool
	if (GThreadPool != NULL)
	{
		GThreadPool->Destroy();
	}
}


void FEngineLoop::AppExit( )
{
	UE_LOG(LogExit, Log, TEXT("Exiting."));

	if (GConfig)
	{
		GConfig->Exit();
		delete GConfig;
		GConfig = NULL;
	}

	if( GLog )
	{
		GLog->TearDown();
		GLog = NULL;
	}

	FInternationalization::Terminate();
}

#undef LOCTEXT_NAMESPACE

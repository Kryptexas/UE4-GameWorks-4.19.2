// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealFrontendMain.cpp: Implements the UnrealFrontend application's main loop.
=============================================================================*/

#include "UnrealFrontendMain.h"

#include "RequiredProgramMainCPPInclude.h"
#include "AutomationController.h"
#include "DeviceManager.h"
#include "ProfilerClient.h"
#include "SessionFrontend.h"
#include "StatsData.h"
#include "EditorStyle.h"


IMPLEMENT_APPLICATION(UnrealFrontend, "UnrealFrontend");


namespace WorkspaceMenu
{
	TSharedRef<FWorkspaceItem> DeveloperTools = FWorkspaceItem::NewGroup( NSLOCTEXT("UnrealFrontend", "DeveloperToolsMenu", "Developer Tools") );
}


void RunPackageCommand()
{
	FString SourceDir;
	FParse::Value(FCommandLine::Get(), TEXT("-SOURCEDIR="), SourceDir);

	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();

		for (int32 Index = 0; Index < Platforms.Num(); ++Index)
		{
			if (Platforms[Index]->PackageBuild(SourceDir))
			{
			}
		}
	}
}


bool RunDeployCommand()
{
	bool bDeployed = false;

	// get the target device
	FString Device;
	FParse::Value(FCommandLine::Get(), TEXT("-DEVICE="), Device);

	// get the file manifest
	FString Manifest;
	FParse::Value(FCommandLine::Get(), TEXT("-MANIFEST="), Manifest);

	FString SourceDir;
	FParse::Value(FCommandLine::Get(), TEXT("-SOURCEDIR="), SourceDir);

	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (!TPM)
	{
		return false;
	}

	// Initialize the messaging subsystem so we can do device discovery.
	FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");

	// load plug-in modules
	// @todo: allow for better plug-in support in standalone Slate apps
	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreDefault);

	double DeltaTime = 0.0;
	double LastTime = FPlatformTime::Seconds();
	static int32 MasterDisableChangeTagStartFrame = -1;

	// We track the message sent time because we have to keep updating the loop until the message is *actually sent*. (ie all packets queued, sent, buffer flushed, etc.)
	double MessageSentTime = 0.0;
	bool bMessageSent = false;
	while ( !GIsRequestingExit && (MessageSentTime > LastTime + 1.0 || MessageSentTime <= 0.1) )
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FTicker::GetCoreTicker().Tick(DeltaTime);
		FPlatformProcess::Sleep(0);

		DeltaTime = FPlatformTime::Seconds() - LastTime;
		LastTime = FPlatformTime::Seconds();

		if ( !bMessageSent )
		{
			const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();

			FString Platform;
			FString DeviceName;
			Device.Split(TEXT("@"), &Platform, &DeviceName);
			FTargetDeviceId DeviceId(Platform, DeviceName);
			ITargetDevicePtr TargetDevice;
			for (int32 Index = 0; Index < Platforms.Num(); ++Index)
			{
				TargetDevice = Platforms[Index]->GetDevice(DeviceId);
				if (TargetDevice.IsValid())
				{
					FString OutId;
					if (Platforms[Index]->PackageBuild(SourceDir))
					{
						if (TargetDevice->Deploy(SourceDir, OutId))
						{
							bDeployed = true;
						}
						MessageSentTime = LastTime;
						bMessageSent = true;
					}
					else
					{
						MessageSentTime = LastTime;
						bMessageSent = true;
					}
				}
			}
		}
	}
	return bDeployed;
}


bool RunLaunchCommand(const FString& Params)
{
	bool bLaunched = false;

	// get the target device
	FString Device;
	FParse::Value(FCommandLine::Get(), TEXT("-DEVICE="), Device);

	// get the executable to launch
	FString Executable;
	FParse::Value(FCommandLine::Get(), TEXT("-EXE="), Executable);

	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (!TPM)
	{
		return false;
	}

	// Initialize the messaging subsystem so we can do device discovery.
	FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");

	// load plug-in modules
	// @todo: allow for better plug-in support in standalone Slate apps
	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreDefault);

	double DeltaTime = 0.0;
	double LastTime = FPlatformTime::Seconds();
	static int32 MasterDisableChangeTagStartFrame = -1;

	// We track the message sent time because we have to keep updating the loop until the message is *actually sent*. (ie all packets queued, sent, buffer flushed, etc.)
	double MessageSentTime = 0.0;
	bool bMessageSent = false;
	while ( !GIsRequestingExit && (MessageSentTime > LastTime + 1.0 || MessageSentTime <= 0.1) )
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FTicker::GetCoreTicker().Tick(DeltaTime);
		FPlatformProcess::Sleep(0);

		DeltaTime = FPlatformTime::Seconds() - LastTime;
		LastTime = FPlatformTime::Seconds();

		if ( !bMessageSent )
		{
			const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();

			FString Platform;
			FString DeviceName;
			Device.Split(TEXT("@"), &Platform, &DeviceName);
			FTargetDeviceId DeviceId(Platform, DeviceName);
			ITargetDevicePtr TargetDevice;
			for (int32 Index = 0; Index < Platforms.Num(); ++Index)
			{
				TargetDevice = Platforms[Index]->GetDevice(DeviceId);
				if (TargetDevice.IsValid())
				{
					uint32 OutId;
					if (TargetDevice->Run(Executable, Params, &OutId))
					{
						MessageSentTime = LastTime;
						bMessageSent = true;
						bLaunched = true;
					}
					else
					{
						MessageSentTime = LastTime;
						bMessageSent = true;
					}
				}
			}
		}
	}

	return bLaunched;
}


void WriteString(FArchive* File, const ANSICHAR* Format, ...)
{
	if( File != NULL )
	{
		check(File);
		ANSICHAR Array[1024];
		va_list ArgPtr;
		va_start(ArgPtr,Format);
		// Build the string
		int32 Result = FCStringAnsi::GetVarArgs(Array,ARRAY_COUNT(Array),ARRAY_COUNT(Array)-1,Format,ArgPtr);
		// Now write that to the file
		File->Serialize((void*)Array,Result);
	}
}


void RunStatsConvertCommand()
{
	// get the target file
	FString TargetFile;
	FParse::Value(FCommandLine::Get(), TEXT("-INFILE="), TargetFile);
	FString OutFile;
	FParse::Value(FCommandLine::Get(), TEXT("-OUTFILE="), OutFile);
	FString StatListString;
	FParse::Value(FCommandLine::Get(), TEXT("-STATLIST="), StatListString);

	// get the list of stats
	TArray<FString> StatList;
	if (StatListString.ParseIntoArray(&StatList, TEXT("+"), true) == 0)
	{
		StatList.Add(TEXT("STAT_FrameTime"));
	}

	// attempt to read the data and convert to csv
	FArchive* FileReader = IFileManager::Get().CreateFileReader(*TargetFile);
	if (!FileReader)
	{
		return;
	}

	// open a csv file for write
	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*OutFile);
	if (!FileWriter)
	{
		FileReader->Close();
		return;
	}

	// output the csv header
	WriteString(FileWriter, "Frame,Name,Value\r\n");

	// header magic
	uint32 Magic = 0;
	*FileReader << Magic;

	// read in the data
	FStatsReadStream Stream;
	TArray<FStatMessage> Messages;
	FStatsThreadState ThreadState;
	while(FileReader->Tell() < FileReader->TotalSize())
	{
		// read the message
		FStatMessage Message(Stream.ReadMessage(*FileReader));
		if (Message.NameAndInfo.GetShortName() != TEXT("Unknown FName"))
		{
			if (Message.NameAndInfo.GetField<EStatOperation>() == EStatOperation::AdvanceFrameEventGameThread)
			{
				new (Messages) FStatMessage(Message);
				ThreadState.AddMessages(Messages);
				Messages.Reset(Messages.Num());

				// get the frame time and the render time
				if (ThreadState.CurrentGameFrame > 1)
				{
					// get the thread stats
					TArray<FStatMessage> Stats;
					ThreadState.GetInclusiveAggregateStackStats(ThreadState.CurrentGameFrame, Stats);
					for (int32 Index = 0; Index < Stats.Num(); ++Index)
					{
						FStatMessage const& Meta = Stats[Index];
//						UE_LOG(LogTemp, Display, TEXT("Stat: %s"), *Meta.NameAndInfo.GetShortName().ToString());

						for (int32 Jndex = 0; Jndex < StatList.Num(); ++Jndex)
						{
							if (Meta.NameAndInfo.GetShortName().ToString() == StatList[Jndex])
							{
								float CycleTime = 0.0f;
								if (Meta.NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration))
								{
									CycleTime = FPlatformTime::ToMilliseconds( FromPackedCallCountDuration_Duration(Meta.GetValue_int64()) );
								}
								else if (Meta.NameAndInfo.GetFlag(EStatMetaFlags::IsCycle))
								{
									CycleTime = FPlatformTime::ToMilliseconds( Meta.GetValue_int64() );
								}
								else
								{
									CycleTime = Meta.GetValue_int64();
								}

								// write out to the csv file
								WriteString(FileWriter, "%d,%S,%f\r\n", ThreadState.CurrentGameFrame, *StatList[Jndex], CycleTime);
							}
						}
					}
				}
			}

			new (Messages) FStatMessage(Message);
		}
		else
		{
			break;
		}
	}

	FileWriter->Close();
	FileReader->Close();
	delete FileWriter;
	delete FileReader;
}


void RunUI()
{
	FString UnrealFrontendUserSettingsIni = FPaths::GetPath(GEngineIni) + "/UnrealFrontend.ini";

	FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());

	// The frontend currently relies on EditorStyle being loaded
	FModuleManager::LoadModuleChecked<IEditorStyleModule>("EditorStyle");

	// initialize messaging subsystem
	FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");

	// load plug-in modules
	// @todo: allow for better plug-in support in standalone Slate apps
	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreDefault);

	// initialize automation tests
	IAutomationControllerModule& AutomationControllerModule = FModuleManager::LoadModuleChecked<IAutomationControllerModule>("AutomationController");
	AutomationControllerModule.Init();

	// initialize profiler
	FModuleManager::Get().LoadModule("ProfilerClient");

	// initialize user interface
	FModuleManager::Get().LoadModule("DeviceManager");
	FModuleManager::Get().LoadModule("SessionFrontend");
	FModuleManager::Get().LoadModule("SessionLauncher");
	FModuleManager::Get().LoadModule("SettingsEditor");

	// Create developer tools menu with widget reflector.
	FSlateApplication::Get().RegisterWidgetReflectorTabSpawner(WorkspaceMenu::DeveloperTools);

	// check to see if want the widget reflector
	const bool bAllowDebugTools = FParse::Param(FCommandLine::Get(), TEXT("DebugTools"));

	// set the application name
	FGlobalTabmanager::Get()->SetApplicationTitle(NSLOCTEXT("UnrealFrontend", "AppTitle", "Unreal Frontend"));

	// restore application layout
	TSharedRef<FTabManager::FLayout> NewLayout = FTabManager::NewLayout("SessionFrontendLayout_v1.1")
		->AddArea
		(
			FTabManager::NewArea(1280.f, 720.0f)
				->Split
				(
					FTabManager::NewStack()
						->AddTab(FName("DeviceManager"), ETabState::OpenedTab)
						->AddTab(FName("MessagingDebugger"), ETabState::ClosedTab)
						->AddTab(FName("SessionLauncher"), ETabState::OpenedTab)
						->AddTab(FName("SessionFrontend"), ETabState::OpenedTab)
				)
		)
		->AddArea
		(
			FTabManager::NewArea(600.0f, 600.0f)
				->SetWindow(FVector2D(10.0f, 10.0f), false)
				->Split
				(
					FTabManager::NewStack()->AddTab("WidgetReflector", bAllowDebugTools ? ETabState::OpenedTab : ETabState::ClosedTab)
				)
		);

	TSharedRef<FTabManager::FLayout> UserConfiguredNewLayout = FLayoutSaveRestore::LoadFromConfig(NewLayout, UnrealFrontendUserSettingsIni);
	FGlobalTabmanager::Get()->RestoreFrom(UserConfiguredNewLayout, TSharedPtr<SWindow>());

	// enter main loop
	double DeltaTime = 0.0;
	double LastTime = FPlatformTime::Seconds() - 0.0001;
	static int32 MasterDisableChangeTagStartFrame = -1;

	while (!GIsRequestingExit)
	{
		//Save the state of the tabs here rather than after close of application (the tabs are undesirably saved out with ClosedTab state on application close)
		//UserConfiguredNewLayout = FGlobalTabmanager::Get()->PersistLayout();

		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
		FTicker::GetCoreTicker().Tick(DeltaTime);
		AutomationControllerModule.Tick();

		FPlatformProcess::Sleep(0.0f);

		double CurrentTime = FPlatformTime::Seconds();
		DeltaTime =  CurrentTime - LastTime;
		LastTime = CurrentTime;

#if STATS
		FThreadStats::ExplicitFlush();
		FThreadStats::WaitForStats();
		MasterDisableChangeTagStartFrame = FThreadStats::MasterDisableChangeTag();
#endif

		GLog->FlushThreadedLogs();
	}

	// save application layout
	FLayoutSaveRestore::SaveToConfig(UserConfiguredNewLayout, UnrealFrontendUserSettingsIni);
	GConfig->Flush(false, UnrealFrontendUserSettingsIni);

	// shut down application
	FSlateApplication::Shutdown();
}


int32 UnrealFrontendMain( const TCHAR* CommandLine )
{
	// check to see if we should run in command line mode
	FString Command;
	FString Params;
	FString NewCommandLine = CommandLine;

	bool bRunCommand = FParse::Value(*NewCommandLine, TEXT("-RUN="), Command);
	
	if (bRunCommand)
	{
		// extract off any -PARAM= parameters so that they aren't accidentally parsed by engine init
		FParse::Value(*NewCommandLine, TEXT("-PARAMS="), Params);
		if (Params.Len() > 0)
		{
			// remove from the command line
			NewCommandLine = NewCommandLine.Replace(*Params, TEXT(""));

			// trim the quotes
			Params = Params.TrimQuotes();
		}
	}

	if (!FParse::Param(*NewCommandLine, TEXT("-Messaging")))
	{
		NewCommandLine += TEXT(" -Messaging");
	}

	GEngineLoop.PreInit(*NewCommandLine);

	// Tell the module manager is may now process newly-loaded UObjects when new C++ modules are loaded
	FModuleManager::Get().StartProcessingNewlyLoadedObjects();

	bool bReturnValue = true;
	if (bRunCommand)
	{
		if (Command.Equals(TEXT("PACKAGE"), ESearchCase::IgnoreCase))
		{
			RunPackageCommand();
		}
		else if (Command.Equals(TEXT("DEPLOY"), ESearchCase::IgnoreCase))
		{
			bReturnValue = RunDeployCommand();
		}
		else if (Command.Equals(TEXT("LAUNCH"), ESearchCase::IgnoreCase))
		{
			bReturnValue = RunLaunchCommand(Params);
		}
		else if (Command.Equals(TEXT("CONVERT"), ESearchCase::IgnoreCase))
		{
			RunStatsConvertCommand();
		}
	}
	else
	{
		RunUI();
	}

	FEngineLoop::AppPreExit();
	FModuleManager::Get().UnloadModulesAtShutdown();

#if STATS
	FThreadStats::StopThread();
#endif

	FTaskGraphInterface::Shutdown();

	return bReturnValue ? 0 : -1;
}

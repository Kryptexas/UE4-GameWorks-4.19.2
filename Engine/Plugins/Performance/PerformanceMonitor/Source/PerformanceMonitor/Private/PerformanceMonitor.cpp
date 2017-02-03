// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

//#include "OrionGame.h"
#include "PerformanceMonitor.h"
#include "CoreGlobals.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"

#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Engine/GameViewportClient.h"
#include "ConfigCacheIni.h"

#define SUPER_DETAILED_AUTOMATION_STATS 1
PERFORMANCEMONITOR_API int ExportedInt;
IMPLEMENT_MODULE(FPerformanceMonitorModule, PerformanceMonitor);
FPerformanceMonitorModule::FPerformanceMonitorModule()
{
	bRecording = false;
	FileToLogTo = nullptr;

}

FPerformanceMonitorModule::~FPerformanceMonitorModule()
{
	if (FileToLogTo)
	{
		FileToLogTo->Close();
	}
}

bool FPerformanceMonitorModule::Tick(float DeltaTime)
{
#if STATS

	if (bRecording)
	{
		float CurrentTime = FPlatformTime::Seconds();
		if (CurrentTime - TimeBetweenRecords > TimeOfLastRecord)
		{
			RecordFrame();
			TimeOfLastRecord = CurrentTime;
		}
	}
#endif
	return true;
}

void FPerformanceMonitorModule::StartupModule()
{

}

void FPerformanceMonitorModule::ShutdownModule()
{

}

void FPerformanceMonitorModule::Init()
{
	
}

void FPerformanceMonitorModule::SetRecordInterval(float NewInterval)
{
	TimeBetweenRecords = NewInterval;
}

bool FPerformanceMonitorModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if STATS
	// Ignore any execs that don't start with Lobby
	if (FParse::Command(&Cmd, TEXT("PerformanceMonitor")))
	{
		if (FParse::Command(&Cmd, TEXT("start")))
		{
			if (!bRecording)
			{
				FString StringCommand = Cmd;
				if (!StringCommand.IsEmpty())
				{
					StartRecordingPerfTimers(StringCommand, DesiredStats);
					TickHandler = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FPerformanceMonitorModule::Tick));
				}
			}
			else
			{
				Ar.Logf(TEXT("PerformanceMonitor is already running!"));
			}
		}
		else if (FParse::Command(&Cmd, TEXT("stop")))
		{
			if (bRecording)
			{
				FTicker::GetCoreTicker().RemoveTicker(TickHandler);
				StopRecordingPerformanceTimers();
			}
			else
			{
				Ar.Logf(TEXT("PerformanceMonitor can't stop because it isn't running!"));
			}

		}
		else if (FParse::Command(&Cmd, TEXT("addtimer")))
		{
			
			FString StringCommand = Cmd;
			if (!StringCommand.IsEmpty())
			{
				DesiredStats.AddUnique(StringCommand);
			}
		}
		else if (FParse::Command(&Cmd, TEXT("setinterval")))
		{
			FString StringCommand = Cmd;
			if (!StringCommand.IsEmpty())
			{
				float NewTime = FCString::Atof(*StringCommand);
				if (NewTime)
				{
					TimeBetweenRecords = NewTime;
				}
			}
		}
		else
		{
			Ar.Logf(TEXT("Incorrect PerformanceMonitor command syntax! Supported commands are: "));
			Ar.Logf(TEXT("\tPerformanceMonitor start <filename as string>"));
			Ar.Logf(TEXT("\tPerformanceMonitor stop"));
			Ar.Logf(TEXT("\tPerformanceMonitor setinterval <seconds as float>"));
			Ar.Logf(TEXT("\tAutomation addtimer <timername as string>"));
		}


		return true;
	}
#endif
	return false;
}



void FPerformanceMonitorModule::StartRecordingPerfTimers(FString FileNameToUse, TArray<FString> StatsToRecord)
{
#if STATS

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType == EWorldType::Game || WorldContext.WorldType == EWorldType::PIE)
		{
			UWorld* World = WorldContext.World();
			UGameViewportClient* ViewportClient = World->GetGameViewport();
			
			GEngine->SetEngineStat(World, ViewportClient, TEXT("Unit"), true);
			GEngine->SetEngineStat(World, ViewportClient, TEXT("Particles"), true);
			GEngine->SetEngineStat(World, ViewportClient, TEXT("Anim"), true);
			GEngine->SetEngineStat(World, ViewportClient, TEXT("GpuParticles"), true);
			break;
		}
	}
	if (bRecording)
	{
		GLog->Log(TEXT("PerformanceMonitor"), ELogVerbosity::Warning, TEXT("Tried to start recording when we already have started! Don't do that."));
		return;
	}
	FThreadStats::MasterEnableAdd(1);
	if (FileNameToUse == TEXT(""))	
	{
		GLog->Log(TEXT("PerformanceMonitor"), ELogVerbosity::Warning, TEXT("Please set a file name."));
		LogFileName = TEXT("UnnamedPerfData");
	}

	if (LogFileName != FileNameToUse)
	{
		if (FileToLogTo)
		{
			FileToLogTo->Close();
		}
		LogFileName = FileNameToUse;

		FString NewLogFileName = FString::Printf(TEXT("%sFXPerformance/%s.csv"), *FPaths::GameSavedDir(), *LogFileName);
		FileToLogTo = IFileManager::Get().CreateFileWriter(*NewLogFileName, false);
	}

	if (!FileToLogTo)
	{
		FString FileName = FString::Printf(TEXT("%s.csv"), *FileNameToUse);
		FileToLogTo =  IFileManager::Get().CreateFileWriter(*FileName, false);
	}
	if (GConfig)
	{
		FString ConfigCategory = FString::Printf(TEXT("/Plugins/PerformanceMonitor/%s"), *FileNameToUse);
		float floatValueReceived = 0;
		if (GConfig->GetFloat(*ConfigCategory,
			TEXT("PerformanceMonitorInterval"),
			floatValueReceived,
			GGameIni
			))
		{
			TimeBetweenRecords = floatValueReceived;
		}
		floatValueReceived = 0;
		if (GConfig->GetFloat(*ConfigCategory,
			TEXT("PerformanceMonitorTimeout"),
			floatValueReceived,
			GGameIni
			))
		{
			TestTimeOut = floatValueReceived;
		}
		TArray<FString> TimersOfInterest;
		if (GConfig->GetArray(*ConfigCategory,
			TEXT("PerformanceMonitorTimers"), TimersOfInterest, GGameIni))
		{
			DesiredStats = TimersOfInterest;
		}
		FString MapToLoad;
		if (GConfig->GetString(*ConfigCategory,
			TEXT("PerformanceMonitorMap"), MapToLoad, GGameIni))
		{
			MapToTest = MapToLoad;
		}
		bExitOnCompletion = false;
		bool GatheredBool;
		if (GConfig->GetBool(*ConfigCategory,
			TEXT("PerformanceMonitorExitOnFinish"), GatheredBool, GGameIni))
		{
			bExitOnCompletion = GatheredBool;
		}
		bRequiresCutsceneStart = false;
		GatheredBool = false;
		if (GConfig->GetBool(*ConfigCategory,
			TEXT("PerformanceMonitorRequireCutsceneStart"), GatheredBool, GGameIni))
		{
			bRequiresCutsceneStart = GatheredBool;
		}
	}
	if (!StatsToRecord.Num() && !DesiredStats.Num())
	{
		DesiredStats.Add(TEXT("STAT_AnimGameThreadTime"));
		DesiredStats.Add(TEXT("STAT_PerformAnimEvaluation_WorkerThread"));
		DesiredStats.Add(TEXT("STAT_ParticleComputeTickTime"));
		DesiredStats.Add(TEXT("STAT_ParticleRenderingTime"));
		DesiredStats.Add(TEXT("STAT_GPUParticleTickTime"));
	}
	GeneratedStats.Empty();
	for (int i = 0; i < DesiredStats.Num(); i++)
	{
		TArray<float> TempArray;
		GeneratedStats.Add(DesiredStats[i], TempArray);
	}
	bRecording= true;
	TimeOfTestStart = FPlatformTime::Seconds();
	if (bRequiresCutsceneStart)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Game || WorldContext.WorldType == EWorldType::PIE)
			{
				UWorld* World = WorldContext.World();
				GEngine->Exec(World, TEXT("ce start"));
			}
		}
	}
#endif
}


void FPerformanceMonitorModule::RecordFrame()
{
	if (!bRecording)
	{
		return;
	}
	GetStatsBreakdown();
	if (FPlatformTime::Seconds() - TestTimeOut > TimeOfTestStart)
	{
		StopRecordingPerformanceTimers();
	}

}

void FPerformanceMonitorModule::GetStatsBreakdown()
{
#if STATS
	FStatsThreadState& Stats = FStatsThreadState::GetLocalState();
	int curFrame = Stats.GetLatestValidFrame();
	if (curFrame >= 0)
	{
		StoredMessages.Add(Stats.GetCondensedHistory(curFrame));
	}
#endif
}


void FPerformanceMonitorModule::FinalizeFTestPerfReport()
{
#if STATS
	if (bRecording)
	{
		StopRecordingPerformanceTimers();
	}
	if (FileToLogTo)
	{
		FileToLogTo->Close();
	}
	LogFileName = TEXT("");
#endif
}

void FPerformanceMonitorModule::StopRecordingPerformanceTimers()
{
#if STATS
	if (!bRecording)
	{
		GLog->Log(TEXT("FTestPerfRecorder"), ELogVerbosity::Warning, TEXT("Tried to End Recording when we haven't started recording! Don't do that."));
		return;
	}
	RecordData();
	FThreadStats::MasterEnableSubtract(1);
	bRecording = false;
	if (!GeneratedStats.Num())
	{
		bRecording = false;
		GLog->Log(TEXT("FTestPerfRecorder"), ELogVerbosity::Warning, TEXT("No perf data to record. "));
		return;
	}
	if (FileToLogTo)
	{
		FileToLogTo->Close();
	}


	if (bExitOnCompletion)
	{
		FPlatformMisc::RequestExit(true);
	}
#endif
}

void FPerformanceMonitorModule::RecordData()
{
#if STATS


	FString StringToPrint = FString::Printf(TEXT("Interval (s),%0.4f\n"), TimeBetweenRecords);
	FileToLogTo->Serialize(TCHAR_TO_ANSI(*StringToPrint), StringToPrint.Len());
	for (int i = 0; i < StoredMessages.Num(); i++)
	{
		TArray<FStatMessage> CurrentFrame = StoredMessages[i];
		TArray<FString> StatsCoveredThisFrame = DesiredStats;
		for (int j = 0; j < CurrentFrame.Num(); j++)
		{
			FStatMessage TempMessage = CurrentFrame[j];
			FString StatName = TempMessage.NameAndInfo.GetShortName().ToString();
			if (StatsCoveredThisFrame.Contains(StatName))
			{
				TArray<float> ArrayForStatName = GeneratedStats.FindOrAdd(StatName);
				ArrayForStatName.Add(FPlatformTime::ToMilliseconds(TempMessage.GetValue_Duration()));
				GeneratedStats.Emplace(StatName, ArrayForStatName);
				StatsCoveredThisFrame.Remove(StatName);
			}			
		}
		for (int j = 0; j < StatsCoveredThisFrame.Num(); j++)
		{
			TArray<float> ArrayForStatName = GeneratedStats.FindOrAdd(StatsCoveredThisFrame[j]);
			ArrayForStatName.Add(0.f);
			GeneratedStats.Emplace(StatsCoveredThisFrame[j], ArrayForStatName);
		}
	}
	for (TMap<FString, TArray<float>>::TConstIterator ThreadedItr(GeneratedStats); ThreadedItr; ++ThreadedItr)
	{
				StringToPrint = ThreadedItr.Key();
				TArray<float> StatValues = ThreadedItr.Value();
				for (int i = 0; i < StatValues.Num(); i++)
				{
					StringToPrint = FString::Printf(TEXT("%s,%0.4f"), *StringToPrint, StatValues[i]);
				}
				StringToPrint.Append(TEXT("\n"));
				FileToLogTo->Serialize(TCHAR_TO_ANSI(*StringToPrint), StringToPrint.Len());
	}

#endif
}

void FPerformanceMonitorModule::CleanUpPerfFileHandles()
{
#if STATS

	if (FileToLogTo)
	{
		FileToLogTo->Close();
	}
	LogFileName.Empty();
	DesiredStats.Empty();
#endif
}
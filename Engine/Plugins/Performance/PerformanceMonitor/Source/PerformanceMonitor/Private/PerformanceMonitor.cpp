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



#if STATS
void FPerformanceMonitorModule::StartRecordingPerfTimers(FString FileNameToUse, TArray<FString> StatsToRecord)
{
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
	if (!StatsToRecord.Num())
	{
		DesiredStats.Add(TEXT("STAT_AnimGameThreadTime"));
		DesiredStats.Add(TEXT("STAT_PerformAnimEvaluation_WorkerThread"));
		DesiredStats.Add(TEXT("STAT_ParticleComputeTickTime"));
		DesiredStats.Add(TEXT("STAT_ParticleRenderingTime"));
		DesiredStats.Add(TEXT("STAT_GPUParticleTickTime"));
	}
	else
	{
		DesiredStats = StatsToRecord;
	}
	GeneratedStats.Empty();
	for (int i = 0; i < DesiredStats.Num(); i++)
	{
		TArray<float> TempArray;
		GeneratedStats.Add(DesiredStats[i], TempArray);
	}
	bRecording= true;
	//AOrionPlayerController_Game* PlayerController = UOrionAutomationHelper::GetPlayerGameController();
	//if (PlayerController)
	//{
	//	UWorld * PlayerWorld = PlayerController->GetWorld();
	//	if (PlayerWorld)
	//	{
	//		// Postpone garbage collection for five minutes or until test ends, whichever comes first.
	//		PlayerWorld->SetTimeUntilNextGarbageCollection(300.f);
	//	}
	//}
}

#endif
void FPerformanceMonitorModule::RecordFrame()
{
	if (!bRecording)
	{
		return;
	}
	GetStatsBreakdown();
	//#if STATS
	//	FOrionCharacterPerfEvent::GetStatsBreakdown(PerfEvent);
	//	AOrionPlayerController_Game* PlayerController = UOrionAutomationHelper::GetPlayerGameController();
	//	AOrionCharHero * MyPawn = nullptr;
	//	if (PlayerController != nullptr)
	//	{
	//		MyPawn = Cast<AOrionCharHero>(PlayerController->GetPawn());
	//	}
	//	// Ensure we have a player controller and a pawn
	//	if ((PlayerController == nullptr) || (MyPawn == nullptr))
	//	{
	//		//we need these !!
	//		return;
	//	}
	//
	//	float CurrentPlatformTime = FPlatformTime::Seconds();
	//	UWorld* CurWorld = PlayerController->GetWorld();
	//	UGameViewportClient* Viewport = nullptr;
	//	for (int i = 0; i < GEngine->GetWorldContexts().Num(); i++)
	//	{
	//		if (GEngine->GetWorldContexts()[i].WorldType == EWorldType::Game || GEngine->GetWorldContexts()[i].WorldType == EWorldType::PIE)
	//		{
	//			Viewport = GEngine->GetWorldContexts()[i].World()->GetGameViewport();
	//			// If this viewport is actually valid, we break. Check to make sure we're not getting DS world by mistake.
	//			if (Viewport)
	//			{
	//				break;
	//			}
	//		}
	//	}
	//	if (Viewport != nullptr)
	//	{
	//		const FStatUnitData* StatUnitData = Viewport->GetStatUnitData();
	//
	//			PerfEvent->OverallFrameTimes.FrameDurations.Add(StatUnitData->FrameTime);
	//			PerfEvent->OverallFrameTimes.FrameGameTimes.Add(FGenericPlatformTime::ToMilliseconds(GGameThreadTime));
	//			PerfEvent->OverallFrameTimes.FrameRenderTimes.Add(FGenericPlatformTime::ToMilliseconds(GRenderThreadTime));
	//			PerfEvent->OverallFrameTimes.FrameTimeStamps.Add(CurrentPlatformTime);
	//			PerfEvent->FrameNum++;
	//	
	//
	//	}
	//
	//#endif
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

	// TODO: Force GC. 
	//if (PlayerController)
	//{
	//	UWorld * PlayerWorld = PlayerController->GetWorld();
	//	if (PlayerWorld)
	//	{
	//		// End our self imposed moratorium on GC.
	//		PlayerWorld->ForceGarbageCollection();
	//	}
	//}
#endif
	return;
}

void FPerformanceMonitorModule::RecordData()
{
#if STATS


	//TSharedPtr<FJsonObject> ReportJson;

	//// FrameDurations
	//EventToRecord.StatName = TEXT("FrameDuration");
	//EventToRecord.MinVal = FMath::Min(PerfEvent->OverallFrameTimes.FrameDurations);
	//EventToRecord.AvgVal = UOrionAutomationHelper::GetAverageOfArray(PerfEvent->OverallFrameTimes.FrameDurations);
	//EventToRecord.MaxVal = FMath::Max(PerfEvent->OverallFrameTimes.FrameDurations);
	//EventToRecord.NumFrames = PerfEvent->OverallFrameTimes.FrameDurations.Num();

	//ReportJson = FJsonObjectConverter::UStructToJsonObject(EventToRecord);
	//RecordPerfEvent();

	//// GameThread
	//EventToRecord.StatName = TEXT("GameThreadTime");
	//EventToRecord.MinVal = FMath::Min(PerfEvent->OverallFrameTimes.FrameGameTimes);
	//EventToRecord.AvgVal = UOrionAutomationHelper::GetAverageOfArray(PerfEvent->OverallFrameTimes.FrameGameTimes);
	//EventToRecord.MaxVal = FMath::Max(PerfEvent->OverallFrameTimes.FrameGameTimes);
	//EventToRecord.NumFrames = PerfEvent->OverallFrameTimes.FrameGameTimes.Num();

	//ReportJson = FJsonObjectConverter::UStructToJsonObject(EventToRecord);
	//RecordPerfEvent();


	//// RenderThread
	//EventToRecord.StatName = TEXT("RenderThreadTime");
	//EventToRecord.MinVal = FMath::Min(PerfEvent->OverallFrameTimes.FrameRenderTimes);
	//EventToRecord.AvgVal = UOrionAutomationHelper::GetAverageOfArray(PerfEvent->OverallFrameTimes.FrameRenderTimes);
	//EventToRecord.MaxVal = FMath::Max(PerfEvent->OverallFrameTimes.FrameRenderTimes);
	//EventToRecord.NumFrames = PerfEvent->OverallFrameTimes.FrameRenderTimes.Num();
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
	//for (TMap<FString, TMap<FString, TArray<float>>>::TConstIterator ThreadedItr(GeneratedStats); ThreadedItr; ++ThreadedItr)
	//{
	//	const TMap<FString, TArray<float>> ThreadGroup = ThreadedItr.Value();
	//	for (TMap<FString, TArray<float>>::TConstIterator ThreadedItr2(ThreadGroup); ThreadedItr2; ++ThreadedItr2)
	//	{
	//		FString StringToPrint = ThreadedItr2.Key();
	//		TArray<float> StatValues = ThreadedItr2.Value();
	//		for (int i = 0; i < StatValues.Num(); i++)
	//		{
	//			StringToPrint = FString::Printf(TEXT("%s,%0.4f"), *StringToPrint, StatValues[i]);
	//		}
	//		StringToPrint.Append(TEXT("\n"));
	//		FileToLogTo->Serialize(TCHAR_TO_ANSI(*StringToPrint), StringToPrint.Len());
	//	}
	//}
	//for (TMap<FString, TMap<FString, TArray<FStatData>>>::TConstIterator ThreadedItr(PerfEvent->ThreadStatBreakdown); ThreadedItr; ++ThreadedItr)
	//{
	//	const FName ThreadName = ThreadedItr.Key();
	//	const TMap<FName, TArray<FStatData>>& ThreadBreakdown = ThreadedItr.Value();
	//	for (TMap<FName, TArray<FStatData>>::TConstIterator StatItr(ThreadBreakdown); StatItr; ++StatItr)
	//	{
	//		const FName StatName = StatItr.Key();
	//		if (!DesiredStats.Contains(StatName.ToString()))
	//		{
	//			continue;
	//		}
	//		FString ThreadStatKeyAvg = FString::Printf(TEXT("AvgInclusive %s [%s]"), *ThreadName.ToString(), *StatName.ToString());
	//		FString ThreadStatKeyMax = FString::Printf(TEXT("MaxInclusive %s [%s]"), *ThreadName.ToString(), *StatName.ToString());
	//		FString StatAvgString;
	//		FString StatMaxString;
	//		TArray<float> StatAvgs;
	//		TArray<float> StatMaxes;
	//		const TArray<FStatData>& StatDatas = StatItr.Value();
	//		int ValidFrame = 0;
	//		for (int FrameNum = 0; FrameNum <= PerfEvent->FrameNum; ++FrameNum)
	//		{
	//			float AvgTime = -1.f;
	//			float MaxTime = -1.f;
	//			if (ValidFrame < StatDatas.Num())
	//			{
	//				const FStatData& StatData = StatDatas[ValidFrame];
	//				if (StatData.FrameNum == FrameNum)
	//				{
	//					AvgTime = StatData.StatIncAvg;
	//					MaxTime = StatData.StatIncMax;
	//					++ValidFrame;
	//				}
	//			}

	//			if (AvgTime != -1.f)
	//			{
	//				StatAvgs.Add(AvgTime);
	//			}
	//			if (MaxTime != -1.f)
	//			{
	//				StatMaxes.Add(MaxTime);
	//			}
	//		}
	//		//float AvgAverage = UOrionAutomationHelper::GetAverageOfArray(StatAvgs);
	//		//float AvgMaxes = UOrionAutomationHelper::GetAverageOfArray(StatMaxes);

	//		//// StatMaxes
	//		//EventToRecord.StatName = FString::Printf(TEXT("%s_MAX"), *StatName.ToString());
	//		//EventToRecord.MinVal = FMath::Min(StatMaxes);
	//		//EventToRecord.AvgVal = AvgMaxes;
	//		//EventToRecord.MaxVal = FMath::Max(StatMaxes);
	//		//EventToRecord.NumFrames = StatMaxes.Num();
	//		//ReportJson = FJsonObjectConverter::UStructToJsonObject(EventToRecord);
	//		//RecordPerfEvent();

	//		//// StatAvgs
	//		//EventToRecord.StatName = FString::Printf(TEXT("%s_AVG"), *StatName.ToString());
	//		//EventToRecord.MinVal = FMath::Min(StatAvgs);
	//		//EventToRecord.AvgVal = AvgAverage;
	//		//EventToRecord.MaxVal = FMath::Max(StatAvgs);
	//		//EventToRecord.NumFrames = StatAvgs.Num();
	//		//ReportJson = FJsonObjectConverter::UStructToJsonObject(EventToRecord);
	//		//RecordPerfEvent();
	//	}
	//}
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
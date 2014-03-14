// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "CollisionDebugDrawingPublic.h"


//////////////////////////////////////////////////////////////////////////

void FLogVisualizer::SummonUI(UWorld* InWorld) 
{
	UE_LOG(LogLogVisualizer, Log, TEXT("Opening LogVisualizer..."));

	if( IsInGameThread() )
	{
		if (LogWindow.IsValid() && World.IsValid() && World == InWorld)
		{
			return;
		}

		World = InWorld;
		FVisualLog* VisualLog = FVisualLog::Get();
		VisualLog->RegisterNewLogsObserver(FVisualLog::FOnNewLogCreatedDelegate::CreateRaw(this, &FLogVisualizer::OnNewLog));
		PullDataFromVisualLog(VisualLog);

		// Give window to slate
		if (!LogWindow.IsValid())
		{
			// Make a window
			TSharedRef<SWindow> NewWindow = SNew(SWindow)
				.ClientSize(FVector2D(720,768))
				.Title( NSLOCTEXT("LogVisualizer", "WindowTitle", "Log Visualizer") )
				[
					SNew(SLogVisualizer, this)
				];

			LogWindow = FSlateApplication::Get().AddWindow(NewWindow);
		}
		
		//@TODO fill Logs array with whatever is there in FVisualLog instance
	}
	else
	{
		UE_LOG(LogLogVisualizer, Warning, TEXT("FLogVisualizer::SummonUI: Not in game thread."));
	}
}

void FLogVisualizer::CloseUI(UWorld* InWorld) 
{
	UE_LOG(LogLogVisualizer, Log, TEXT("Opening LogVisualizer..."));

	if( IsInGameThread() )
	{
		if (LogWindow.IsValid() && (World.IsValid() == false || World == InWorld))
		{
			CleanUp();
			FSlateApplication::Get().RequestDestroyWindow(LogWindow.Pin().ToSharedRef());
		}
	}
	else
	{
		UE_LOG(LogLogVisualizer, Warning, TEXT("FLogVisualizer::CloseUI: Not in game thread."));
	}
}

void FLogVisualizer::CleanUp()
{
	FVisualLog::Get()->ClearNewLogsObserver();
}

void FLogVisualizer::PullDataFromVisualLog(FVisualLog* VisualLog)
{
	if (VisualLog == NULL)
	{
		return;
	}

	Logs.Reset();
	const FVisualLog::FLogsMap* LogsMap = VisualLog->GetLogs();
	for (FVisualLog::FLogsMap::TConstIterator MapIt(*LogsMap); MapIt; ++MapIt)
	{
		Logs.Add(MapIt.Value());
		LogAddedEvent.Broadcast();
	}
}

void FLogVisualizer::OnNewLog(const AActor* Actor, TSharedPtr<FActorsVisLog> Log)
{
	Logs.Add(Log);
	LogAddedEvent.Broadcast();
}

void FLogVisualizer::AddLoadedLog(TSharedPtr<FActorsVisLog> Log)
{
	Logs.Add(Log);
	LogAddedEvent.Broadcast();
}

bool FLogVisualizer::IsRecording()
{
	return FVisualLog::Get()->IsRecording();
}

void FLogVisualizer::SetIsRecording(bool bNewRecording)
{
	FVisualLog::Get()->SetIsRecording(bNewRecording);
}

int32 FLogVisualizer::GetLogIndexForActor(const AActor* Actor)
{
	int32 ResultIndex = INDEX_NONE;
	const FString FullName = Actor->GetFullName();
	TSharedPtr<FActorsVisLog>* Log = Logs.GetTypedData();
	for (int32 LogIndex = 0; LogIndex < Logs.Num(); ++LogIndex, ++Log)
	{
		if ((*Log).IsValid() && (*Log)->FullName == FullName)
		{
			ResultIndex = LogIndex;
			break;
		}
	}

	return ResultIndex;
}

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "EventLogger.h"

static FString PrettyPrint(EEventLog::Type Event, const FString& AdditionalContent = FString(), TSharedPtr<SWidget> Widget = TSharedPtr<SWidget>())
{
	FString WidgetInfo;
	if (Widget.IsValid())
	{
		WidgetInfo = FString::Printf(TEXT(", %s %s"), *Widget->GetTypeAsString(), *Widget->GetReadableLocation());
	}

	FString Content;
	if (AdditionalContent.Len())
	{
		Content = FString::Printf(TEXT(": %s"), *AdditionalContent);
	}

	return FString::Printf(
		TEXT("[%07.2f] %s%s%s"),
		FPlatformTime::Seconds() - GStartTime, *EEventLog::ToString(Event), *Content, *WidgetInfo
	);
}

void FFileEventLogger::Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget)
{
	if (Widget.IsValid())
	{
		LoggedEvents.Add( PrettyPrint(Event, AdditionalContent, Widget) );
	}
}

void FFileEventLogger::SaveToFile()
{
	FString LogFilePath;
	{
		const TCHAR LogFileName[] = TEXT("EventLog");
		LogFilePath = FPaths::CreateTempFilename( *FPaths::GameLogDir(), LogFileName, TEXT( ".log" ) );
	}
	FOutputDeviceFile EventLogFile(*LogFilePath);

	for (int32 i = 0; i < LoggedEvents.Num(); ++i)
	{
		EventLogFile.Serialize(*LoggedEvents[i], ELogVerbosity::Log, NAME_None);
	}

	EventLogFile.Flush();
	EventLogFile.TearDown();
}

FString FFileEventLogger::GetLog() const
{
	FString OutputLog = LINE_TERMINATOR LINE_TERMINATOR;
	for (int32 i = 0; i < LoggedEvents.Num(); ++i)
	{
		OutputLog += LoggedEvents[i] + LINE_TERMINATOR;
	}
	return OutputLog;
}



void FConsoleEventLogger::Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget)
{
	UE_LOG(LogSlate, Log, TEXT("%s"), *PrettyPrint(Event, AdditionalContent, Widget) );
}

void FConsoleEventLogger::SaveToFile()
{
}

FString FConsoleEventLogger::GetLog() const
{
	return FString();
}



/** Limit of how many items we should have in our stability log */
static const int32 STABILITY_LOG_MAX_SIZE = 100;

void FStabilityEventLogger::Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget)
{
	// filter out events that happen a lot
	if (Event == EEventLog::MouseMove ||
		Event == EEventLog::MouseEnter ||
		Event == EEventLog::MouseLeave ||
		Event == EEventLog::DragEnter ||
		Event == EEventLog::DragLeave ||
		Event == EEventLog::DragOver)
	{
		return;
	}

	if (Widget.IsValid())
	{
		LoggedEvents.Add( PrettyPrint(Event, AdditionalContent, Widget) );
	}
	else
	{
		LoggedEvents.Add( PrettyPrint(Event, AdditionalContent) );
	}
	if (LoggedEvents.Num() > STABILITY_LOG_MAX_SIZE)
	{
		LoggedEvents.RemoveAt(0, LoggedEvents.Num() - STABILITY_LOG_MAX_SIZE);
	}
}

void FStabilityEventLogger::SaveToFile()
{
}

FString FStabilityEventLogger::GetLog() const
{
	FString OutputLog = LINE_TERMINATOR LINE_TERMINATOR;
	for (int32 i = 0; i < LoggedEvents.Num(); ++i)
	{
		OutputLog += LoggedEvents[i] + LINE_TERMINATOR;
	}
	return OutputLog;
}

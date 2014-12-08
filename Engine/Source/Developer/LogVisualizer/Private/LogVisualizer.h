// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VisualLogger/VisualLogger.h"

#if ENABLE_VISUAL_LOG

namespace VisualLogJson
{
	static const FString TAG_NAME = TEXT("Name");
	static const FString TAG_FULLNAME = TEXT("FullName");
	static const FString TAG_ENTRIES = TEXT("Entries");
	static const FString TAG_TIMESTAMP = TEXT("TimeStamp");
	static const FString TAG_LOCATION = TEXT("Location");
	static const FString TAG_STATUS = TEXT("Status");
	static const FString TAG_STATUSLINES = TEXT("StatusLines");
	static const FString TAG_CATEGORY = TEXT("Category");
	static const FString TAG_LINE = TEXT("Line");
	static const FString TAG_VERBOSITY = TEXT("Verb");
	static const FString TAG_LOGLINES = TEXT("LogLines");
	static const FString TAG_DESCRIPTION = TEXT("Description");
	static const FString TAG_TYPECOLORSIZE = TEXT("TypeColorSize");
	static const FString TAG_POINTS = TEXT("Points");
	static const FString TAG_ELEMENTSTODRAW = TEXT("ElementsToDraw");
	static const FString TAG_TAGNAME = TEXT("DataBlockTagName");
	static const FString TAG_USERDATA = TEXT("UserData");
	static const FString TAG_CAPSULESTODRAW = TEXT("CapsulesToDraw");
	static const FString TAG_COUNTER = TEXT("Counter");
	static const FString TAG_EVENTNAME = TEXT("EventName");
	static const FString TAG_EVENTAGS = TEXT("EventTags");
	static const FString TAG_EVENTSAMPLES = TEXT("EventSamples");

	static const FString TAG_HISTOGRAMSAMPLES = TEXT("HistogramSamples");
	static const FString TAG_HISTOGRAMSAMPLE = TEXT("Sample");
	static const FString TAG_HISTOGRAMGRAPHNAME = TEXT("GraphName");
	static const FString TAG_HISTOGRAMDATANAME = TEXT("DataName");

	static const FString TAG_DATABLOCK = TEXT("DataBlock");
	static const FString TAG_DATABLOCK_DATA = TEXT("DataBlockData");

	static const FString TAG_LOGS = TEXT("Logs");
}

struct FActorsVisLog
{
	static const int VisLogInitialSize = 8;

	FName Name;
	FString FullName;
	TArray<TSharedPtr<FVisualLogEntry> > Entries;

	FActorsVisLog(FName Name, TArray<TWeakObjectPtr<UObject> >* Children);
	FActorsVisLog(const class UObject* Object, TArray<TWeakObjectPtr<UObject> >* Children);
	FActorsVisLog(TSharedPtr<FJsonValue> FromJson);

	TSharedPtr<FJsonValue> ToJson() const;
};


/** Actual implementation of LogVisualizer, private inside module */
class FLogVisualizer : public ILogVisualizer, public FVisualLogDevice
{
public:
	virtual void SummonUI(UWorld* InWorld) override;
	virtual void CloseUI(UWorld* InWorld) override;
	virtual bool IsOpenUI(UWorld* InWorld) override;

	virtual void Serialize(const class UObject* LogOwner, FName OwnerName, const FVisualLogEntry& LogEntry);
	virtual bool HasFlags(int32 InFlags) const { return !!(InFlags & EVisualLoggerDeviceFlags::NoFlags); }

	virtual bool IsRecording();
	// End ILogVisualizer interface


	/** Change the current recording state */
	void SetIsRecording(bool bNewRecording);

	FLogVisualizer()
	{
	}

	virtual ~FLogVisualizer() 
	{
		CleanUp();
	}
	
	DECLARE_EVENT( FLogVisualizer, FVisLogsChangedEvent );
	FVisLogsChangedEvent& OnLogAdded() { return LogAddedEvent; }

	void CleanUp();

	void PullDataFromVisualLog(const TArray<FVisualLogDevice::FVisualLogEntryItem>& RecordedLogs);

	int32 GetLogIndexForActor(const class AActor* Actor);

	void AddLoadedLog(TSharedPtr<FActorsVisLog> Log);

	UWorld* GetWorld() { return World.Get(); }

	virtual class AActor* GetHelperActor(class UWorld* InWorld);

protected:
	void OnNewLog(const class UObject* Object, TSharedPtr<FActorsVisLog> Log);
	
public:
	/** All hosted logs */
	TArray<TSharedPtr<FActorsVisLog> > Logs;

private:
	/** Event called when a new log is created */
	FVisLogsChangedEvent	LogAddedEvent;
	TWeakObjectPtr<UWorld>	World;
	TWeakPtr<SWindow>		LogWindow;
	TWeakObjectPtr<class ALogVisualizerDebugActor> DebugActor;
	TMap<FName, TSharedPtr<FActorsVisLog>> HelperMap;

};

#endif //ENABLE_VISUAL_LOG
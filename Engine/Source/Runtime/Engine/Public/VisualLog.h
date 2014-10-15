// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "VisualLogger/VisualLogger.h"

#define LOG_TO_UELOG_AS_WELL 0

// helpful text macros
#define TEXT_NULL TEXT("NULL")
#define TEXT_TRUE TEXT("TRUE")
#define TEXT_FALSE TEXT("FALSE")
#define TEXT_EMPTY TEXT("")
#define TEXT_CONDITION(Condition) ((Condition) ? TEXT_TRUE : TEXT_FALSE)
#define TEXT_SELF_IF_TRUE(Self) (Self ? TEXT(#Self) : TEXT_EMPTY)
#define TEXT_NAME(Object) ((Object != NULL) ? *(Object->GetName()) : TEXT_NULL)

class FJsonValue;

struct FVisLogSelfDrawingElement
{
	virtual void Draw(class UCanvas* Canvas) const = 0;
};

#if ENABLE_VISUAL_LOG

#if LOG_TO_UELOG_AS_WELL
#define TO_TEXT_LOG(CategoryName, Verbosity, Format, ...) FMsg::Logf(__FILE__, __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, Format, ##__VA_ARGS__);
#else
#define TO_TEXT_LOG(CategoryName, Verbosity, Format, ...) 
#endif // LOG_TO_UELOG_AS_WELL

namespace VisualLogger
{
	struct FDataBlock;
	struct FVisualLogEntry;
}

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

struct ENGINE_API FActorsVisLog
{
	static const int VisLogInitialSize = 8;

	FName Name;
	FString FullName;
	TArray<TSharedPtr<FVisualLogEntry> > Entries;

	FActorsVisLog(const class UObject* Object, TArray<TWeakObjectPtr<UObject> >* Children);
	FActorsVisLog(TSharedPtr<FJsonValue> FromJson);

	TSharedPtr<FJsonValue> ToJson() const;
};

DECLARE_DELEGATE_RetVal(FString, FVisualLogFilenameGetterDelegate);

/** This class is to capture all log output even if the Visual Logger is closed */
class ENGINE_API FVisualLog : public FVisualLogDevice
{
public:
	typedef TMap<const class UObject*, TSharedPtr<FActorsVisLog> > FLogsMap;
	typedef TMap<const class AActor*, TArray<TWeakObjectPtr<UObject> > > FLogRedirectsMap;

	DECLARE_DELEGATE_TwoParams(FOnNewLogCreatedDelegate, const AActor*, TSharedPtr<FActorsVisLog>);

	FVisualLog();
	~FVisualLog();
	
	static FVisualLog& GetStatic()
	{
		static FVisualLog VisLog;
		return VisLog;
	}

	virtual void Cleanup(bool bReleaseMemory) override;
	virtual void StartRecordingToFile(float TImeStamp) override;
	virtual void StopRecordingToFile(float TImeStamp) override;
	virtual void Serialize(const class UObject* LogOwner, const FVisualLogEntry& LogEntry) override;
	virtual void SetFileName(const FString& InFileName) override;

	void Redirect(class UObject* Source, const class AActor* NewRedirection);
	void RedirectToVisualLog(const class UObject* Src, const class AActor* Dest);
	const class AActor* GetVisualLogRedirection(const class UObject* Source);

	void LogLine(const class UObject* Object, const FName& CategoryName, ELogVerbosity::Type Verbosity, const FString& Line, int64 UserData = 0, FName TagName = NAME_Name);

	const FLogsMap* GetLogs() const { return &LogsMap; }

	void RegisterNewLogsObserver(const FOnNewLogCreatedDelegate& NewObserver) { OnNewLogCreated = NewObserver; }
	void ClearNewLogsObserver() { OnNewLogCreated = NULL; }

	void SetIsRecording(bool NewRecording, bool bRecordToFile = false);
	FORCEINLINE bool IsRecording() const { return true; }
	void SetIsRecordingOnServer(bool NewRecording) { bIsRecordingOnServer = NewRecording; }
	FORCEINLINE bool IsRecordingOnServer() const { return !!bIsRecordingOnServer; }
	void DumpRecordedLogs();

	FORCEINLINE bool InWhitelist(FName InName) const { return Whitelist.Find(InName) != INDEX_NONE; }
	FORCEINLINE bool IsAllBlocked() const { return !!bIsAllBlocked; }
	FORCEINLINE void BlockAllLogs(bool bBlock = true) { bIsAllBlocked = bBlock; }
	FORCEINLINE void AddCategortyToWhiteList(FName InCategory) { Whitelist.AddUnique(InCategory); }
	FORCEINLINE void ClearWhiteList() { Whitelist.Reset(); }

	FArchive* FileAr;

	FVisualLogEntry* GetEntryToWrite(const class UObject* Object, float Timestamp = -1, const FVector& Location = FVector::ZeroVector);

	/** highly encouradged to use FVisualLogFilenameGetterDelegate::CreateUObject with this */
	void SetLogFileNameGetter(const FVisualLogFilenameGetterDelegate& InLogFileNameGetter) { LogFileNameGetter = InLogFileNameGetter; }

protected:

	FString GetLogFileFullName() const;

	void LogElementImpl(FVisLogSelfDrawingElement* Element);

	FORCEINLINE TSharedPtr<FActorsVisLog> GetLog(const class UObject* Object)
	{
		TSharedPtr<FActorsVisLog>* Log = LogsMap.Find(Object);
		if (Log == NULL)
		{
			const class AActor* Actor = GetVisualLogRedirection(Object);
			Log = &(LogsMap.Add(Object, MakeShareable(new FActorsVisLog(Object, RedirectsMap.Find(Actor)))));
			OnNewLogCreated.ExecuteIfBound(Actor, *Log);
		}
		return *Log;
	}
			
private:
	/** All log entries since this module has been started (via Start() call)
	 *	or since the last extraction-with-removal (@todo name function here) */
	FLogsMap LogsMap;

	FLogRedirectsMap RedirectsMap;

	TArray<FName>	Whitelist;

	FOnNewLogCreatedDelegate OnNewLogCreated;

	float StartRecordingTime;

	FString BaseFileName;

	int32 bIsRecordingOnServer : 1;
	int32 bIsRecordingToFile : 1;
	int32 bIsAllBlocked : 1;

	FVisualLogFilenameGetterDelegate LogFileNameGetter;
};
#endif //ENABLE_VISUAL_LOG

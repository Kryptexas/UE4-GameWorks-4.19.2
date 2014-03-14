// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#define LOG_TO_UELOG_AS_WELL 1

// helpful text macros
#define TEXT_NULL TEXT("NULL")
#define TEXT_TRUE TEXT("TRUE")
#define TEXT_FALSE TEXT("FALSE")
#define TEXT_EMPTY TEXT("")
#define TEXT_CONDITION(Condition) ((Condition) ? TEXT_TRUE : TEXT_FALSE)
#define TEXT_SELF_IF_TRUE(Self) (Self ? TEXT(#Self) : TEXT_EMPTY)
#define TEXT_NAME(Object) ((Object != NULL) ? *(Object->GetName()) : TEXT_NULL)

DECLARE_LOG_CATEGORY_EXTERN(LogVisual, Warning, All);

class FJsonValue;

struct FVisLogSelfDrawingElement
{
	virtual void Draw(class UCanvas* Canvas) const = 0;
};

struct ENGINE_API FVisLogEntry
{
	float TimeStamp;
#if ENABLE_VISUAL_LOG
	FVector Location;
	struct FLogLine
	{
		FName Category;
		FString Line;
		ELogVerbosity::Type Verbosity;

		FLogLine(const FName& InCategory, const FString& InLine, ELogVerbosity::Type InVerbosity)
			: Category(InCategory), Line(InLine), Verbosity(InVerbosity)
		{}
	};
	TArray<FLogLine> LogLines;
	FString StatusString;

	struct FElementToDraw
	{
		enum EElementType {
			Invalid = 0,
			SinglePoint, // individual points. 
			Segment, // pairs of points 
			Path,	// sequence of point
			Box,
			// note that in order to remain backward compatibility in terms of log
			// serialization new enum values need to be added at the end
		};

		FString Description;
		TArray<FVector> Points;
	protected:
		friend struct FVisLogEntry;
		uint8 Type;
	public:
		uint8 Color;
		union
		{
			uint16 Thicknes;
			uint16 Radius;
		};

		FElementToDraw(EElementType InType = Invalid) : Type(InType), Color(0xff), Thicknes(0)
		{}

		FElementToDraw(const FString& InDescription, const FColor& InColor, uint16 InThickness) : Type(uint16(Invalid)), Thicknes(InThickness)
		{
			if (InDescription.IsEmpty() == false)
			{
				Description = InDescription;
			}
			SetColor(InColor);
		}

		FORCEINLINE void SetColor(const FColor& InColor) 
		{ 
			Color = ((InColor.DWColor() >> 30) << 6)
				| (((InColor.DWColor() & 0x00ff0000) >> 22) << 4)
				| (((InColor.DWColor() & 0x0000ff00) >> 14) << 2)
				|  ((InColor.DWColor() & 0x000000ff) >> 6);
		}

		FORCEINLINE EElementType GetType() const
		{
			return EElementType(Type);
		}

		FORCEINLINE void SetType(EElementType InType)
		{
			Type = InType;
		}

		FORCEINLINE FColor GetFColor() const
		{ 
			return FColor(
				((Color & 0xc0) << 24)
				| ((Color & 0x30) << 18)
				| ((Color & 0x0c) << 12)
				| ((Color & 0x03) << 6)
				);
		}
	};

	TArray<FElementToDraw> ElementsToDraw;

	FVisLogEntry(const class AActor* Actor, TArray<TWeakObjectPtr<AActor> >* Children);
	FVisLogEntry(TSharedPtr<FJsonValue> FromJson);
	
	TSharedPtr<FJsonValue> ToJson() const;

	// path
	void AddElement(const TArray<FVector>& Points, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
	// location
	void AddElement(const FVector& Point, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
	// segment
	void AddElement(const FVector& Start, const FVector& End, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
	// box
	void AddElement(const FBox& Box, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
#endif // ENABLE_VISUAL_LOG
};

#if ENABLE_VISUAL_LOG

#if LOG_TO_UELOG_AS_WELL
#define TO_TEXT_LOG(CategoryName, Verbosity, Format, ...) FMsg::Logf(__FILE__, __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, Format, ##__VA_ARGS__);
#else
#define TO_TEXT_LOG(CategoryName, Verbosity, Format, ...) 
#endif // LOG_TO_UELOG_AS_WELL

#define REDIRECT_TO_VLOG(Dest) RedirectToVisualLog(Dest)
#define REDIRECT_ACTOR_TO_VLOG(Src, Dest) Src->RedirectToVisualLog(Dest)

#define UE_VLOG(Actor, CategoryName, Verbosity, Format, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	checkAtCompileTime((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, Verbosity_must_be_constant_and_in_range); \
	if (UE_LOG_CHECK_COMPILEDIN_VERBOSITY(CategoryName, Verbosity)) \
	{ \
		if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity) && (Actor) != NULL) \
		{ \
			FVisualLog::Get()->LogLine(Actor, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
			TO_TEXT_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
		} \
	} \
}

#define UE_CVLOG(Condition, Actor, CategoryName, Verbosity, Format, ...) UE_CLOG(Condition, CategoryName, Verbosity, Format, ##__VA_ARGS__) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	checkAtCompileTime((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, Verbosity_must_be_constant_and_in_range); \
	if (UE_LOG_CHECK_COMPILEDIN_VERBOSITY(CategoryName, Verbosity)) \
	{ \
		if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity) && (Actor) != NULL && (Condition)) \
		{ \
			FVisualLog::Get()->LogLine(Actor, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
			TO_TEXT_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
		} \
	} \
}

#define UE_VLOG_SEGMENT(Actor, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	if (FVisualLog::Get()->IsRecording()) \
	{ \
		FVisualLog::Get()->GetEntryToWrite(Actor)->AddElement(SegmentStart, SegmentEnd, Color, FString::Printf(DescriptionFormat, ##__VA_ARGS__)); \
	} \
}

#define UE_VLOG_LOCATION(Actor, Location, Radius, Color, DescriptionFormat, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	if (FVisualLog::Get()->IsRecording()) \
	{ \
		FVisualLog::Get()->GetEntryToWrite(Actor)->AddElement(Location, Color, FString::Printf(DescriptionFormat, ##__VA_ARGS__), Radius); \
	} \
}

#define UE_VLOG_BOX(Actor, Box, Color, DescriptionFormat, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	if (FVisualLog::Get()->IsRecording()) \
	{ \
		FVisualLog::Get()->GetEntryToWrite(Actor)->AddElement(Box, Color, FString::Printf(DescriptionFormat, ##__VA_ARGS__)); \
	} \
}

struct ENGINE_API FActorsVisLog
{
	static const int VisLogInitialSize = 8;

	FName Name;
	FString FullName;
	TArray<TSharedPtr<FVisLogEntry> > Entries;

	FActorsVisLog(const class AActor* Actor, TArray<TWeakObjectPtr<AActor> >* Children);
	FActorsVisLog(TSharedPtr<FJsonValue> FromJson);

	TSharedPtr<FJsonValue> ToJson() const;
};

/** This class is to capture all log output even if the Visual Logger is closed */
class ENGINE_API FVisualLog : public FOutputDevice
{
public:
	typedef TMap<const class AActor*, TSharedPtr<FActorsVisLog> > FLogsMap;
	typedef TMap<const class AActor*, TArray<TWeakObjectPtr<AActor> > > FLogRedirectsMap;

	DECLARE_DELEGATE_TwoParams(FOnNewLogCreatedDelegate, const AActor*, TSharedPtr<FActorsVisLog>);

	FVisualLog();
	~FVisualLog();
	
	static FVisualLog* Get()
	{
		static FVisualLog StaticLog;
		return &StaticLog;
	}

	void Cleanup();

	void Redirect(class AActor* Actor, const class AActor* NewRedirection);
	
	void LogLine(const class AActor* Actor, const FName& CategoryName, ELogVerbosity::Type Verbosity, const FString& Line);

	const FLogsMap* GetLogs() const { return &LogsMap; }

	void RegisterNewLogsObserver(const FOnNewLogCreatedDelegate& NewObserver) { OnNewLogCreated = NewObserver; }
	void ClearNewLogsObserver() { OnNewLogCreated = NULL; }

	void SetIsRecording(bool NewRecording) { bIsRecording = NewRecording; }
	FORCEINLINE bool IsRecording() const { return !!bIsRecording; }

	FORCEINLINE_DEBUGGABLE FVisLogEntry* GetEntryToWrite(const class AActor* Actor)
	{
		check(Actor && Actor->GetWorld() && Actor->GetVisualLogRedirection());
		const class AActor* LogOwner = Actor->GetVisualLogRedirection();
		const float TimeStamp = Actor->GetWorld()->TimeSeconds;
		TSharedPtr<FActorsVisLog> Log = GetLog(LogOwner);
		FVisLogEntry* Entry = Log->Entries.Num() > 0 ? Log->Entries[Log->Entries.Num()-1].Get() : NULL;

		if (Entry == NULL || Entry->TimeStamp < TimeStamp)
		{
			// create new entry
			Entry = Log->Entries[Log->Entries.Add(MakeShareable(new FVisLogEntry(LogOwner, RedirectsMap.Find(LogOwner))))].Get();
		}

		check(Entry);
		return Entry;
	}

protected:

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE;

	void LogElementImpl(FVisLogSelfDrawingElement* Element);

	FORCEINLINE_DEBUGGABLE TSharedPtr<FActorsVisLog> GetLog(const class AActor* Actor)
	{
		TSharedPtr<FActorsVisLog>* Log = LogsMap.Find(Actor);
		if (Log == NULL)
		{
			Log = &(LogsMap.Add(Actor, MakeShareable(new FActorsVisLog(Actor, RedirectsMap.Find(Actor)))));
			OnNewLogCreated.ExecuteIfBound(Actor, *Log);
		}
		return *Log;
	}
			
private:
	/** All log entries since this module has been started (via Start() call)
	 *	or since the last extraction-with-removal (@todo name function here) */
	FLogsMap LogsMap;

	FLogRedirectsMap RedirectsMap;

	FOnNewLogCreatedDelegate OnNewLogCreated;

	int32 bIsRecording : 1;
};

#else
	#define UE_VLOG(Actor, CategoryName, Verbosity, Format, ...) UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__)
	#define UE_CVLOG(Condition, Actor, CategoryName, Verbosity, Format, ...) UE_CLOG(Condition, CategoryName, Verbosity, Format, ##__VA_ARGS__)
	#define UE_VLOG_SEGMENT(Actor, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...)
	#define UE_VLOG_LOCATION(Actor, Location, Radius, Color, DescriptionFormat, ...)
	#define UE_VLOG_BOX(Actor, Box, Color, DescriptionFormat, ...) 
	#define REDIRECT_TO_VLOG(Dest)
	#define REDIRECT_ACTOR_TO_VLOG(Src, Destination) 
#endif
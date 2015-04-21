// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLogger/VisualLogger.h"
#include "VisualLogger/VisualLogger.h"
#include "VisualLogger/VisualLoggerBinaryFileDevice.h"
#if WITH_EDITOR
#	include "Editor/UnrealEd/Public/EditorComponents.h"
#	include "Editor/UnrealEd/Public/EditorReimportHandler.h"
#	include "Editor/UnrealEd/Public/TexAlignTools.h"
#	include "Editor/UnrealEd/Public/TickableEditorObject.h"
#	include "UnrealEdClasses.h"
#	include "Editor/UnrealEd/Public/Editor.h"
#	include "Editor/UnrealEd/Public/EditorViewportClient.h"
#endif


#if ENABLE_VISUAL_LOG 

DEFINE_STAT(STAT_VisualLog);
DEFINE_LOG_CATEGORY(LogVisual);

TMap<UObject*, TArray<TWeakObjectPtr<const UObject> > > FVisualLogger::RedirectionMap;

// Unfortunately needs to be a #define since it uses GET_VARARGS_RESULT which uses the va_list stuff which operates on the
// current function, so we can't easily call a function
#define COLLAPSED_LOGF(SerializeFunc) \
	int32	BufferSize	= 1024; \
	TCHAR*	Buffer		= nullptr; \
	int32	Result		= -1; \
	/* allocate some stack space to use on the first pass, which matches most strings */ \
	TCHAR	StackBuffer[512]; \
	TCHAR*	AllocatedBuffer = nullptr; \
	\
	/* first, try using the stack buffer */ \
	Buffer = StackBuffer; \
	GET_VARARGS_RESULT( Buffer, ARRAY_COUNT(StackBuffer), ARRAY_COUNT(StackBuffer) - 1, Fmt, Fmt, Result ); \
	\
	/* if that fails, then use heap allocation to make enough space */ \
		while(Result == -1) \
			{ \
		FMemory::SystemFree(AllocatedBuffer); \
		/* We need to use malloc here directly as GMalloc might not be safe. */ \
		Buffer = AllocatedBuffer = (TCHAR*) FMemory::SystemMalloc( BufferSize * sizeof(TCHAR) ); \
		GET_VARARGS_RESULT( Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result ); \
		BufferSize *= 2; \
			}; \
	Buffer[Result] = 0; \
	; \
	\
	SerializeFunc; \
	FMemory::SystemFree(AllocatedBuffer);

inline
bool FVisualLogger::CheckVisualLogInputInternal(const class UObject* Object, const struct FLogCategoryBase& Category, ELogVerbosity::Type Verbosity, UWorld **World, FVisualLogEntry **CurrentEntry)
{
	FVisualLogger& VisualLogger = FVisualLogger::Get();
	if (!Object || (GEngine && GEngine->bDisableAILogging) || VisualLogger.IsRecording() == false || Object->HasAnyFlags(RF_ClassDefaultObject))
	{
		return false;
	}
	const FName CategoryName = Category.GetCategoryName();
	if (VisualLogger.IsBlockedForAllCategories() && VisualLogger.IsWhiteListed(CategoryName) == false)
	{
		return false;
	}

	*World = GEngine->GetWorldFromContextObject(Object, false);
	if (ensure(*World != nullptr) == false)
	{
		return false;
	}

	*CurrentEntry = VisualLogger.GetEntryToWrite(Object, (*World)->TimeSeconds);
	if (ensure(CurrentEntry != nullptr) == false)
	{
		return false;
	}

	return true;
}

inline
FVisualLogEntry* FVisualLogger::GetEntryToWrite(const class UObject* Object, float TimeStamp, ECreateIfNeeded ShouldCreate)
{
	FVisualLogEntry* CurrentEntry = nullptr;
	UObject * LogOwner = FVisualLogger::FindRedirection(Object);

	bool InitializeNewEntry = false;

	TWeakObjectPtr<UWorld> World = GetWorld(Object);

	if (CurrentEntryPerObject.Contains(LogOwner))
	{
		CurrentEntry = &CurrentEntryPerObject[LogOwner];
		InitializeNewEntry = TimeStamp > CurrentEntry->TimeStamp && ShouldCreate == ECreateIfNeeded::Create;
		if (World.IsValid())
		{
			World->GetTimerManager().ClearTimer(VisualLoggerCleanupTimerHandle);
			for (auto& CurrentPair : CurrentEntryPerObject)
			{
				FVisualLogEntry* Entry = &CurrentPair.Value;
				if (Entry->TimeStamp >= 0 && Entry->TimeStamp < TimeStamp)
				{
					for (auto* Device : OutputDevices)
					{
						Device->Serialize(CurrentPair.Key, ObjectToNameMap[CurrentPair.Key], ObjectToClassNameMap[CurrentPair.Key], *Entry);
					}
					Entry->Reset();
				}
			}
		}
	}

	if (!CurrentEntry)
	{
		CurrentEntry = &CurrentEntryPerObject.Add(LogOwner);
		ObjectToNameMap.Add(LogOwner, LogOwner->GetFName());
		ObjectToClassNameMap.Add(LogOwner, *(LogOwner->GetClass()->GetName()));
		ObjectToPointerMap.Add(LogOwner, LogOwner);
		InitializeNewEntry = true;
	}

	if (InitializeNewEntry)
	{
		CurrentEntry->Reset();
		CurrentEntry->TimeStamp = TimeStamp;

		if (RedirectionMap.Contains(LogOwner))
		{
			if (ObjectToPointerMap.Contains(LogOwner) && ObjectToPointerMap[LogOwner].IsValid())
			{
				const class AActor* LogOwnerAsActor = Cast<class AActor>(LogOwner);
				if (LogOwnerAsActor)
				{
					LogOwnerAsActor->GrabDebugSnapshot(CurrentEntry);
				}
			}
			for (auto Child : RedirectionMap[LogOwner])
			{
				if (Child.IsValid())
				{
					const class AActor* ChildAsActor = Cast<class AActor>(Child.Get());
					if (ChildAsActor)
					{
						ChildAsActor->GrabDebugSnapshot(CurrentEntry);
					}
				}
			}
		}
		else
		{
			const class AActor* ObjectAsActor = Cast<class AActor>(Object);
			if (ObjectAsActor)
			{
				CurrentEntry->Location = ObjectAsActor->GetActorLocation();
				ObjectAsActor->GrabDebugSnapshot(CurrentEntry);
			}
		}
	}

	if (World.IsValid())
	{
		//set next tick timer to flush obsolete/old entries
		World->GetTimerManager().SetTimer(VisualLoggerCleanupTimerHandle, FTimerDelegate::CreateLambda(
			[this, World](){
			for (auto& CurrentPair : CurrentEntryPerObject)
			{
				FVisualLogEntry* Entry = &CurrentPair.Value;
				if (Entry->TimeStamp >= 0 && (!World.IsValid() || Entry->TimeStamp < World->GetTimeSeconds())) // CurrentEntry->TimeStamp == -1 means it's not initialized entry information
				{
					for (auto* Device : OutputDevices)
					{
						Device->Serialize(CurrentPair.Key, ObjectToNameMap[CurrentPair.Key], ObjectToClassNameMap[CurrentPair.Key], *Entry);
					}
					Entry->Reset();
				}
			}
		}
		), 0.1, false);
	}

	return CurrentEntry;
}

inline
void FVisualLogger::Flush()
{
	for (auto &CurrentEntry : CurrentEntryPerObject)
	{
		if (CurrentEntry.Value.TimeStamp >= 0)
		{
			for (auto* Device : OutputDevices)
			{
				Device->Serialize(CurrentEntry.Key, ObjectToNameMap[CurrentEntry.Key], ObjectToClassNameMap[CurrentEntry.Key], CurrentEntry.Value);
			}
			CurrentEntry.Value.Reset();
		}
	}
}


inline
VARARG_BODY(void, FVisualLogger::CategorizedLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddText(Buffer, Category.GetCategoryName(), Verbosity);
	);
}

inline
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Start) VARARG_EXTRA(const FVector& End) VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddElement(Start, End, Category.GetCategoryName(), Verbosity, Color, Buffer);
	);
}

inline
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Location) VARARG_EXTRA(float Radius) VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddElement(Location, Category.GetCategoryName(), Verbosity, Color, Buffer, Radius);
	);
}

inline
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FBox& Box) VARARG_EXTRA(const FMatrix& Matrix) VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddElement(Box, Matrix, Category.GetCategoryName(), Verbosity, Color, Buffer);
	);
}

inline
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Orgin) VARARG_EXTRA(const FVector& Direction) VARARG_EXTRA(const float Length) VARARG_EXTRA(const float Angle)  VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddElement(Orgin, Direction, Length, Angle, Angle, Category.GetCategoryName(), Verbosity, Color, Buffer);
	);
}

inline
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Start) VARARG_EXTRA(const FVector& End) VARARG_EXTRA(const float Radius) VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddElement(Start, End, Radius, Category.GetCategoryName(), Verbosity, Color, Buffer);
	);
}

inline
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Center) VARARG_EXTRA(float HalfHeight) VARARG_EXTRA(float Radius) VARARG_EXTRA(const FQuat& Rotation) VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddElement(Center, HalfHeight, Radius, Rotation, Category.GetCategoryName(), Verbosity, Color, Buffer);
	);
}

inline
VARARG_BODY(void, FVisualLogger::HistogramDataLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(FName GraphName) VARARG_EXTRA(FName DataName) VARARG_EXTRA(const FVector2D& Data) VARARG_EXTRA(const FColor& Color))
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	if (CheckVisualLogInputInternal(Object, Category, Verbosity, &World, &CurrentEntry) == false)
	{
		return;
	}

	COLLAPSED_LOGF(
		CurrentEntry->AddHistogramData(Data, Category.GetCategoryName(), Verbosity, GraphName, DataName);
	);
}

#undef COLLAPSED_LOGF

inline
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3, const FVisualLogEventBase& Event4, const FVisualLogEventBase& Event5, const FVisualLogEventBase& Event6)
{
	EventLog(Object, EventTag1, Event1, Event2, Event3, Event4, Event5);
	EventLog(Object, EventTag1, Event6);
}

inline
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3, const FVisualLogEventBase& Event4, const FVisualLogEventBase& Event5)
{
	EventLog(Object, EventTag1, Event1, Event2, Event3, Event4);
	EventLog(Object, EventTag1, Event5);
}

inline
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3, const FVisualLogEventBase& Event4)
{
	EventLog(Object, EventTag1, Event1, Event2, Event3);
	EventLog(Object, EventTag1, Event4);
}

inline
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3)
{
	EventLog(Object, EventTag1, Event1, Event2);
	EventLog(Object, EventTag1, Event3);
}

inline
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2)
{
	EventLog(Object, EventTag1, Event1);
	EventLog(Object, EventTag1, Event2);
}

inline
void FVisualLogger::EventLog(const class UObject* LogOwner, const FVisualLogEventBase& Event1, const FName EventTag1, const FName EventTag2, const FName EventTag3, const FName EventTag4, const FName EventTag5, const FName EventTag6)
{
	EventLog(LogOwner, EventTag1, Event1, EventTag2, EventTag3, EventTag4, EventTag5, EventTag6);
}

inline
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event, const FName EventTag2, const FName EventTag3, const FName EventTag4, const FName EventTag5, const FName EventTag6)
{
	SCOPE_CYCLE_COUNTER(STAT_VisualLog);
	UWorld *World = nullptr;
	FVisualLogEntry *CurrentEntry = nullptr;
	const FLogCategory<ELogVerbosity::Log, ELogVerbosity::Log> Category(*Event.Name);
	if (CheckVisualLogInputInternal(Object, Category, ELogVerbosity::Log, &World, &CurrentEntry) == false)
	{
		return;
	}

	int32 Index = CurrentEntry->Events.Find(FVisualLogEvent(Event));
	if (Index != INDEX_NONE)
	{
		CurrentEntry->Events[Index].Counter++;
	}
	else
	{
		Index = CurrentEntry->AddEvent(Event);
	}

	CurrentEntry->Events[Index].EventTags.FindOrAdd(EventTag1)++;
	CurrentEntry->Events[Index].EventTags.FindOrAdd(EventTag2)++;
	CurrentEntry->Events[Index].EventTags.FindOrAdd(EventTag3)++;
	CurrentEntry->Events[Index].EventTags.FindOrAdd(EventTag4)++;
	CurrentEntry->Events[Index].EventTags.FindOrAdd(EventTag5)++;
	CurrentEntry->Events[Index].EventTags.FindOrAdd(EventTag6)++;
	CurrentEntry->Events[Index].EventTags.Remove(NAME_None);
}


FVisualLogger::FVisualLogger()
{
	BlockAllCategories(false);
	AddDevice(&FVisualLoggerBinaryFileDevice::Get());
	SetIsRecording(GEngine ? !!GEngine->bEnableVisualLogRecordingOnStart : false);
	SetIsRecordingOnServer(false);

	if (FParse::Param(FCommandLine::Get(), TEXT("EnableAILogging")))
	{
		SetIsRecording(true);
		SetIsRecordingToFile(true);
	}
}

UWorld* FVisualLogger::GetWorld(const class UObject* Object)
{
	UWorld* World = Object ? GEngine->GetWorldFromContextObject(Object, false) : nullptr;
#if WITH_EDITOR
	UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EEngine != nullptr && World == nullptr)
	{
		// lets use PlayWorld during PIE/Simulate and regular world from editor otherwise, to draw debug information
		World = EEngine->PlayWorld != nullptr ? EEngine->PlayWorld : EEngine->GetEditorWorldContext().World();
	}

#endif
	if (!GIsEditor && World == nullptr)
	{
		World = GEngine->GetWorld();
	}

	return World;
}

void FVisualLogger::Shutdown()
{
	SetIsRecording(false);
	SetIsRecordingToFile(false);

	if (UseBinaryFileDevice)
	{
		RemoveDevice(&FVisualLoggerBinaryFileDevice::Get());
	}
}

void FVisualLogger::Cleanup(bool bReleaseMemory)
{
	const bool WasRecordingToFile = IsRecordingToFile();
	if (WasRecordingToFile)
	{
		SetIsRecordingToFile(false);
	}

	for (FVisualLogDevice* Device : FVisualLogger::Get().OutputDevices)
	{
		Device->Cleanup(bReleaseMemory);
	}

	RedirectionMap.Reset();
	LastUniqueIds.Reset();
	CurrentEntryPerObject.Reset();

	if (WasRecordingToFile)
	{
		SetIsRecordingToFile(true);
	}
}

int32 FVisualLogger::GetUniqueId(float Timestamp)
{
	return LastUniqueIds.FindOrAdd(Timestamp)++;
}

void FVisualLogger::Redirect(UObject* FromObject, UObject* ToObject)
{
	if (FromObject == ToObject || FromObject == nullptr || ToObject == nullptr)
	{
		return;
	}

	UObject* OldRedirection = FindRedirection(FromObject);
	UObject* NewRedirection = FindRedirection(ToObject);

	if (OldRedirection != NewRedirection)
	{
		auto OldArray = RedirectionMap.Find(OldRedirection);
		if (OldArray)
		{
			OldArray->RemoveSingleSwap(FromObject);
		}

		RedirectionMap.FindOrAdd(NewRedirection).AddUnique(FromObject);

		UE_CVLOG(FromObject != nullptr, FromObject, LogVisual, Log, TEXT("Redirected '%s' to '%s'"), *FromObject->GetName(), *NewRedirection->GetName());
	}
}

class UObject* FVisualLogger::FindRedirection(const UObject* Object)
{
	if (RedirectionMap.Contains(Object) == false)
	{
		for (auto& Redirection : RedirectionMap)
		{
			if (Redirection.Value.Find(Object) != INDEX_NONE)
			{
				return FindRedirection(Redirection.Key);
			}
		}
	}

	return const_cast<class UObject*>(Object);
}

void FVisualLogger::SetIsRecording(bool InIsRecording) 
{ 
	if (IsRecordingToFile())
	{
		SetIsRecordingToFile(false);
	}
	bIsRecording = InIsRecording; 
};

void FVisualLogger::SetIsRecordingToFile(bool InIsRecording)
{
	if (!bIsRecording && InIsRecording)
	{
		SetIsRecording(true);
	}

	UWorld* World = GEngine ? GEngine->GetWorld() : nullptr;

	const FString BaseFileName = LogFileNameGetter.IsBound() ? LogFileNameGetter.Execute() : TEXT("VisualLog");
	const FString MapName = World ? World->GetMapName() : TEXT("");

	FString OutputFileName = FString::Printf(TEXT("%s_%s"), *BaseFileName, *MapName);

	if (bIsRecordingToFile && !InIsRecording)
	{
		for (auto* Device : OutputDevices)
		{
			if (Device->HasFlags(EVisualLoggerDeviceFlags::CanSaveToFile))
			{
				Device->SetFileName(OutputFileName);
				Device->StopRecordingToFile(World ? World->TimeSeconds : StartRecordingToFileTime);
			}
		}
	}
	else if (!bIsRecordingToFile && InIsRecording)
	{
		StartRecordingToFileTime = World ? World->TimeSeconds : 0;
		for (auto* Device : OutputDevices)
		{
			if (Device->HasFlags(EVisualLoggerDeviceFlags::CanSaveToFile))
			{
				Device->StartRecordingToFile(StartRecordingToFileTime);
			}
		}
	}

	bIsRecordingToFile = InIsRecording;
}

#endif //ENABLE_VISUAL_LOG

const FGuid EVisualLoggerVersion::GUID = FGuid(0xA4237A36, 0xCAEA41C9, 0x8FA218F8, 0x58681BF3);
FCustomVersionRegistration GVisualLoggerVersion(EVisualLoggerVersion::GUID, EVisualLoggerVersion::LatestVersion, TEXT("VisualLogger"));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if WITH_EDITOR
#include "SlateBasics.h"
#endif
static class FLogVisualizerExec : private FSelfRegisteringExec
{
public:
	/** Console commands, see embeded usage statement **/
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		if (FParse::Command(&Cmd, TEXT("VISLOG")))
		{
			if (FModuleManager::Get().LoadModulePtr<IModuleInterface>("LogVisualizer") != nullptr)
			{
#if ENABLE_VISUAL_LOG
				FString Command = FParse::Token(Cmd, 0);
				if (Command == TEXT("record"))
				{
					FVisualLogger::Get().SetIsRecording(true);
					return true;
				}
				else if (Command == TEXT("stop"))
				{
					FVisualLogger::Get().SetIsRecording(false);
					return true;
				}
				else if (Command == TEXT("disableallbut"))
				{
					FString Category = FParse::Token(Cmd, 1);
					FVisualLogger::Get().BlockAllCategories(true);
					FVisualLogger::Get().GetWhiteList().AddUnique(*Category);
					return true;
				}
#if WITH_EDITOR
				else
				{
					FGlobalTabmanager::Get()->InvokeTab(FName(TEXT("VisualLogger")));
					return true;
				}
#endif
#else
			UE_LOG(LogVisual, Warning, TEXT("Unable to open LogVisualizer - logs are disabled"));
#endif
			}
		}
		return false;
	}
} LogVisualizerExec;


#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

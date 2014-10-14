// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Runtime/Core/Private/Misc/VarargsHelper.h"

#define CHECK_VLOG_DATA() \
	float CurrentTime = 0; \
	const FName CategoryName = const_cast<struct FLogCategoryBase&>(Category).GetCategoryName(); \
	if (GEngine && GEngine->bDisableAILogging || FVisualLogger::Get().bIsRecording == false || !Object || Object->HasAnyFlags(RF_ClassDefaultObject) || (FVisualLogger::Get().IsBlockedForAllCategories() && FVisualLogger::Get().CategoriesWhiteList.Find(CategoryName) == INDEX_NONE)) \
	{ \
		return; \
	} \
	UObject* LogOwner = FindRedirection(Object); \
	if (LogOwner == NULL) \
	{ \
		return; \
	} \
	UWorld* World = GEngine->GetWorldFromContextObject(Object); \
	if(ensure(World) == false) \
	{ \
		return; \
	}

FORCEINLINE_DEBUGGABLE
FVisualLogEntry* FVisualLogger::GetEntryToWrite(const class UObject* Object, float TimeStamp, VisualLogger::ECreateIfNeeded ShouldCreate)
{
	FVisualLogEntry* CurrentEntry = NULL;

	bool InitializeNewEntry = false;
	if (CurrentEntryPerObject.Contains(Object))
	{
		CurrentEntry = &CurrentEntryPerObject[Object];
		if (TimeStamp > CurrentEntry->TimeStamp && ShouldCreate == VisualLogger::Create)
		{
			if (CurrentEntry->TimeStamp >= 0) //-1 means not initialized entry information
			{
				for (auto* Device : OutputDevices)
				{
					Device->Serialize(Object, *CurrentEntry);
				}
			}
			InitializeNewEntry = true;
		}
	}
	else
	{
		CurrentEntry = &CurrentEntryPerObject.Add(Object);
		InitializeNewEntry = true;
	}

	if (InitializeNewEntry)
	{
		CurrentEntry->Reset();
		CurrentEntry->TimeStamp = TimeStamp;
		const class AActor* AsActor = Cast<class AActor>(Object);
		if (AsActor)
		{
			CurrentEntry->Location = AsActor->GetActorLocation();
			AsActor->GrabDebugSnapshot(CurrentEntry);
		}
		if (RedirectionMap.Contains(Object))
		{
			for (auto Child : RedirectionMap[Object])
			{
				const class AActor* AsActor = Cast<class AActor>(Child);
				if (AsActor)
				{
					AsActor->GrabDebugSnapshot(CurrentEntry);
				}
			}
		}
	}

	return CurrentEntry;
}

FORCEINLINE_DEBUGGABLE
void FVisualLogger::Flush()
{
	for (auto &CurrentEntry : CurrentEntryPerObject)
	{
		if (CurrentEntry.Value.TimeStamp >= 0)
		{
			for (auto* Device : OutputDevices)
			{
				Device->Serialize(CurrentEntry.Key, CurrentEntry.Value);
			}
			CurrentEntry.Value.Reset();
		}
	}
}


FORCEINLINE
VARARG_BODY(void, FVisualLogger::CategorizedLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddText(Buffer, CategoryName);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Start) VARARG_EXTRA(const FVector& End) VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddElement(Start, End, CategoryName, Color, Buffer);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Location) VARARG_EXTRA(float Radius) VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddElement(Location, CategoryName, Color, Buffer, Radius);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FBox& Box) VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddElement(Box, CategoryName, Color, Buffer);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Orgin) VARARG_EXTRA(const FVector& Direction) VARARG_EXTRA(const float Length) VARARG_EXTRA(const float Angle)  VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddElement(Orgin, Direction, Length, Angle, Angle, CategoryName, Color, Buffer);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Start) VARARG_EXTRA(const FVector& End) VARARG_EXTRA(const float Radius) VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddElement(Start, End, Radius, CategoryName, Color, Buffer);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::GeometryShapeLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Center) VARARG_EXTRA(float HalfHeight) VARARG_EXTRA(float Radius) VARARG_EXTRA(const FQuat& Rotation) VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddCapsule(Center, HalfHeight, Radius, Rotation, CategoryName, Color, Buffer);
	);
}

FORCEINLINE
VARARG_BODY(void, FVisualLogger::HistogramDataLogf, const TCHAR*, VARARG_EXTRA(const class UObject* Object) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(FName GraphName) VARARG_EXTRA(FName DataName) VARARG_EXTRA(const FVector2D& Data) VARARG_EXTRA(const FColor& Color))
{
	CHECK_VLOG_DATA();

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	ensure(CurrentEntry);

	GROWABLE_LOGF(
		CurrentEntry->AddHistogramData(Data, CategoryName, GraphName, DataName);
	);
}

FORCEINLINE
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3, const FVisualLogEventBase& Event4, const FVisualLogEventBase& Event5, const FVisualLogEventBase& Event6)
{
	EventLog(Object, EventTag1, Event1, Event2, Event3, Event4, Event5);
	EventLog(Object, EventTag1, Event6);
}

FORCEINLINE
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3, const FVisualLogEventBase& Event4, const FVisualLogEventBase& Event5)
{
	EventLog(Object, EventTag1, Event1, Event2, Event3, Event4);
	EventLog(Object, EventTag1, Event5);
}

FORCEINLINE
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3, const FVisualLogEventBase& Event4)
{
	EventLog(Object, EventTag1, Event1, Event2, Event3);
	EventLog(Object, EventTag1, Event4);
}

FORCEINLINE
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2, const FVisualLogEventBase& Event3)
{
	EventLog(Object, EventTag1, Event1, Event2);
	EventLog(Object, EventTag1, Event3);
}

FORCEINLINE
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event1, const FVisualLogEventBase& Event2)
{
	EventLog(Object, EventTag1, Event1);
	EventLog(Object, EventTag1, Event2);
}

FORCEINLINE_DEBUGGABLE_ACTUAL
void FVisualLogger::EventLog(const class UObject* Object, const FName EventTag1, const FVisualLogEventBase& Event, const FName EventTag2, const FName EventTag3, const FName EventTag4, const FName EventTag5, const FName EventTag6)
{
	const FName CategoryName = *Event.GetName();

	float CurrentTime = 0;
	if ((GEngine && GEngine->bDisableAILogging) || FVisualLogger::Get().bIsRecording == false || !Object || Object->HasAnyFlags(RF_ClassDefaultObject) || (FVisualLogger::Get().IsBlockedForAllCategories() && FVisualLogger::Get().CategoriesWhiteList.Find(CategoryName) == INDEX_NONE))
	{
		return;
	}
	UObject* LogOwner = FindRedirection(Object);
	UWorld* World = GEngine->GetWorldFromContextObject(Object);
	if (World == NULL) 
	{ 
		ensure(World); 
		return; 
	} 

	FVisualLogEntry* CurrentEntry = FVisualLogger::Get().GetEntryToWrite(LogOwner, World->TimeSeconds);
	if (ensure(CurrentEntry))
	{
		int32 Index = CurrentEntry->Events.Find(FVisualLogEntry::FLogEvent(Event));
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
}

#undef  CHECK_VLOG_DATA

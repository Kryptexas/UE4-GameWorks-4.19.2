// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "EngineDefines.h"

namespace VisualLogger
{
	enum ECreateIfNeeded
	{
		Invalid = -1,
		DontCreate = 0,
		Create = 1,
	};

	enum DeviceFlags
	{
		CanSaveToFile = 1,
		StoreLogsLocally = 2,
	};
}

struct ENGINE_API FVisualLogEventBase
{
	const FString Name;
	const FString FriendlyDesc;
	const ELogVerbosity::Type Verbosity;

	FVisualLogEventBase(const FString& InName, const FString& InFriendlyDesc, ELogVerbosity::Type InVerbosity)
		: Name(InName), FriendlyDesc(InFriendlyDesc), Verbosity(InVerbosity)
	{
	}
};

struct ENGINE_API FVisualLogEntry
{
	struct FLogEvent
	{
		FString Name;
		FString UserFriendlyDesc;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		TMap<FName, int32>	 EventTags;
		int32 Counter;
		int64 UserData;
		FName TagName;

		FLogEvent() : Counter(1) {}
		FLogEvent(const FVisualLogEventBase& Event) : Counter(1)
		{
			Name = Event.Name;
			UserFriendlyDesc = Event.FriendlyDesc;
			Verbosity = Event.Verbosity;
		}

		FLogEvent& operator = (const FVisualLogEventBase& Event)
		{
			Name = Event.Name;
			UserFriendlyDesc = Event.FriendlyDesc;
			Verbosity = Event.Verbosity;
			return *this;
		}
	};
	struct FLogLine
	{
		FString Line;
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		int32 UniqueId;
		int64 UserData;
		FName TagName;

		FLogLine() {}

		FLogLine(const FName& InCategory, ELogVerbosity::Type InVerbosity, const FString& InLine)
			: Line(InLine), Category(InCategory), Verbosity(InVerbosity), UserData(0)
		{}

		FLogLine(const FName& InCategory, ELogVerbosity::Type InVerbosity, const FString& InLine, int64 InUserData)
			: Line(InLine), Category(InCategory), Verbosity(InVerbosity), UserData(InUserData)
		{}
	};

	struct FStatusCategory
	{
		TArray<FString> Data;
		FString Category;
		int32 UniqueId;

		FORCEINLINE void Add(const FString& Key, const FString& Value)
		{
			Data.Add(FString(Key).AppendChar(TEXT('|')) + Value);
		}

		FORCEINLINE bool GetDesc(int32 Index, FString& Key, FString& Value) const
		{
			if (Data.IsValidIndex(Index))
			{
				int32 SplitIdx = INDEX_NONE;
				if (Data[Index].FindChar(TEXT('|'), SplitIdx))
				{
					Key = Data[Index].Left(SplitIdx);
					Value = Data[Index].Mid(SplitIdx + 1);
					return true;
				}
			}

			return false;
		}
	};

	struct FElementToDraw
	{
		friend struct FVisualLogEntry;

		enum EElementType {
			Invalid = 0,
			SinglePoint, // individual points. 
			Segment, // pairs of points 
			Path,	// sequence of point
			Box,
			Cone,
			Cylinder,
			Capsule,
			// note that in order to remain backward compatibility in terms of log
			// serialization new enum values need to be added at the end
		};

		FString Description;
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		TArray<FVector> Points;
		int32 UniqueId;

	public:
		uint8 Type;
		uint8 Color;
		union
		{
			uint16 Thicknes;
			uint16 Radius;
		};

		FElementToDraw(EElementType InType = Invalid) : Verbosity(ELogVerbosity::All), Type(InType), Color(0xff), Thicknes(0)
		{}

		FElementToDraw(const FString& InDescription, const FColor& InColor, uint16 InThickness, const FName& InCategory) : Category(InCategory), Verbosity(ELogVerbosity::All), Type(uint16(Invalid)), Thicknes(InThickness)
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
				| ((InColor.DWColor() & 0x000000ff) >> 6);
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

	struct FHistogramSample
	{
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		FName GraphName;
		FName DataName;
		FVector2D SampleValue;
		int32 UniqueId;
	};

	struct FDataBlock
	{
		FName TagName;
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		TArray<uint8>	Data;
		int32 UniqueId;
	};

#if ENABLE_VISUAL_LOG
		float TimeStamp;
		FVector Location;

		TArray<FLogEvent> Events;
		TArray<FLogLine> LogLines;
		TArray<FStatusCategory> Status;
		TArray<FElementToDraw> ElementsToDraw;
		TArray<FHistogramSample>	HistogramSamples;
		TArray<FDataBlock>	DataBlocks;

		FVisualLogEntry() { Reset(); }
		FVisualLogEntry(const FVisualLogEntry& Entry);
		FVisualLogEntry(const class AActor* InActor, TArray<TWeakObjectPtr<UObject> >* Children);
		FVisualLogEntry(float InTimeStamp, FVector InLocation, const UObject* Object, TArray<TWeakObjectPtr<UObject> >* Children);

		void Reset()
		{
			TimeStamp = -1;
			Location = FVector::ZeroVector;
			Events.Reset();
			LogLines.Reset();
			Status.Reset();
			ElementsToDraw.Reset();
			HistogramSamples.Reset();
			DataBlocks.Reset();
		}

		void AddText(const FString& TextLine, const FName& CategoryName);
		// path
		void AddElement(const TArray<FVector>& Points, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
		// location
		void AddElement(const FVector& Point, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
		// segment
		void AddElement(const FVector& Start, const FVector& End, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
		// box
		void AddElement(const FBox& Box, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
		// Cone
		void AddElement(const FVector& Orgin, const FVector& Direction, float Length, float AngleWidth, float AngleHeight, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
		// Cylinder
		void AddElement(const FVector& Start, const FVector& End, float Radius, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
		// histogram sample
		void AddHistogramData(const FVector2D& DataSample, const FName& CategoryName, const FName& GraphName, const FName& DataName);
		// Custom data block
		FVisualLogEntry::FDataBlock& AddDataBlock(const FString& TagName, const TArray<uint8>& BlobDataArray, const FName& CategoryName);
		// capsule
		void AddCapsule(FVector const& Center, float HalfHeight, float Radius, const FQuat & Rotation, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""));
		// Event
		int32 AddEvent(const FVisualLogEventBase& Event);

		int32 FindStatusIndex(const FString& CategoryName);

		// find index of status category

#endif // ENABLE_VISUAL_LOG
};

#if  ENABLE_VISUAL_LOG
FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry::FDataBlock& Data)
{
	Ar << Data.TagName;
	Ar << Data.Category;
	Ar << Data.Verbosity;
	Ar << Data.Data;
	Ar << Data.UniqueId;

	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry::FHistogramSample& Sample)
{
	Ar << Sample.Category;
	Ar << Sample.Verbosity;
	Ar << Sample.GraphName;
	Ar << Sample.DataName;
	Ar << Sample.SampleValue;
	Ar << Sample.UniqueId;

	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry::FElementToDraw& Element)
{
	Ar << Element.Description;
	Ar << Element.Category;
	Ar << Element.Verbosity;
	Ar << Element.Points;
	Ar << Element.UniqueId;
	Ar << Element.Type;
	Ar << Element.Color;
	Ar << Element.Thicknes;

	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry::FLogEvent& Event)
{
	Ar << Event.Name;
	Ar << Event.UserFriendlyDesc;
	Ar << Event.Verbosity;
	Ar << Event.EventTags;
	Ar << Event.Counter;
	Ar << Event.UserData;
	Ar << Event.TagName;

	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry::FLogLine& LogLine)
{
	Ar << LogLine.Category;
	Ar << LogLine.Verbosity;
	Ar << LogLine.TagName;
	Ar << LogLine.UniqueId;
	Ar << LogLine.UserData;
	Ar << LogLine.Line;
	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry::FStatusCategory& Status)
{
	Ar << Status.Category;
	Ar << Status.Data;

	return Ar;
}

FORCEINLINE
FArchive& operator<<(FArchive& Ar, FVisualLogEntry& LogEntry)
{
	Ar << LogEntry.TimeStamp;
	Ar << LogEntry.Location;
	Ar << LogEntry.LogLines;
	Ar << LogEntry.Status;
	Ar << LogEntry.Events;
	Ar << LogEntry.ElementsToDraw;

	return Ar;
}

FORCEINLINE int32 FVisualLogEntry::FindStatusIndex(const FString& CategoryName)
{
	for (int32 TestCategoryIndex = 0; TestCategoryIndex < Status.Num(); TestCategoryIndex++)
	{
		if (Status[TestCategoryIndex].Category == CategoryName)
		{
			return TestCategoryIndex;
		}
	}

	return INDEX_NONE;
}

FORCEINLINE bool operator==(const FVisualLogEntry::FLogEvent& Left, const FVisualLogEntry::FLogEvent& Right)
{
	return Left.Name == Right.Name;
}

struct FVisualLoggerHelpers
{
	static FString GenerateTemporaryFilename(const FString& FileExt);
	static FString GenerateFilename(const FString& TempFileName, const FString& Prefix, float StartRecordingTime, float EndTimeStamp);
};

FORCEINLINE 
FString FVisualLoggerHelpers::GenerateTemporaryFilename(const FString& FileExt)
{ 
	return FString::Printf(TEXT("VTEMP_%s.%s"), *FDateTime::Now().ToString(), *FileExt);
}

FORCEINLINE
FString FVisualLoggerHelpers::GenerateFilename(const FString& TempFileName, const FString& Prefix, float StartRecordingTime, float EndTimeStamp)
{
	const FString TempFullFilename = FString::Printf(TEXT("%slogs/%s"), *FPaths::GameSavedDir(), *TempFileName);
	const FString FullFilename = FString::Printf(TEXT("%slogs/%s_%s"), *FPaths::GameSavedDir(), *Prefix, *TempFileName);
	const FString TimeFrameString = FString::Printf(TEXT("%d-%d_"), FMath::TruncToInt(StartRecordingTime), FMath::TruncToInt(EndTimeStamp));
	return FullFilename.Replace(TEXT("VTEMP_"), *TimeFrameString, ESearchCase::CaseSensitive);
}

#endif // ENABLE_VISUAL_LOG

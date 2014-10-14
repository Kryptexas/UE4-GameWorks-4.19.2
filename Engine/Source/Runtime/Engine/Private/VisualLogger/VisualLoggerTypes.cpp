// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"
#include "VisualLogger/VisualLogger.h"

#if ENABLE_VISUAL_LOG

FVisualLogEntry::FVisualLogEntry(const FVisualLogEntry& Entry)
{
	TimeStamp = Entry.TimeStamp;
	Location = Entry.Location;

	Events = Entry.Events;
	LogLines = Entry.LogLines;
	Status = Entry.Status;
	ElementsToDraw = Entry.ElementsToDraw;
	HistogramSamples = Entry.HistogramSamples;
	DataBlocks = Entry.DataBlocks;
}

FVisualLogEntry::FVisualLogEntry(const class AActor* InActor, TArray<TWeakObjectPtr<UObject> >* Children)
{
	if (InActor && InActor->IsPendingKill() == false)
	{
		TimeStamp = InActor->GetWorld()->TimeSeconds;
		Location = InActor->GetActorLocation();
		InActor->GrabDebugSnapshot(this);
		if (Children != NULL)
		{
			TWeakObjectPtr<UObject>* WeakActorPtr = Children->GetData();
			for (int32 Index = 0; Index < Children->Num(); ++Index, ++WeakActorPtr)
			{
				if (WeakActorPtr->IsValid())
				{
					const AActor* ChildActor = Cast<AActor>(WeakActorPtr->Get());
					if (ChildActor)
					{
						ChildActor->GrabDebugSnapshot(this);
					}
				}
			}
		}
	}
}

FVisualLogEntry::FVisualLogEntry(float InTimeStamp, FVector InLocation, const UObject* Object, TArray<TWeakObjectPtr<UObject> >* Children)
{
	TimeStamp = InTimeStamp;
	Location = InLocation;
	const AActor* AsActor = Cast<AActor>(Object);
	if (AsActor)
	{
		AsActor->GrabDebugSnapshot(this);
	}
	if (Children != NULL)
	{
		TWeakObjectPtr<UObject>* WeakActorPtr = Children->GetData();
		for (int32 Index = 0; Index < Children->Num(); ++Index, ++WeakActorPtr)
		{
			if (WeakActorPtr->IsValid())
			{
				const AActor* ChildActor = Cast<AActor>(WeakActorPtr->Get());
				if (ChildActor)
				{
					ChildActor->GrabDebugSnapshot(this);
				}
			}
		}
	}
}

int32 FVisualLogEntry::AddEvent(const FVisualLogEventBase& Event)
{
	return Events.Add(Event);
}

void FVisualLogEntry::AddText(const FString& TextLine, const FName& CategoryName)
{
	LogLines.Add(FLogLine(CategoryName, ELogVerbosity::All, TextLine));
}

void FVisualLogEntry::AddElement(const TArray<FVector>& Points, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points = Points;
	Element.Type = FElementToDraw::Path;
	ElementsToDraw.Add(Element);
}

void FVisualLogEntry::AddElement(const FVector& Point, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Add(Point);
	Element.Type = FElementToDraw::SinglePoint;
	ElementsToDraw.Add(Element);
}

void FVisualLogEntry::AddElement(const FVector& Start, const FVector& End, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Reserve(2);
	Element.Points.Add(Start);
	Element.Points.Add(End);
	Element.Type = FElementToDraw::Segment;
	ElementsToDraw.Add(Element);
}

void FVisualLogEntry::AddElement(const FBox& Box, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Reserve(2);
	Element.Points.Add(Box.Min);
	Element.Points.Add(Box.Max);
	Element.Type = FElementToDraw::Box;
	ElementsToDraw.Add(Element);
}

void FVisualLogEntry::AddElement(const FVector& Orgin, const FVector& Direction, float Length, float AngleWidth, float AngleHeight, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Reserve(3);
	Element.Points.Add(Orgin);
	Element.Points.Add(Direction);
	Element.Points.Add(FVector(Length, AngleWidth, AngleHeight));
	Element.Type = FElementToDraw::Cone;
	ElementsToDraw.Add(Element);
}

void FVisualLogEntry::AddElement(const FVector& Start, const FVector& End, float Radius, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Reserve(3);
	Element.Points.Add(Start);
	Element.Points.Add(End);
	Element.Points.Add(FVector(Radius, Thickness, 0));
	Element.Type = FElementToDraw::Cylinder;
	ElementsToDraw.Add(Element);
}

void FVisualLogEntry::AddHistogramData(const FVector2D& DataSample, const FName& CategoryName, const FName& GraphName, const FName& DataName)
{
	FHistogramSample Sample;
	Sample.Category = CategoryName;
	Sample.GraphName = GraphName;
	Sample.DataName = DataName;
	Sample.SampleValue = DataSample;

	HistogramSamples.Add(Sample);
}

FVisualLogEntry::FDataBlock& FVisualLogEntry::AddDataBlock(const FString& TagName, const TArray<uint8>& BlobDataArray, const FName& CategoryName)
{
	FDataBlock DataBlock;
	DataBlock.Category = CategoryName;
	DataBlock.TagName = *TagName;
	DataBlock.Data = BlobDataArray;

	const int32 Index = DataBlocks.Add(DataBlock);
	return DataBlocks[Index];
}

void FVisualLogEntry::AddCapsule(FVector const& Center, float HalfHeight, float Radius, const FQuat & Rotation, const FName& CategoryName, const FColor& Color, const FString& Description)
{
	FElementToDraw Element(Description, Color, 0, CategoryName);
	Element.Points.Reserve(3);
	Element.Points.Add(Center);
	Element.Points.Add(FVector(HalfHeight, Radius, Rotation.X));
	Element.Points.Add(FVector(Rotation.Y, Rotation.Z, Rotation.W));
	Element.Type = FElementToDraw::Capsule;
	ElementsToDraw.Add(Element);
}

#endif //ENABLE_VISUAL_LOG


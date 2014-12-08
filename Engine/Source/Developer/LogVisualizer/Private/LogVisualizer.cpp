// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "Misc/Base64.h"
#include "Json.h"
#include "CollisionDebugDrawingPublic.h"
#include "LogVisualizerDebugActor.h"

#if ENABLE_VISUAL_LOG
//////////////////////////////////////////////////////////////////////////

TSharedPtr<FJsonValue> GenerateJson(FVisualLogEntry* InEntry)
{
	TSharedPtr<FJsonObject> JsonEntryObject = MakeShareable(new FJsonObject);

	JsonEntryObject->SetNumberField(VisualLogJson::TAG_TIMESTAMP, InEntry->TimeStamp);
	JsonEntryObject->SetStringField(VisualLogJson::TAG_LOCATION, InEntry->Location.ToString());

	const int32 StatusCategoryCount = InEntry->Status.Num();
	const FVisualLogStatusCategory* StatusCategory = InEntry->Status.GetData();
	TArray< TSharedPtr<FJsonValue> > JsonStatus;
	JsonStatus.AddZeroed(StatusCategoryCount);

	for (int32 StatusIdx = 0; StatusIdx < InEntry->Status.Num(); StatusIdx++, StatusCategory++)
	{
		TSharedPtr<FJsonObject> JsonStatusCategoryObject = MakeShareable(new FJsonObject);
		JsonStatusCategoryObject->SetStringField(VisualLogJson::TAG_CATEGORY, StatusCategory->Category);

		TArray< TSharedPtr<FJsonValue> > JsonStatusLines;
		JsonStatusLines.AddZeroed(StatusCategory->Data.Num());
		for (int32 LineIdx = 0; LineIdx < StatusCategory->Data.Num(); LineIdx++)
		{
			JsonStatusLines[LineIdx] = MakeShareable(new FJsonValueString(StatusCategory->Data[LineIdx]));
		}

		JsonStatusCategoryObject->SetArrayField(VisualLogJson::TAG_STATUSLINES, JsonStatusLines);
		JsonStatus[StatusIdx] = MakeShareable(new FJsonValueObject(JsonStatusCategoryObject));
	}
	JsonEntryObject->SetArrayField(VisualLogJson::TAG_STATUS, JsonStatus);

	const int32 LogLineCount = InEntry->LogLines.Num();
	const FVisualLogLine* LogLine = InEntry->LogLines.GetData();
	TArray< TSharedPtr<FJsonValue> > JsonLines;
	JsonLines.AddZeroed(LogLineCount);

	for (int32 LogLineIndex = 0; LogLineIndex < LogLineCount; ++LogLineIndex, ++LogLine)
	{
		TSharedPtr<FJsonObject> JsonLogLineObject = MakeShareable(new FJsonObject);
		JsonLogLineObject->SetStringField(VisualLogJson::TAG_CATEGORY, LogLine->Category.ToString());
		JsonLogLineObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, LogLine->Verbosity);
		JsonLogLineObject->SetStringField(VisualLogJson::TAG_LINE, LogLine->Line);
		JsonLogLineObject->SetStringField(VisualLogJson::TAG_TAGNAME, LogLine->TagName.ToString());
		JsonLogLineObject->SetNumberField(VisualLogJson::TAG_USERDATA, LogLine->UserData);
		JsonLines[LogLineIndex] = MakeShareable(new FJsonValueObject(JsonLogLineObject));
	}

	JsonEntryObject->SetArrayField(VisualLogJson::TAG_LOGLINES, JsonLines);

	const int32 ElementsToDrawCount = InEntry->ElementsToDraw.Num();
	const FVisualLogShapeElement* Element = InEntry->ElementsToDraw.GetData();
	TArray< TSharedPtr<FJsonValue> > JsonElementsToDraw;
	JsonElementsToDraw.AddZeroed(ElementsToDrawCount);

	for (int32 ElementIndex = 0; ElementIndex < ElementsToDrawCount; ++ElementIndex, ++Element)
	{
		TSharedPtr<FJsonObject> JsonElementToDrawObject = MakeShareable(new FJsonObject);

		JsonElementToDrawObject->SetStringField(VisualLogJson::TAG_DESCRIPTION, Element->Description);
		JsonElementToDrawObject->SetStringField(VisualLogJson::TAG_CATEGORY, Element->Category.ToString());
		JsonElementToDrawObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, Element->Verbosity);

		const int32 EncodedTypeColorSize = uint8(Element->Type) << 24 | Element->Color << 16 | Element->Thicknes;
		JsonElementToDrawObject->SetStringField(VisualLogJson::TAG_TYPECOLORSIZE, FString::Printf(TEXT("%d"), EncodedTypeColorSize));

		const int32 PointsCount = Element->Points.Num();
		TArray< TSharedPtr<FJsonValue> > JsonStringPoints;
		JsonStringPoints.AddZeroed(PointsCount);
		const FVector* PointToDraw = Element->Points.GetData();
		for (int32 PointIndex = 0; PointIndex < PointsCount; ++PointIndex, ++PointToDraw)
		{
			JsonStringPoints[PointIndex] = MakeShareable(new FJsonValueString(PointToDraw->ToString()));
		}
		JsonElementToDrawObject->SetArrayField(VisualLogJson::TAG_POINTS, JsonStringPoints);

		JsonElementsToDraw[ElementIndex] = MakeShareable(new FJsonValueObject(JsonElementToDrawObject));
	}

	JsonEntryObject->SetArrayField(VisualLogJson::TAG_ELEMENTSTODRAW, JsonElementsToDraw);

	const int32 HistogramSamplesCount = InEntry->HistogramSamples.Num();
	const FVisualLogHistogramSample* Sample = InEntry->HistogramSamples.GetData();
	TArray<TSharedPtr<FJsonValue> > JsonHistogramSamples;
	JsonHistogramSamples.AddZeroed(HistogramSamplesCount);

	for (int32 SampleIndex = 0; SampleIndex < HistogramSamplesCount; ++SampleIndex, ++Sample)
	{
		TSharedPtr<FJsonObject> JsonSampleObject = MakeShareable(new FJsonObject);

		JsonSampleObject->SetStringField(VisualLogJson::TAG_CATEGORY, Sample->Category.ToString());
		JsonSampleObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, Sample->Verbosity);
		JsonSampleObject->SetStringField(VisualLogJson::TAG_HISTOGRAMSAMPLE, Sample->SampleValue.ToString());
		JsonSampleObject->SetStringField(VisualLogJson::TAG_HISTOGRAMGRAPHNAME, Sample->GraphName.ToString());
		JsonSampleObject->SetStringField(VisualLogJson::TAG_HISTOGRAMDATANAME, Sample->DataName.ToString());

		JsonHistogramSamples[SampleIndex] = MakeShareable(new FJsonValueObject(JsonSampleObject));
	}

	JsonEntryObject->SetArrayField(VisualLogJson::TAG_HISTOGRAMSAMPLES, JsonHistogramSamples);

	// generate json from events
	{
		const int32 EventsCount = InEntry->Events.Num();
		TArray<TSharedPtr<FJsonValue> > JsonEventSamples;
		JsonEventSamples.Reserve(HistogramSamplesCount);

		const FVisualLogHistogramSample* Sample = InEntry->HistogramSamples.GetData();
		for (auto& CurrentEvent : InEntry->Events)
		{
			TSharedPtr<FJsonObject> JsonSampleObject = MakeShareable(new FJsonObject);

			JsonSampleObject->SetStringField(VisualLogJson::TAG_CATEGORY, CurrentEvent.Name);
			JsonSampleObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, CurrentEvent.Verbosity);
			JsonSampleObject->SetStringField(VisualLogJson::TAG_TAGNAME, CurrentEvent.TagName.ToString());
			JsonSampleObject->SetNumberField(VisualLogJson::TAG_USERDATA, CurrentEvent.UserData);
			JsonSampleObject->SetStringField(VisualLogJson::TAG_DESCRIPTION, CurrentEvent.UserFriendlyDesc);
			JsonSampleObject->SetNumberField(VisualLogJson::TAG_COUNTER, CurrentEvent.Counter);

			TArray<TSharedPtr<FJsonValue> > JsonEventTags;
			JsonEventTags.Reserve(HistogramSamplesCount);
			for (auto& CurrentTag : CurrentEvent.EventTags)
			{
				TSharedPtr<FJsonObject> JsonEventObject = MakeShareable(new FJsonObject);
				JsonEventObject->SetStringField(VisualLogJson::TAG_EVENTNAME, CurrentTag.Key.ToString());
				JsonEventObject->SetNumberField(VisualLogJson::TAG_COUNTER, CurrentTag.Value);

				JsonEventTags.Add(MakeShareable(new FJsonValueObject(JsonEventObject)));
			}
			JsonSampleObject->SetArrayField(VisualLogJson::TAG_EVENTAGS, JsonEventTags);

			JsonEventSamples.Add(MakeShareable(new FJsonValueObject(JsonSampleObject)));
		}

		JsonEntryObject->SetArrayField(VisualLogJson::TAG_EVENTSAMPLES, JsonEventSamples);
	}

	int32 DataBlockIndex = 0;
	TArray<TSharedPtr<FJsonValue> > JasonDataBlocks;
	JasonDataBlocks.AddZeroed(InEntry->DataBlocks.Num());
	for (const auto CurrentData : InEntry->DataBlocks)
	{
		TSharedPtr<FJsonObject> JsonSampleObject = MakeShareable(new FJsonObject);

		TArray<uint8> CompressedData;
		const int32 UncompressedSize = CurrentData.Data.Num();
		const int32 HeaderSize = sizeof(int32);
		CompressedData.Init(0, HeaderSize + FMath::TruncToInt(1.1f * UncompressedSize));

		int32 CompressedSize = CompressedData.Num() - HeaderSize;
		uint8* DestBuffer = CompressedData.GetData();
		FMemory::Memcpy(DestBuffer, &UncompressedSize, HeaderSize);
		DestBuffer += HeaderSize;

		FCompression::CompressMemory((ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory), (void*)DestBuffer, CompressedSize, (void*)CurrentData.Data.GetData(), UncompressedSize);
		CompressedData.SetNum(CompressedSize + HeaderSize, true);
		const FString CurrentDataAsString = FBase64::Encode(CompressedData);

		JsonSampleObject->SetStringField(VisualLogJson::TAG_CATEGORY, CurrentData.Category.ToString());
		JsonSampleObject->SetStringField(VisualLogJson::TAG_TAGNAME, CurrentData.TagName.ToString());
		JsonSampleObject->SetStringField(VisualLogJson::TAG_DATABLOCK_DATA, CurrentDataAsString);
		JsonSampleObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, CurrentData.Verbosity);

		JasonDataBlocks[DataBlockIndex++] = MakeShareable(new FJsonValueObject(JsonSampleObject));
	}
	JsonEntryObject->SetArrayField(VisualLogJson::TAG_DATABLOCK, JasonDataBlocks);

	return MakeShareable(new FJsonValueObject(JsonEntryObject));
}

TSharedPtr<FVisualLogEntry> JsonToVisualLogEntry(TSharedPtr<FJsonValue> FromJson)
{
	TSharedPtr<FJsonObject> JsonEntryObject = FromJson->AsObject();
	if (JsonEntryObject.IsValid() == false)
	{
		return NULL;
	}

	TSharedPtr<FVisualLogEntry> VisLogEntry = MakeShareable(new FVisualLogEntry());
	VisLogEntry->TimeStamp = JsonEntryObject->GetNumberField(VisualLogJson::TAG_TIMESTAMP);
	VisLogEntry->Location.InitFromString(JsonEntryObject->GetStringField(VisualLogJson::TAG_LOCATION));

	TArray< TSharedPtr<FJsonValue> > JsonStatus = JsonEntryObject->GetArrayField(VisualLogJson::TAG_STATUS);
	if (JsonStatus.Num() > 0)
	{
		VisLogEntry->Status.AddZeroed(JsonStatus.Num());
		for (int32 StatusIndex = 0; StatusIndex < JsonStatus.Num(); ++StatusIndex)
		{
			TSharedPtr<FJsonObject> JsonStatusCategory = JsonStatus[StatusIndex]->AsObject();
			if (JsonStatusCategory.IsValid())
			{
				VisLogEntry->Status[StatusIndex].Category = JsonStatusCategory->GetStringField(VisualLogJson::TAG_CATEGORY);

				TArray< TSharedPtr<FJsonValue> > JsonStatusLines = JsonStatusCategory->GetArrayField(VisualLogJson::TAG_STATUSLINES);
				if (JsonStatusLines.Num() > 0)
				{
					VisLogEntry->Status[StatusIndex].Data.AddZeroed(JsonStatusLines.Num());
					for (int32 LineIdx = 0; LineIdx < JsonStatusLines.Num(); LineIdx++)
					{
						VisLogEntry->Status[StatusIndex].Data.Add(JsonStatusLines[LineIdx]->AsString());
					}
				}
			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JsonLines = JsonEntryObject->GetArrayField(VisualLogJson::TAG_LOGLINES);
	if (JsonLines.Num() > 0)
	{
		VisLogEntry->LogLines.AddZeroed(JsonLines.Num());
		for (int32 LogLineIndex = 0; LogLineIndex < JsonLines.Num(); ++LogLineIndex)
		{
			TSharedPtr<FJsonObject> JsonLogLine = JsonLines[LogLineIndex]->AsObject();
			if (JsonLogLine.IsValid())
			{
				VisLogEntry->LogLines[LogLineIndex].Category = FName(*(JsonLogLine->GetStringField(VisualLogJson::TAG_CATEGORY)));
				VisLogEntry->LogLines[LogLineIndex].Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonLogLine->GetNumberField(VisualLogJson::TAG_VERBOSITY)));
				VisLogEntry->LogLines[LogLineIndex].Line = JsonLogLine->GetStringField(VisualLogJson::TAG_LINE);
				VisLogEntry->LogLines[LogLineIndex].TagName = FName(*(JsonLogLine->GetStringField(VisualLogJson::TAG_TAGNAME)));
				VisLogEntry->LogLines[LogLineIndex].UserData = (int64)(JsonLogLine->GetNumberField(VisualLogJson::TAG_USERDATA));

			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JsonElementsToDraw = JsonEntryObject->GetArrayField(VisualLogJson::TAG_ELEMENTSTODRAW);
	if (JsonElementsToDraw.Num() > 0)
	{
		VisLogEntry->ElementsToDraw.Reserve(JsonElementsToDraw.Num());
		for (int32 ElementIndex = 0; ElementIndex < JsonElementsToDraw.Num(); ++ElementIndex)
		{
			TSharedPtr<FJsonObject> JsonElementObject = JsonElementsToDraw[ElementIndex]->AsObject();
			if (JsonElementObject.IsValid())
			{
				FVisualLogShapeElement& Element = VisLogEntry->ElementsToDraw[VisLogEntry->ElementsToDraw.Add(FVisualLogShapeElement())];

				Element.Description = JsonElementObject->GetStringField(VisualLogJson::TAG_DESCRIPTION);
				Element.Category = FName(*(JsonElementObject->GetStringField(VisualLogJson::TAG_CATEGORY)));
				Element.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonElementObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));

				// Element->Type << 24 | Element->Color << 16 | Element->Thicknes;
				int32 EncodedTypeColorSize = 0;
				TTypeFromString<int32>::FromString(EncodedTypeColorSize, *(JsonElementObject->GetStringField(VisualLogJson::TAG_TYPECOLORSIZE)));
				Element.Type = EVisualLoggerShapeElement(EncodedTypeColorSize >> 24);
				Element.Color = (EncodedTypeColorSize << 8) >> 24;
				Element.Thicknes = (EncodedTypeColorSize << 16) >> 16;

				TArray< TSharedPtr<FJsonValue> > JsonPoints = JsonElementObject->GetArrayField(VisualLogJson::TAG_POINTS);
				if (JsonPoints.Num() > 0)
				{
					Element.Points.AddUninitialized(JsonPoints.Num());
					for (int32 PointIndex = 0; PointIndex < JsonPoints.Num(); ++PointIndex)
					{
						Element.Points[PointIndex].InitFromString(JsonPoints[PointIndex]->AsString());
					}
				}
			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JsonHistogramSamples = JsonEntryObject->GetArrayField(VisualLogJson::TAG_HISTOGRAMSAMPLES);
	if (JsonHistogramSamples.Num() > 0)
	{
		VisLogEntry->HistogramSamples.Reserve(JsonHistogramSamples.Num());
		for (int32 SampleIndex = 0; SampleIndex < JsonHistogramSamples.Num(); ++SampleIndex)
		{
			TSharedPtr<FJsonObject> JsonSampleObject = JsonHistogramSamples[SampleIndex]->AsObject();
			if (JsonSampleObject.IsValid())
			{
				FVisualLogHistogramSample& Sample = VisLogEntry->HistogramSamples[VisLogEntry->HistogramSamples.Add(FVisualLogHistogramSample())];

				Sample.Category = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_CATEGORY)));
				Sample.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonSampleObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));
				Sample.GraphName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_HISTOGRAMGRAPHNAME)));
				Sample.DataName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_HISTOGRAMDATANAME)));
				Sample.SampleValue.InitFromString(JsonSampleObject->GetStringField(VisualLogJson::TAG_HISTOGRAMSAMPLE));
			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JsonEventSamples = JsonEntryObject->GetArrayField(VisualLogJson::TAG_EVENTSAMPLES);
	if (JsonEventSamples.Num() > 0)
	{
		VisLogEntry->Events.Reserve(JsonEventSamples.Num());
		for (int32 SampleIndex = 0; SampleIndex < JsonEventSamples.Num(); ++SampleIndex)
		{
			TSharedPtr<FJsonObject> JsonSampleObject = JsonEventSamples[SampleIndex]->AsObject();
			if (JsonSampleObject.IsValid())
			{
				FVisualLogEvent& Event = VisLogEntry->Events[VisLogEntry->Events.Add(FVisualLogEvent())];
				Event.Name = JsonSampleObject->GetStringField(VisualLogJson::TAG_CATEGORY);
				Event.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonSampleObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));
				Event.TagName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_TAGNAME)));
				Event.UserData = (int64)(JsonSampleObject->GetNumberField(VisualLogJson::TAG_USERDATA));
				Event.UserFriendlyDesc = JsonSampleObject->GetStringField(VisualLogJson::TAG_DESCRIPTION);
				Event.Counter = (int64)(JsonSampleObject->GetNumberField(VisualLogJson::TAG_COUNTER));

				TArray< TSharedPtr<FJsonValue> > JsonEventTags = JsonSampleObject->GetArrayField(VisualLogJson::TAG_EVENTAGS);
				if (JsonEventTags.Num() > 0)
				{
					Event.EventTags.Reserve(JsonEventSamples.Num());
					for (int32 TagIndex = 0; TagIndex < JsonEventTags.Num(); ++TagIndex)
					{
						TSharedPtr<FJsonObject> JsonTagObject = JsonEventTags[TagIndex]->AsObject();
						if (JsonTagObject.IsValid())
						{
							FString TagName = JsonTagObject->GetStringField(VisualLogJson::TAG_EVENTNAME);
							int32 TagCounter = JsonTagObject->GetNumberField(VisualLogJson::TAG_COUNTER);
							Event.EventTags.Add(*TagName, TagCounter);
						}
					}
				}
			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JasonDataBlocks = JsonEntryObject->GetArrayField(VisualLogJson::TAG_DATABLOCK);
	if (JasonDataBlocks.Num() > 0)
	{
		VisLogEntry->DataBlocks.Reserve(JasonDataBlocks.Num());
		for (int32 SampleIndex = 0; SampleIndex < JasonDataBlocks.Num(); ++SampleIndex)
		{
			TSharedPtr<FJsonObject> JsonSampleObject = JasonDataBlocks[SampleIndex]->AsObject();
			if (JsonSampleObject.IsValid())
			{
				FVisualLogDataBlock& Sample = VisLogEntry->DataBlocks[VisLogEntry->DataBlocks.Add(FVisualLogDataBlock())];
				Sample.TagName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_TAGNAME)));
				Sample.Category = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_CATEGORY)));
				Sample.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonSampleObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));

				// decode data from Base64 string
				const FString DataBlockAsCompressedString = JsonSampleObject->GetStringField(VisualLogJson::TAG_DATABLOCK_DATA);
				TArray<uint8> CompressedDataBlock;
				FBase64::Decode(DataBlockAsCompressedString, CompressedDataBlock);

				// uncompress decoded data to get final TArray<uint8>
				int32 UncompressedSize = 0;
				const int32 HeaderSize = sizeof(int32);
				uint8* SrcBuffer = (uint8*)CompressedDataBlock.GetData();
				FMemory::Memcpy(&UncompressedSize, SrcBuffer, HeaderSize);
				SrcBuffer += HeaderSize;
				const int32 CompressedSize = CompressedDataBlock.Num() - HeaderSize;

				Sample.Data.AddZeroed(UncompressedSize);
				FCompression::UncompressMemory((ECompressionFlags)(COMPRESS_ZLIB), (void*)Sample.Data.GetData(), UncompressedSize, SrcBuffer, CompressedSize);
			}
		}
	}

	return VisLogEntry;
}
//----------------------------------------------------------------------//
// FActorsVisLog
//----------------------------------------------------------------------//
FActorsVisLog::FActorsVisLog(FName InName, TArray<TWeakObjectPtr<UObject> >* Children)
	: Name(InName)
{
	Entries.Reserve(VisLogInitialSize);
}

FActorsVisLog::FActorsVisLog(const class UObject* Object, TArray<TWeakObjectPtr<UObject> >* Children)
	: Name(Object->GetFName())
	, FullName(Object->GetFullName())
{
	Entries.Reserve(VisLogInitialSize);
}

FActorsVisLog::FActorsVisLog(TSharedPtr<FJsonValue> FromJson)
{
	TSharedPtr<FJsonObject> JsonLogObject = FromJson->AsObject();
	if (JsonLogObject.IsValid() == false)
	{
		return;
	}

	Name = FName(*JsonLogObject->GetStringField(VisualLogJson::TAG_NAME));
	FullName = JsonLogObject->GetStringField(VisualLogJson::TAG_FULLNAME);

	TArray< TSharedPtr<FJsonValue> > JsonEntries = JsonLogObject->GetArrayField(VisualLogJson::TAG_ENTRIES);
	if (JsonEntries.Num() > 0)
	{
		Entries.AddZeroed(JsonEntries.Num());
		for (int32 EntryIndex = 0; EntryIndex < JsonEntries.Num(); ++EntryIndex)
		{
			Entries[EntryIndex] = JsonToVisualLogEntry(JsonEntries[EntryIndex]);
		}
	}
}

TSharedPtr<FJsonValue> FActorsVisLog::ToJson() const
{
	TSharedPtr<FJsonObject> JsonLogObject = MakeShareable(new FJsonObject);

	JsonLogObject->SetStringField(VisualLogJson::TAG_NAME, Name.ToString());
	JsonLogObject->SetStringField(VisualLogJson::TAG_FULLNAME, FullName);

	const TSharedPtr<FVisualLogEntry>* Entry = Entries.GetData();
	const int32 EntryCount = Entries.Num();
	TArray< TSharedPtr<FJsonValue> > JsonLogEntries;
	JsonLogEntries.AddZeroed(EntryCount);

	for (int32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex, ++Entry)
	{
		JsonLogEntries[EntryIndex] = GenerateJson((*Entry).Get());
	}

	JsonLogObject->SetArrayField(VisualLogJson::TAG_ENTRIES, JsonLogEntries);

	return MakeShareable(new FJsonValueObject(JsonLogObject));
}

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

		auto &Devices = FVisualLogger::Get().GetDevices();
		for (auto CurrentDevice : Devices)
		{
			if (CurrentDevice != NULL && CurrentDevice->HasFlags(EVisualLoggerDeviceFlags::StoreLogsLocally))
			{
				TArray<FVisualLogDevice::FVisualLogEntryItem> RecordedLogs;
				CurrentDevice->GetRecordedLogs(RecordedLogs);
				PullDataFromVisualLog(RecordedLogs);
				break;
			}

		}
		FVisualLogger& VisualLog = FVisualLogger::Get();
		VisualLog.AddDevice(this);

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
			DebugActor = NULL;

			CleanUp();
			FSlateApplication::Get().RequestDestroyWindow(LogWindow.Pin().ToSharedRef());
		}
	}
	else
	{
		UE_LOG(LogLogVisualizer, Warning, TEXT("FLogVisualizer::CloseUI: Not in game thread."));
	}
}

bool FLogVisualizer::IsOpenUI(UWorld* InWorld)
{
	if (LogWindow.IsValid() && World.IsValid() && World == InWorld)
	{
		return true;
	}

	return false;
}

void FLogVisualizer::CleanUp()
{
	FVisualLogger& VisualLog = FVisualLogger::Get();
	VisualLog.RemoveDevice(this);
}

class AActor* FLogVisualizer::GetHelperActor(class UWorld* InWorld)
{
	UWorld* ActorWorld = DebugActor.IsValid() ? DebugActor->GetWorld() : NULL;
	if (DebugActor.IsValid() && ActorWorld == InWorld)
	{
		return DebugActor.Get();
	}

	for (TActorIterator<ALogVisualizerDebugActor> It(InWorld); It; ++It)
	{
		ALogVisualizerDebugActor* LogVisualizerDebugActor = *It;

		DebugActor = LogVisualizerDebugActor;
		return LogVisualizerDebugActor;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoCollisionFail = true;
	SpawnInfo.Name = *FString::Printf(TEXT("LogVisualizerDebugActor"));
	DebugActor = InWorld->SpawnActor<ALogVisualizerDebugActor>(SpawnInfo);

	return DebugActor.Get();
}

void FLogVisualizer::PullDataFromVisualLog(const TArray<FVisualLogDevice::FVisualLogEntryItem>& RecordedLogs)
{
	Logs.Reset();
	HelperMap.Reset();

	for (auto& CurrentLog : RecordedLogs)
	{
		if (HelperMap.Contains(CurrentLog.OwnerName) == false)
		{
			HelperMap.Add(CurrentLog.OwnerName, MakeShareable(new FActorsVisLog(CurrentLog.OwnerName, NULL)));
		}
		HelperMap[CurrentLog.OwnerName]->Entries.Add(MakeShareable(new FVisualLogEntry(CurrentLog.Entry)));
	}

	for (auto& CurrentLog : HelperMap)
	{
		Logs.Add(CurrentLog.Value);
		LogAddedEvent.Broadcast();
	}


}

void FLogVisualizer::OnNewLog(const UObject* Object, TSharedPtr<FActorsVisLog> Log)
{
	Logs.Add(Log);
	LogAddedEvent.Broadcast();
}

void FLogVisualizer::Serialize(const class UObject* LogOwner, FName OwnerName, const FVisualLogEntry& CurrentLog)
{
	const bool bContainsLogOwner = HelperMap.Contains(OwnerName);
	if (!bContainsLogOwner)
	{
		HelperMap.Add(OwnerName, MakeShareable(new FActorsVisLog(OwnerName, NULL)));
	}
	HelperMap[OwnerName]->Entries.Add(MakeShareable(new FVisualLogEntry(CurrentLog)));

	if (!bContainsLogOwner)
	{
		Logs.Add(HelperMap[OwnerName]);
		LogAddedEvent.Broadcast();
	}
}

void FLogVisualizer::AddLoadedLog(TSharedPtr<FActorsVisLog> Log)
{
	for (int32 Index = 0; Index < Logs.Num(); ++Index)
	{
		if (Logs[Index]->Name == Log->Name)
		{
			Logs[Index]->Entries.Append(Log->Entries);
			LogAddedEvent.Broadcast();
			return;
		}
	}

	if (Log.IsValid() && Log->Entries.Num() > 0)
	{
		Logs.Add(Log);
		LogAddedEvent.Broadcast();
	}
}

bool FLogVisualizer::IsRecording()
{
	return FVisualLogger::Get().IsRecording();
}

void FLogVisualizer::SetIsRecording(bool bNewRecording)
{
	FVisualLogger::Get().SetIsRecording(bNewRecording);
}

int32 FLogVisualizer::GetLogIndexForActor(const AActor* Actor)
{
	int32 ResultIndex = INDEX_NONE;
	if (!Actor)
	{
		return INDEX_NONE;
	}
	const FString FullName = Actor->GetFullName();
	TSharedPtr<FActorsVisLog>* Log = Logs.GetData();
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

#endif //ENABLE_VISUAL_LOG

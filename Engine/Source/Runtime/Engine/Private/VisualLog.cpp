// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Misc/Base64.h"
#include "VisualLog.h"
#include "Json.h"

#if ENABLE_VISUAL_LOG
DEFINE_STAT(STAT_VisualLog);

TSharedPtr<FJsonValue> GenerateJson(FVisualLogEntry* InEntry)
{
	TSharedPtr<FJsonObject> JsonEntryObject = MakeShareable(new FJsonObject);

	JsonEntryObject->SetNumberField(VisualLogJson::TAG_TIMESTAMP, InEntry->TimeStamp);
	JsonEntryObject->SetStringField(VisualLogJson::TAG_LOCATION, InEntry->Location.ToString());

	const int32 StatusCategoryCount = InEntry->Status.Num();
	const FVisualLogEntry::FStatusCategory* StatusCategory = InEntry->Status.GetData();
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
	const FVisualLogEntry::FLogLine* LogLine = InEntry->LogLines.GetData();
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
	const FVisualLogEntry::FElementToDraw* Element = InEntry->ElementsToDraw.GetData();
	TArray< TSharedPtr<FJsonValue> > JsonElementsToDraw;
	JsonElementsToDraw.AddZeroed(ElementsToDrawCount);

	for (int32 ElementIndex = 0; ElementIndex < ElementsToDrawCount; ++ElementIndex, ++Element)
	{
		TSharedPtr<FJsonObject> JsonElementToDrawObject = MakeShareable(new FJsonObject);

		JsonElementToDrawObject->SetStringField(VisualLogJson::TAG_DESCRIPTION, Element->Description);
		JsonElementToDrawObject->SetStringField(VisualLogJson::TAG_CATEGORY, Element->Category.ToString());
		JsonElementToDrawObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, Element->Verbosity);

		const int32 EncodedTypeColorSize = Element->Type << 24 | Element->Color << 16 | Element->Thicknes;
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
	const FVisualLogEntry::FHistogramSample* Sample = InEntry->HistogramSamples.GetData();
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

		const FVisualLogEntry::FHistogramSample* Sample = InEntry->HistogramSamples.GetData();
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
				FVisualLogEntry::FElementToDraw& Element = VisLogEntry->ElementsToDraw[VisLogEntry->ElementsToDraw.Add(FVisualLogEntry::FElementToDraw())];

				Element.Description = JsonElementObject->GetStringField(VisualLogJson::TAG_DESCRIPTION);
				Element.Category = FName(*(JsonElementObject->GetStringField(VisualLogJson::TAG_CATEGORY)));
				Element.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonElementObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));

				// Element->Type << 24 | Element->Color << 16 | Element->Thicknes;
				int32 EncodedTypeColorSize = 0;
				TTypeFromString<int32>::FromString(EncodedTypeColorSize, *(JsonElementObject->GetStringField(VisualLogJson::TAG_TYPECOLORSIZE)));
				Element.Type = EncodedTypeColorSize >> 24;
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
				FVisualLogEntry::FHistogramSample& Sample = VisLogEntry->HistogramSamples[VisLogEntry->HistogramSamples.Add(FVisualLogEntry::FHistogramSample())];

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
				FVisualLogEntry::FLogEvent& Event = VisLogEntry->Events[VisLogEntry->Events.Add(FVisualLogEntry::FLogEvent())];
				Event.Name = JsonSampleObject->GetStringField(VisualLogJson::TAG_CATEGORY);
				Event.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonSampleObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));
				Event.TagName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_TAGNAME)));
				Event.UserData = (int64)(JsonSampleObject->GetNumberField(VisualLogJson::TAG_USERDATA));
				Event.UserFriendlyDesc = JsonSampleObject->GetStringField(VisualLogJson::TAG_DESCRIPTION);
				Event.Counter = (int64)(JsonSampleObject->GetNumberField(VisualLogJson::TAG_COUNTER));

				TArray< TSharedPtr<FJsonValue> > JsonEventTags = JsonEntryObject->GetArrayField(VisualLogJson::TAG_EVENTAGS);
				if (JsonEventTags.Num() > 0)
				{
					Event.EventTags.Reserve(JsonEventSamples.Num());
					for (int32 TagIndex = 0; TagIndex < JsonEventTags.Num(); ++TagIndex)
					{
						TSharedPtr<FJsonObject> JsonTagObject = JsonEventTags[SampleIndex]->AsObject();
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
				FVisualLogEntry::FDataBlock& Sample = VisLogEntry->DataBlocks[VisLogEntry->DataBlocks.Add(FVisualLogEntry::FDataBlock())];
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
FActorsVisLog::FActorsVisLog(const class UObject* Object, TArray<TWeakObjectPtr<UObject> >* Children)
	: Name(Object->GetFName())
	, FullName(Object->GetFullName())
{
	const class AActor* AsActor = Cast<AActor>(Object);

	Entries.Reserve(VisLogInitialSize);
	if (!AsActor)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(Object);
		check(World);

		Entries.Add(MakeShareable(new FVisualLogEntry(World->GetTimeSeconds(), FVector(), Object, Children)));
	}
	else
	{
		Entries.Add(MakeShareable(new FVisualLogEntry(AsActor, Children)));
	}
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

//----------------------------------------------------------------------//
// FVisualLog
//----------------------------------------------------------------------//
FVisualLog::FVisualLog()
	: FileAr(NULL)
	, StartRecordingTime(0.0f)
	, bIsRecordingOnServer(false)
	, bIsRecordingToFile(false)
	, bIsAllBlocked(false)
{
	Whitelist.Reserve(10);
}

FVisualLog::~FVisualLog()
{
	if (bIsRecordingToFile)
	{
		if (GWorld)
		{
			GWorld->GetTimerManager().ClearTimer(FTimerDelegate::CreateRaw(this, &FVisualLog::DumpRecordedLogs));
		}
		SetIsRecording(false);
	}
}

void FVisualLog::StartRecordingToFile(float TImeStamp)
{
	SetIsRecording(IsRecording(), true);
	if (GWorld)
	{
		GWorld->GetTimerManager().SetTimer(FTimerDelegate::CreateRaw(this, &FVisualLog::DumpRecordedLogs), 4, true);
	}
}

void FVisualLog::StopRecordingToFile(float TImeStamp)
{
	SetIsRecording(IsRecording(), false);
	if (GWorld)
	{
		GWorld->GetTimerManager().ClearTimer(FTimerDelegate::CreateRaw(this, &FVisualLog::DumpRecordedLogs));
	}
}

void FVisualLog::SetFileName(const FString& InFileName)
{
	BaseFileName = InFileName;
}

void FVisualLog::Serialize(const class UObject* LogOwner, const FVisualLogEntry& LogEntry)
{
	TSharedPtr<FActorsVisLog> Log = GetLog(LogOwner);
	Log->Entries.Add(MakeShareable(new FVisualLogEntry(LogEntry)));
}

FString FVisualLog::GetLogFileFullName() const
{
	FString FileName = FString::Printf(TEXT("%s_%.0f-%.0f_%s.vlog"), *BaseFileName, StartRecordingTime, GWorld ? GWorld->TimeSeconds : 0, *FDateTime::Now().ToString());

	const FString FullFilename = FString::Printf(TEXT("%slogs/%s"), *FPaths::GameSavedDir(), *FileName);

	return FullFilename;
}

void FVisualLog::DumpRecordedLogs()
{
	if (!FileAr)
	{
		return;
	}

	TArray<TSharedPtr<FActorsVisLog>> Logs;
	LogsMap.GenerateValueArray(Logs);

	for (int32 ItemIndex = 0; ItemIndex < Logs.Num(); ++ItemIndex)
	{
		if (Logs.IsValidIndex(ItemIndex))
		{
			TSharedPtr<FActorsVisLog> Log = Logs[ItemIndex];

			TSharedPtr<FJsonObject> JsonLogObject = MakeShareable(new FJsonObject);
			JsonLogObject->SetStringField(VisualLogJson::TAG_NAME, Log->Name.ToString());
			JsonLogObject->SetStringField(VisualLogJson::TAG_FULLNAME, Log->FullName);

			TArray< TSharedPtr<FJsonValue> > JsonLogEntries;
			JsonLogEntries.AddZeroed(Log->Entries.Num());
			for (int32 EntryIndex = 0; EntryIndex < Log->Entries.Num(); ++EntryIndex)
			{
				FVisualLogEntry* Entry = Log->Entries[EntryIndex].Get();
				if (Entry)
				{
					JsonLogEntries[EntryIndex] = GenerateJson(Entry);
				}
			}
			JsonLogObject->SetArrayField(VisualLogJson::TAG_ENTRIES, JsonLogEntries);

			UCS2CHAR Char = UCS2CHAR(',');
			FileAr->Serialize(&Char, sizeof(UCS2CHAR));

			TSharedRef<TJsonWriter<UCS2CHAR> > Writer = TJsonWriter<UCS2CHAR>::Create(FileAr);
			FJsonSerializer::Serialize(JsonLogObject.ToSharedRef(), Writer);

			Log->Entries.Reset();
		}
	}
}

void FVisualLog::SetIsRecording(bool NewRecording, bool bRecordToFile)
{
	if (IsRecording() && bIsRecordingToFile && !bRecordToFile)
	{
		if (FileAr)
		{
			// dump remaining logs
			FVisualLogger::Get().Flush();
			DumpRecordedLogs();

			// close JSON data correctly
			const FString HeadetStr = TEXT("]}");
			auto AnsiAdditionalData = StringCast<UCS2CHAR>(*HeadetStr);
			FileAr->Serialize((UCS2CHAR*)AnsiAdditionalData.Get(), HeadetStr.Len() * sizeof(UCS2CHAR));
			FileAr->Close();
			delete FileAr;
			FileAr = NULL;
		}

		Cleanup(true);
		bIsRecordingToFile = false;
	}

	if (IsRecording() && !bIsRecordingToFile && bRecordToFile)
	{
		bIsRecordingToFile = bRecordToFile;
		StartRecordingTime = GWorld ? GWorld->TimeSeconds : 0.0f;

		if (!FileAr)
		{
			FileAr = IFileManager::Get().CreateFileWriter(*GetLogFileFullName());

			const FString HeadetStr = TEXT("{\"Logs\":[{}");
			auto AnsiAdditionalData = StringCast<UCS2CHAR>(*HeadetStr);
			FileAr->Serialize((UCS2CHAR*)AnsiAdditionalData.Get(), HeadetStr.Len() * sizeof(UCS2CHAR));
		}
	}
}

FVisualLogEntry*  FVisualLog::GetEntryToWrite(const class UObject* Object, float Timestamp, const FVector& Location)
{
	const class AActor* RedirectionActor = GetVisualLogRedirection(Object);

	UWorld* World = RedirectionActor && RedirectionActor->GetWorld() ? RedirectionActor->GetWorld() : GEngine->GetWorldFromContextObject(Object);
	ensure(World);

	const float TimeStamp = Timestamp < 0 ? World->TimeSeconds : Timestamp;
	TSharedPtr<FActorsVisLog> Log = GetLog(RedirectionActor ? RedirectionActor : Object);
	const int32 LastIndex = Log->Entries.Num() - 1;
	FVisualLogEntry* Entry = Log->Entries.Num() > 0 ? Log->Entries[LastIndex].Get() : NULL;

	if (Entry == NULL || Entry->TimeStamp < TimeStamp)
	{
		// create new entry
		if (RedirectionActor)
		{
			Entry = Log->Entries[Log->Entries.Add(MakeShareable(new FVisualLogEntry(TimeStamp, Location, RedirectionActor, RedirectsMap.Find(RedirectionActor))))].Get();
		}
		else
		{
			Entry = Log->Entries[Log->Entries.Add(MakeShareable(new FVisualLogEntry(TimeStamp, Location, Object, RedirectsMap.Find(RedirectionActor))))].Get();
		}
	}

	check(Entry);
	return Entry;
}

void FVisualLog::Cleanup(bool bReleaseMemory)
{
	//if (bIsRecording && bIsRecordingToFile)
	//{
	//	// dump collected data if needed
	//	DumpRecordedLogs();
	//}

	if (bReleaseMemory)
	{
		LogsMap.Empty();
	}
	else
	{
		LogsMap.Reset();
	}
}

const class AActor* FVisualLog::GetVisualLogRedirection(const class UObject* Source)
{
	if (!Source)
	{
		return NULL;
	}

	for (auto Iterator = RedirectsMap.CreateConstIterator(); Iterator; ++Iterator)
	{
		if ((*Iterator).Key != NULL && (*Iterator).Value.Num() > 0)
		{
			const TArray<TWeakObjectPtr<UObject>>& Children = (*Iterator).Value;
			if (Children.Find(Source) != INDEX_NONE)
			{
				return (*Iterator).Key;
			}
		}
	}

	return Cast<AActor>(Source);
}

void FVisualLog::RedirectToVisualLog(const class UObject* Src, const class AActor* Dest)
{
	Redirect(const_cast<class UObject*>(Src), Dest);
}

void FVisualLog::Redirect(UObject* Source, const AActor* NewRedirection)
{
	// sanity check
	if (Source == NULL)
	{ 
		return;
	}

	if (NewRedirection != NULL)
	{
		NewRedirection = GetVisualLogRedirection(NewRedirection);
	}
	const AActor* OldRedirect = GetVisualLogRedirection(Source);

	if (NewRedirection == OldRedirect)
	{
		return;
	}
	if (!NewRedirection)
	{
		NewRedirection = Cast<AActor>(Source);
	}
	if (!NewRedirection)
	{
		return;
	}

	if (OldRedirect != NULL)
	{
		TArray<TWeakObjectPtr<UObject> >& OldChildren = RedirectsMap.FindOrAdd(OldRedirect);
		OldChildren.RemoveSingleSwap(Source);
	}

	TArray<TWeakObjectPtr<UObject> >& NewTargetChildren = RedirectsMap.FindOrAdd(NewRedirection);
	NewTargetChildren.AddUnique(Source);

	// now update all actors that have Actor as their VLog redirection
	AActor* SourceAsActor = Cast<AActor>(Source);
	if (SourceAsActor)
	{
		TArray<TWeakObjectPtr<UObject> >* Children = RedirectsMap.Find(SourceAsActor);
		if (Children != NULL)
		{
			TWeakObjectPtr<UObject>* WeakActorPtr = Children->GetData();
			for (int32 Index = 0; Index < Children->Num(); ++Index)
			{
				if (WeakActorPtr->IsValid())
				{
					NewTargetChildren.AddUnique(*WeakActorPtr);
				}
			}
			RedirectsMap.Remove(SourceAsActor);
		}
	}

	UE_CVLOG(OldRedirect != NULL, OldRedirect, LogVisual, Display, TEXT("Binding %s to log %s"), *Source->GetName(), *NewRedirection->GetName());
	UE_CVLOG(NewRedirection != NULL, NewRedirection, LogVisual, Display, TEXT("Binding %s to log %s"), *Source->GetName(), *NewRedirection->GetName());
}

void FVisualLog::LogLine(const UObject* Object, const FName& CategoryName, ELogVerbosity::Type Verbosity, const FString& Line, int64 UserData, FName TagName)
{
	if (IsRecording() == false || Object == NULL || Object->IsPendingKill() || (IsAllBlocked() && !InWhitelist(CategoryName)))
	{
		return;
	}
	
	const class AActor* RedirectionActor = GetVisualLogRedirection(Object);
	UWorld* World = RedirectionActor && RedirectionActor->GetWorld() ? RedirectionActor->GetWorld() : GEngine->GetWorldFromContextObject(Object);
	if (!World)
	{
		return;
	}

	FVisualLogEntry* Entry = GetEntryToWrite(Object);

	if (Entry != NULL)
	{
		// @todo will have to store CategoryName separately, and create a map of names 
		// used in log to have saved logs independent from FNames index changes
		int32 LineIndex = Entry->LogLines.Add(FVisualLogEntry::FLogLine(CategoryName, Verbosity, Line, UserData));
		Entry->LogLines[LineIndex].TagName = TagName;
	}
}
#endif // ENABLE_VISUAL_LOG

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#include "Developer/LogVisualizer/Public/LogVisualizerModule.h"

static class FLogVisualizerExec : private FSelfRegisteringExec
{
public:
	/** Console commands, see embeded usage statement **/
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override
	{
		if (FParse::Command(&Cmd, TEXT("VISLOG")))
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
			else if (Command == TEXT("exit"))
			{
				FLogVisualizerModule::Get()->CloseUI(InWorld);
				return true;
			}
			else
			{
				FLogVisualizerModule::Get()->SummonUI(InWorld);
				return true;
			}
#endif
#else
			UE_LOG(LogVisual, Warning, TEXT("Unable to open LogVisualizer - logs are disabled"));
#endif
		}
		return false;
	}
} LogVisualizerExec;


#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

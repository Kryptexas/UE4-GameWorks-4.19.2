// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"
#include "Json.h"

DEFINE_LOG_CATEGORY(LogVisual);

#if ENABLE_VISUAL_LOG

DEFINE_STAT(STAT_VisualLog);

//----------------------------------------------------------------------//
// FVisLogEntry
//----------------------------------------------------------------------//
FVisLogEntry::FVisLogEntry(const class AActor* InActor, TArray<TWeakObjectPtr<AActor> >* Children)
{
	if (InActor->IsPendingKill() == false)
	{
		TimeStamp = InActor->GetWorld()->TimeSeconds;
		Location = InActor->GetActorLocation();
		InActor->GrabDebugSnapshot(this);
		if (Children != NULL)
		{
			TWeakObjectPtr<AActor>* WeakActorPtr = Children->GetTypedData();
			for (int32 Index = 0; Index < Children->Num(); ++Index, ++WeakActorPtr)
			{
				if (WeakActorPtr->IsValid())
				{
					const AActor* ChildActor = WeakActorPtr->Get();
					ChildActor->GrabDebugSnapshot(this);
				}
			}
		}
	}
}

FVisLogEntry::FVisLogEntry(TSharedPtr<FJsonValue> FromJson)
{
	TSharedPtr<FJsonObject> JsonEntryObject = FromJson->AsObject();
	if (JsonEntryObject.IsValid() == false)
	{
		return;
	}

	TimeStamp = JsonEntryObject->GetNumberField(VisualLogJson::TAG_TIMESTAMP);
	Location.InitFromString(JsonEntryObject->GetStringField(VisualLogJson::TAG_LOCATION));

	TArray< TSharedPtr<FJsonValue> > JsonStatus = JsonEntryObject->GetArrayField(VisualLogJson::TAG_STATUS);
	if (JsonStatus.Num() > 0)
	{
		Status.AddZeroed(JsonStatus.Num());
		for (int32 StatusIndex = 0; StatusIndex < JsonStatus.Num(); ++StatusIndex)
		{
			TSharedPtr<FJsonObject> JsonStatusCategory = JsonStatus[StatusIndex]->AsObject();
			if (JsonStatusCategory.IsValid())
			{
				Status[StatusIndex].Category = JsonStatusCategory->GetStringField(VisualLogJson::TAG_CATEGORY);
				
				TArray< TSharedPtr<FJsonValue> > JsonStatusLines = JsonStatusCategory->GetArrayField(VisualLogJson::TAG_STATUSLINES);
				if (JsonStatusLines.Num() > 0)
				{
					Status[StatusIndex].Data.AddZeroed(JsonStatusLines.Num());					
					for (int32 LineIdx = 0; LineIdx < JsonStatusLines.Num(); LineIdx++)
					{
						Status[StatusIndex].Data.Add(JsonStatusLines[LineIdx]->AsString());
					}
				}
			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JsonLines = JsonEntryObject->GetArrayField(VisualLogJson::TAG_LOGLINES);
	if (JsonLines.Num() > 0)
	{
		LogLines.AddZeroed(JsonLines.Num());
		for (int32 LogLineIndex = 0; LogLineIndex < JsonLines.Num(); ++LogLineIndex)
		{
			TSharedPtr<FJsonObject> JsonLogLine = JsonLines[LogLineIndex]->AsObject();
			if (JsonLogLine.IsValid())
			{
				LogLines[LogLineIndex].Category = FName(*(JsonLogLine->GetStringField(VisualLogJson::TAG_CATEGORY)));
				LogLines[LogLineIndex].Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonLogLine->GetNumberField(VisualLogJson::TAG_VERBOSITY)));
				LogLines[LogLineIndex].Line = JsonLogLine->GetStringField(VisualLogJson::TAG_LINE);
			}
		}
	}

	TArray< TSharedPtr<FJsonValue> > JsonElementsToDraw = JsonEntryObject->GetArrayField(VisualLogJson::TAG_ELEMENTSTODRAW);
	if (JsonElementsToDraw.Num() > 0)
	{
		ElementsToDraw.Reserve(JsonElementsToDraw.Num());
		for (int32 ElementIndex = 0; ElementIndex < JsonElementsToDraw.Num(); ++ElementIndex)
		{
			TSharedPtr<FJsonObject> JsonElementObject = JsonElementsToDraw[ElementIndex]->AsObject();
			if (JsonElementObject.IsValid())
			{
				FElementToDraw& Element = ElementsToDraw[ElementsToDraw.Add(FElementToDraw())];

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
		HistogramSamples.Reserve(JsonHistogramSamples.Num());
		for (int32 SampleIndex = 0; SampleIndex < JsonHistogramSamples.Num(); ++SampleIndex)
		{
			TSharedPtr<FJsonObject> JsonSampleObject = JsonHistogramSamples[SampleIndex]->AsObject();
			if (JsonSampleObject.IsValid())
			{
				FHistogramSample& Sample = HistogramSamples[HistogramSamples.Add(FHistogramSample())];

				Sample.Category = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_CATEGORY)));
				Sample.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonSampleObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));
				Sample.GraphName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_HISTOGRAMGRAPHNAME)));
				Sample.DataName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_HISTOGRAMDATANAME)));
				Sample.SampleValue.InitFromString(JsonSampleObject->GetStringField(VisualLogJson::TAG_HISTOGRAMSAMPLE));
			}
		}
	}

}

TSharedPtr<FJsonValue> FVisLogEntry::ToJson() const
{
	TSharedPtr<FJsonObject> JsonEntryObject = MakeShareable(new FJsonObject);

	JsonEntryObject->SetNumberField(VisualLogJson::TAG_TIMESTAMP, TimeStamp);
	JsonEntryObject->SetStringField(VisualLogJson::TAG_LOCATION, Location.ToString());

	const int32 StatusCategoryCount = Status.Num();
	const FVisLogEntry::FStatusCategory* StatusCategory = Status.GetTypedData();
	TArray< TSharedPtr<FJsonValue> > JsonStatus;
	JsonStatus.AddZeroed(StatusCategoryCount);

	for (int32 StatusIdx = 0; StatusIdx < Status.Num(); StatusIdx++, StatusCategory++)
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

	const int32 LogLineCount = LogLines.Num();
	const FVisLogEntry::FLogLine* LogLine = LogLines.GetTypedData();
	TArray< TSharedPtr<FJsonValue> > JsonLines;
	JsonLines.AddZeroed(LogLineCount);

	for (int32 LogLineIndex = 0; LogLineIndex < LogLineCount; ++LogLineIndex, ++LogLine)
	{
		TSharedPtr<FJsonObject> JsonLogLineObject = MakeShareable(new FJsonObject);
		JsonLogLineObject->SetStringField(VisualLogJson::TAG_CATEGORY, LogLine->Category.ToString());
		JsonLogLineObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, LogLine->Verbosity);
		JsonLogLineObject->SetStringField(VisualLogJson::TAG_LINE, LogLine->Line);
		JsonLines[LogLineIndex] = MakeShareable(new FJsonValueObject(JsonLogLineObject));
	}

	JsonEntryObject->SetArrayField(VisualLogJson::TAG_LOGLINES, JsonLines);

	const int32 ElementsToDrawCount = ElementsToDraw.Num();
	const FVisLogEntry::FElementToDraw* Element = ElementsToDraw.GetTypedData();
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
		const FVector* PointToDraw = Element->Points.GetTypedData();
		for (int32 PointIndex = 0; PointIndex < PointsCount; ++PointIndex, ++PointToDraw)
		{
			JsonStringPoints[PointIndex] = MakeShareable(new FJsonValueString(PointToDraw->ToString()));
		}
		JsonElementToDrawObject->SetArrayField(VisualLogJson::TAG_POINTS, JsonStringPoints);
		
		JsonElementsToDraw[ElementIndex] = MakeShareable(new FJsonValueObject(JsonElementToDrawObject));
	}

	JsonEntryObject->SetArrayField(VisualLogJson::TAG_ELEMENTSTODRAW, JsonElementsToDraw);
	
	const int32 HistogramSamplesCount = HistogramSamples.Num();
	const FVisLogEntry::FHistogramSample* Sample = HistogramSamples.GetTypedData();
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
	

	return MakeShareable(new FJsonValueObject(JsonEntryObject));
}

void FVisLogEntry::AddElement(const TArray<FVector>& Points, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points = Points;
	Element.Type = FElementToDraw::Path;
	ElementsToDraw.Add(Element);
}

void FVisLogEntry::AddElement(const FVector& Point, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Add(Point);
	Element.Type = FElementToDraw::SinglePoint;
	ElementsToDraw.Add(Element);
}

void FVisLogEntry::AddElement(const FVector& Start, const FVector& End, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Reserve(2);
	Element.Points.Add(Start);
	Element.Points.Add(End);
	Element.Type = FElementToDraw::Segment;
	ElementsToDraw.Add(Element);
}

void FVisLogEntry::AddElement(const FBox& Box, const FName& CategoryName, const FColor& Color, const FString& Description, uint16 Thickness)
{
	FElementToDraw Element(Description, Color, Thickness, CategoryName);
	Element.Points.Reserve(2);
	Element.Points.Add(Box.Min);
	Element.Points.Add(Box.Max);
	Element.Type = FElementToDraw::Box;
	ElementsToDraw.Add(Element);
}

void FVisLogEntry::AddHistogramData(const FVector2D& DataSample, const FName& CategoryName, const FName& GraphName, const FName& DataName)
{
	FHistogramSample Sample;
	Sample.Category = CategoryName;
	Sample.GraphName = GraphName;
	Sample.DataName = DataName;
	Sample.SampleValue = DataSample;

	HistogramSamples.Add(Sample);
}

//----------------------------------------------------------------------//
// FActorsVisLog
//----------------------------------------------------------------------//
FActorsVisLog::FActorsVisLog(const class AActor* Actor, TArray<TWeakObjectPtr<AActor> >* Children)
	: Name(Actor->GetFName())
	, FullName(Actor->GetFullName())
{
	Entries.Reserve(VisLogInitialSize);
	Entries.Add(MakeShareable(new FVisLogEntry(Actor, Children)));
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
			Entries[EntryIndex] = MakeShareable(new FVisLogEntry(JsonEntries[EntryIndex]));
		}
	}
}

TSharedPtr<FJsonValue> FActorsVisLog::ToJson() const
{
	TSharedPtr<FJsonObject> JsonLogObject = MakeShareable(new FJsonObject);

	JsonLogObject->SetStringField(VisualLogJson::TAG_NAME, Name.ToString());
	JsonLogObject->SetStringField(VisualLogJson::TAG_FULLNAME, FullName);

	const TSharedPtr<FVisLogEntry>* Entry = Entries.GetTypedData();
	const int32 EntryCount = Entries.Num();
	TArray< TSharedPtr<FJsonValue> > JsonLogEntries;
	JsonLogEntries.AddZeroed(EntryCount);

	for (int32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex, ++Entry)
	{
		JsonLogEntries[EntryIndex] = (*Entry)->ToJson(); 
	}

	JsonLogObject->SetArrayField(VisualLogJson::TAG_ENTRIES, JsonLogEntries);

	return MakeShareable(new FJsonValueObject(JsonLogObject));
}
//----------------------------------------------------------------------//
// FVisualLog
//----------------------------------------------------------------------//
FVisualLog::FVisualLog()
: bIsRecording(GEngine->bEnableVisualLogRecordingOnStart), bIsRecordingOnServer(false)
{
	FileAr = NULL;
	bIsRecordingToFile = false;
	bIsAllBlocked = false;
	Whitelist.Reserve(10);
}

FVisualLog::~FVisualLog()
{
	if (bIsRecording)
	{
		SetIsRecording(false);
	}
}

void FVisualLog::DumpRecordedLogs()
{
	if (!FileAr)
	{
		TempFileName = FString::Printf(TEXT("VisualLog_TEMP_%s.vlog"), *FDateTime::Now().ToString());
		const FString TempFullFilename = FString::Printf(TEXT("%s/logs/%s"), *FPaths::GameSavedDir(), *TempFileName);
		FileAr = IFileManager::Get().CreateFileWriter(*TempFullFilename);

		const FString HeadetStr = TEXT("{\"Logs\":[{}");
		auto AnsiAdditionalData = StringCast<UCS2CHAR>(*HeadetStr);
		FileAr->Serialize((UCS2CHAR*)AnsiAdditionalData.Get(), HeadetStr.Len() * sizeof(UCS2CHAR));
	}

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
				FVisLogEntry* Entry = Log->Entries[EntryIndex].Get();
				if (Entry)
				{
					JsonLogEntries[EntryIndex] = Entry->ToJson();
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

void FVisualLog::Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category )
{

}

void FVisualLog::SetIsRecording(bool NewRecording, bool bRecordToFile)
{
	if (bIsRecording && bIsRecordingToFile && !NewRecording)
	{
		if (FileAr)
		{
			// dump remaining logs
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

		const FString TempFullFilename = FString::Printf(TEXT("%s/logs/%s"), *FPaths::GameSavedDir(), *TempFileName);
		const FString FileName = FString::Printf(TEXT("VisualLog_%.0f-%.0f_%s.vlog"), StartRecordingTime, GWorld ? GWorld->TimeSeconds : 0, *FDateTime::Now().ToString());
		FString FullFilename = FString::Printf(TEXT("%s/logs/%s"), *FPaths::GameSavedDir(), *FileName);
		
		IFileManager::Get().Move(*FullFilename, *TempFullFilename, true, true);
	}

	bIsRecording = NewRecording;
	if (bIsRecording)
	{
		bIsRecordingToFile = bRecordToFile;
		StartRecordingTime = GWorld ? GWorld->TimeSeconds : 0;
	}
}

FVisLogEntry*  FVisualLog::GetEntryToWrite(const class AActor* Actor)
{
	check(Actor && Actor->GetWorld() && Actor->GetVisualLogRedirection());
	const class AActor* LogOwner = Actor->GetVisualLogRedirection();
	const float TimeStamp = Actor->GetWorld()->TimeSeconds;
	TSharedPtr<FActorsVisLog> Log = GetLog(LogOwner);
	const int32 LastIndex = Log->Entries.Num() - 1;
	FVisLogEntry* Entry = Log->Entries.Num() > 0 ? Log->Entries[LastIndex].Get() : NULL;

	if (Entry == NULL || Entry->TimeStamp < TimeStamp)
	{
		// create new entry
		Entry = Log->Entries[Log->Entries.Add( MakeShareable(new FVisLogEntry(LogOwner, RedirectsMap.Find(LogOwner))) )].Get();
	}

	check(Entry);
	return Entry;
}

void FVisualLog::Cleanup(bool bReleaseMemory)
{
	if (bReleaseMemory)
	{
		LogsMap.Empty();
		RedirectsMap.Empty();
	}
	else
	{
		LogsMap.Reset();
		RedirectsMap.Reset();
	}
}

void FVisualLog::Redirect(AActor* Actor, const AActor* NewRedirection)
{
	// sanity check
	if (Actor == NULL)
	{ 
		return;
	}

	if (NewRedirection != NULL)
	{
		NewRedirection = NewRedirection->GetVisualLogRedirection();
	}
	const AActor* OldRedirect = Actor->GetVisualLogRedirection();

	if (NewRedirection == OldRedirect)
	{
		return;
	}
	if (NewRedirection == NULL)
	{
		NewRedirection = Actor;
	}

	// this should log to OldRedirect
	UE_VLOG(Actor, LogVisual, Display, TEXT("Binding %s to log %s"), *Actor->GetName(), *NewRedirection->GetName());

	TArray<TWeakObjectPtr<AActor> >& NewTargetChildren = RedirectsMap.FindOrAdd(NewRedirection);

	Actor->SetVisualLogRedirection(NewRedirection);
	NewTargetChildren.AddUnique(Actor);

	// now update all actors that have Actor as their VLog redirection
	TArray<TWeakObjectPtr<AActor> >* Children = RedirectsMap.Find(Actor);
	if (Children != NULL)
	{
		TWeakObjectPtr<AActor>* WeakActorPtr = Children->GetTypedData();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (WeakActorPtr->IsValid())
			{
				WeakActorPtr->Get()->SetVisualLogRedirection(NewRedirection);
				NewTargetChildren.AddUnique(*WeakActorPtr);
			}
		}
		RedirectsMap.Remove(Actor);
	}
}

void FVisualLog::LogLine(const AActor* Actor, const FName& CategoryName, ELogVerbosity::Type Verbosity, const FString& Line)
{
	if (IsRecording() == false || Actor == NULL || Actor->IsPendingKill() || (IsAllBlocked() && !InWhitelist(CategoryName)))
	{
		return;
	}
	
	FVisLogEntry* Entry = GetEntryToWrite(Actor);

	if (Entry != NULL)
	{
		// @todo will have to store CategoryName separately, and create a map of names 
		// used in log to have saved logs independent from FNames index changes
		Entry->LogLines.Add(FVisLogEntry::FLogLine(CategoryName, Verbosity, Line));
	}
}

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
			FString Command = FParse::Token(Cmd, 0);
			if (Command == TEXT("record"))
			{
				FVisualLog::Get()->SetIsRecording(true);
				return true;
			}
			else if (Command == TEXT("stop"))
			{
				FVisualLog::Get()->SetIsRecording(false);
				return true;
			}
			else if (Command == TEXT("exit"))
			{
				FLogVisualizerModule::Get()->CloseUI(InWorld);
				return true;
			}
			else if (Command == TEXT("disableallbut"))
			{
				FString Category = FParse::Token(Cmd, 1);
				FVisualLog* VisLog = FVisualLog::Get();
				if (VisLog)
				{
					VisLog->BlockAllLogs(true);
					VisLog->AddCategortyToWhiteList(*Category);
				}
				return true;
			}
			else
			{
				FLogVisualizerModule::Get()->SummonUI(InWorld);
				return true;
			}
		}
		return false;
	}
} LogVisualizerExec;

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#endif // ENABLE_VISUAL_LOG
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"
#include "Json.h"

DEFINE_LOG_CATEGORY(LogVisual);

#if ENABLE_VISUAL_LOG

DEFINE_STAT(STAT_VisualLog);

/** The table used to encode a 6 bit value as an ascii character */
uint8 EncodingAlphabet[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e',
'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

/** The table used to convert an ascii character into a 6 bit value */
uint8 DecodingAlphabet[256] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22,
0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

FString EncodeToBase64(uint8* Source, uint32 Length)
{
	// Each 3 uint8 set of data expands to 4 bytes and must be padded to 4 bytes
	uint32 ExpectedLength = (Length + 2) / 3 * 4;
	FString OutBuffer;
	OutBuffer.Empty(ExpectedLength);

	ANSICHAR EncodedBytes[5];
	// Null terminate this so we can append it easily as a string
	EncodedBytes[4] = 0;
	// Loop through the buffer converting 3 bytes of binary data at a time
	while (Length >= 3)
	{
		uint8 A = *Source++;
		uint8 B = *Source++;
		uint8 C = *Source++;
		Length -= 3;
		// The algorithm takes 24 bits of data (3 bytes) and breaks it into 4 6bit chunks represented as ascii
		uint32 ByteTriplet = A << 16 | B << 8 | C;
		// Use the 6bit block to find the representation ascii character for it
		EncodedBytes[3] = EncodingAlphabet[ByteTriplet & 0x3F];
		// Shift each time to get the next 6 bit chunk
		ByteTriplet >>= 6;
		EncodedBytes[2] = EncodingAlphabet[ByteTriplet & 0x3F];
		ByteTriplet >>= 6;
		EncodedBytes[1] = EncodingAlphabet[ByteTriplet & 0x3F];
		ByteTriplet >>= 6;
		EncodedBytes[0] = EncodingAlphabet[ByteTriplet & 0x3F];
		// Now we can append this buffer to our destination string
		OutBuffer += EncodedBytes;
	}
	// Since this algorithm operates on blocks, we may need to pad the last chunks
	if (Length > 0)
	{
		uint8 A = *Source++;
		uint8 B = 0;
		uint8 C = 0;
		// Grab the second character if it is a 2 uint8 finish
		if (Length == 2)
		{
			B = *Source;
		}
		uint32 ByteTriplet = A << 16 | B << 8 | C;
		// Pad with = to make a 4 uint8 chunk
		EncodedBytes[3] = '=';
		ByteTriplet >>= 6;
		// If there's only one 1 uint8 left in the source, then you need 2 pad chars
		if (Length == 1)
		{
			EncodedBytes[2] = '=';
		}
		else
		{
			EncodedBytes[2] = EncodingAlphabet[ByteTriplet & 0x3F];
		}
		// Now encode the remaining bits the same way
		ByteTriplet >>= 6;
		EncodedBytes[1] = EncodingAlphabet[ByteTriplet & 0x3F];
		ByteTriplet >>= 6;
		EncodedBytes[0] = EncodingAlphabet[ByteTriplet & 0x3F];
		OutBuffer += EncodedBytes;
	}
	return OutBuffer;
}

bool DecodeFromBase64(const ANSICHAR* Source, uint32 Length, uint8* Dest, uint32& PadCount)
{
	PadCount = 0;
	uint8 DecodedValues[4];
	while (Length)
	{
		// Decode the next 4 BYTEs
		for (int32 Index = 0; Index < 4; Index++)
		{
			// Tell the caller if there were any pad bytes
			if (*Source == '=')
			{
				PadCount++;
			}
			DecodedValues[Index] = DecodingAlphabet[(int32)(*Source++)];
			// Abort on values that we don't understand
			if (DecodedValues[Index] == -1)
			{
				return false;
			}
		}
		Length -= 4;
		// Rebuild the original 3 bytes from the 4 chunks of 6 bits
		uint32 OriginalTriplet = DecodedValues[0];
		OriginalTriplet <<= 6;
		OriginalTriplet |= DecodedValues[1];
		OriginalTriplet <<= 6;
		OriginalTriplet |= DecodedValues[2];
		OriginalTriplet <<= 6;
		OriginalTriplet |= DecodedValues[3];
		// Now we can tear the uint32 into bytes
		Dest[2] = OriginalTriplet & 0xFF;
		OriginalTriplet >>= 8;
		Dest[1] = OriginalTriplet & 0xFF;
		OriginalTriplet >>= 8;
		Dest[0] = OriginalTriplet & 0xFF;
		Dest += 3;
	}
	return true;
}

bool DecodeFromBase64(const FString& Source, TArray<uint8>& Dest)
{
	uint32 Length = Source.Len();
	// Size must be a multiple of 4
	if (Length % 4)
	{
		return false;
	}
	// Each 4 uint8 chunk of characters is 3 bytes of data
	uint32 ExpectedLength = Length / 4 * 3;
	// Add the number we need for output
	Dest.AddZeroed(ExpectedLength);
	uint8* Buffer = Dest.GetTypedData();
	uint32 PadCount = 0;
	bool bWasSuccessful = DecodeFromBase64(TCHAR_TO_ANSI(*Source), Length, Dest.GetTypedData(), PadCount);
	if (bWasSuccessful)
	{
		if (PadCount > 0)
		{
			Dest.RemoveAt(ExpectedLength - PadCount, PadCount);
		}
	}
	return bWasSuccessful;
}

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

	TArray< TSharedPtr<FJsonValue> > JasonDataBlocks = JsonEntryObject->GetArrayField(VisualLogJson::TAG_DATABLOCK);
	if (JasonDataBlocks.Num() > 0)
	{
		DataBlocks.Reserve(JasonDataBlocks.Num());
		for (int32 SampleIndex = 0; SampleIndex < JasonDataBlocks.Num(); ++SampleIndex)
		{
			TSharedPtr<FJsonObject> JsonSampleObject = JasonDataBlocks[SampleIndex]->AsObject();
			if (JsonSampleObject.IsValid())
			{
				FDataBlock& Sample = DataBlocks[DataBlocks.Add(FDataBlock())];
				Sample.TagName = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_DATABLOCK_TAGNAME)));
				Sample.Category = FName(*(JsonSampleObject->GetStringField(VisualLogJson::TAG_CATEGORY)));
				Sample.Verbosity = TEnumAsByte<ELogVerbosity::Type>((uint8)FMath::TruncToInt(JsonSampleObject->GetNumberField(VisualLogJson::TAG_VERBOSITY)));

				// decode data from Base64 string
				const FString DataBlockAsCompressedString = JsonSampleObject->GetStringField(VisualLogJson::TAG_DATABLOCK_DATA);
				TArray<uint8> CompressedDataBlock;
				DecodeFromBase64(DataBlockAsCompressedString, CompressedDataBlock);

				// uncompress decoded data to get final TArray<uint8>
				int32 UncompressedSize = 0;
				const int32 HeaderSize = sizeof(int32);
				uint8* SrcBuffer = (uint8*)CompressedDataBlock.GetTypedData();
				FMemory::Memcpy(&UncompressedSize, SrcBuffer, HeaderSize);
				SrcBuffer += HeaderSize;
				const int32 CompressedSize = CompressedDataBlock.Num() - HeaderSize;

				Sample.Data.AddZeroed(UncompressedSize);
				FCompression::UncompressMemory((ECompressionFlags)(COMPRESS_ZLIB), (void*)Sample.Data.GetTypedData(), UncompressedSize, SrcBuffer, CompressedSize);
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
	
	int32 DataBlockIndex = 0;
	TArray<TSharedPtr<FJsonValue> > JasonDataBlocks;
	JasonDataBlocks.AddZeroed(DataBlocks.Num());
	for (const auto CurrentData : DataBlocks)
	{
		TSharedPtr<FJsonObject> JsonSampleObject = MakeShareable(new FJsonObject);

		TArray<uint8> CompressedData;
		const int32 UncompressedSize = CurrentData.Data.Num();
		const int32 HeaderSize = sizeof(int32);
		CompressedData.Init(0, HeaderSize + FMath::TruncToInt(1.1f * UncompressedSize));

		int32 CompressedSize = CompressedData.Num() - HeaderSize;
		uint8* DestBuffer = CompressedData.GetTypedData();
		FMemory::Memcpy(DestBuffer, &UncompressedSize, HeaderSize);
		DestBuffer += HeaderSize;

		FCompression::CompressMemory((ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory), (void*)DestBuffer, CompressedSize, (void*)CurrentData.Data.GetData(), UncompressedSize);
		CompressedData.SetNum(CompressedSize + HeaderSize, true);
		const FString CurrentDataAsString = EncodeToBase64((uint8*)CompressedData.GetTypedData(), CompressedData.Num());

		JsonSampleObject->SetStringField(VisualLogJson::TAG_CATEGORY, CurrentData.Category.ToString());
		JsonSampleObject->SetStringField(VisualLogJson::TAG_DATABLOCK_TAGNAME, CurrentData.TagName.ToString());
		JsonSampleObject->SetStringField(VisualLogJson::TAG_DATABLOCK_DATA, CurrentDataAsString);
		JsonSampleObject->SetNumberField(VisualLogJson::TAG_VERBOSITY, CurrentData.Verbosity);

		JasonDataBlocks[DataBlockIndex++] = MakeShareable(new FJsonValueObject(JsonSampleObject));
	}
	JsonEntryObject->SetArrayField(VisualLogJson::TAG_DATABLOCK, JasonDataBlocks);

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

void FVisLogEntry::AddDataBlock(const FString& TagName, const TArray<uint8>& BlobDataArray, const FName& CategoryName)
{
	FDataBlock DataBlock;
	DataBlock.Category = CategoryName;
	DataBlock.TagName = *TagName;
	DataBlock.Data = BlobDataArray;

	DataBlocks.Add(DataBlock);
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
	: FileAr(NULL)
	, StartRecordingTime(0.0f)
	, bIsRecording(GEngine ? GEngine->bEnableVisualLogRecordingOnStart : false)
	, bIsRecordingOnServer(false)
	, bIsRecordingToFile(false)
	, bIsAllBlocked(false)
{
	Whitelist.Reserve(10);
}

FVisualLog::~FVisualLog()
{
	if (bIsRecording)
	{
		SetIsRecording(false);
	}
}

FString FVisualLog::GetLogFileFullName() const
{
	FString NameCore(TEXT(""));
	if (LogFileNameGetter.IsBound())
	{
		NameCore = LogFileNameGetter.Execute();
	}
	if (NameCore.IsEmpty())
	{
		NameCore = TEXT("VisualLog");
	}

	FString FileName = FString::Printf(TEXT("%s_%.0f-%.0f_%s.vlog"), *NameCore, StartRecordingTime, GWorld ? GWorld->TimeSeconds : 0, *FDateTime::Now().ToString());

	const FString FullFilename = FString::Printf(TEXT("%slogs/%s"), *FPaths::GameSavedDir(), *FileName);

	return FullFilename;
}

void FVisualLog::DumpRecordedLogs()
{
	if (!FileAr)
	{		
		FileAr = IFileManager::Get().CreateFileWriter(*GetLogFileFullName());

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
	}

	bIsRecording = NewRecording;
	if (bIsRecording)
	{
		bIsRecordingToFile = bRecordToFile;
		StartRecordingTime = GWorld ? GWorld->TimeSeconds : 0.0f;
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
				FVisualLog::Get().SetIsRecording(true);
				return true;
			}
			else if (Command == TEXT("stop"))
			{
				FVisualLog::Get().SetIsRecording(false);
				return true;
			}
			else if (Command == TEXT("disableallbut"))
			{
				FString Category = FParse::Token(Cmd, 1);
				FVisualLog::Get().BlockAllLogs(true);
				FVisualLog::Get().AddCategortyToWhiteList(*Category);
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


#endif // ENABLE_VISUAL_LOG
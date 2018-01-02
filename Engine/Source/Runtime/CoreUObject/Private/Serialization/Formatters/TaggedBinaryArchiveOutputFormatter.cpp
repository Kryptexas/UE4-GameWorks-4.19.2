// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "TaggedBinaryArchiveOutputFormatter.h"

#if WITH_TEXT_ARCHIVE_SUPPORT

FTaggedBinaryArchiveOutputFormatter::FTaggedBinaryArchiveOutputFormatter(FArchive& InInner, TFunction<void (FArchive&,UObject*&)> InSerializeObject)
	: Inner(InInner)
	, SerializeObject(InSerializeObject)
	, StartOffset(Inner.Tell())
	, NextRecordIdx(0)
{
	Inner.ArIsTextFormat = true;

	int64 NamesOffset = 0;
	Inner << NamesOffset;
}

FTaggedBinaryArchiveOutputFormatter::~FTaggedBinaryArchiveOutputFormatter()
{
	int64 NamesOffset = Inner.Tell();

	// Write all the names
	int32 NumNames = Names.Num();
	Inner << NumNames;

	for (FString& Name : Names)
	{
		Inner << Name;
	}

	// Write all the records
	int32 NumRecords = Records.Num();
	Inner << NumRecords;

	for (FRecord& Record : Records)
	{
		int32 NumFields = Record.Fields.Num();
		Inner << NumFields;
		Inner << Record.StartOffset;

		for (FField& Field : Record.Fields)
		{
			Inner << Field.NameIdx;
			WriteSize(Field.Size);
		}
	}

	// Seek back to the start of the file and write the offset to the data
	int32 EndOffset = Inner.Tell();
	Inner.Seek(StartOffset);
	Inner << NamesOffset;
	Inner.Seek(EndOffset);
}

FArchive& FTaggedBinaryArchiveOutputFormatter::GetUnderlyingArchive()
{
	return Inner;
}

void FTaggedBinaryArchiveOutputFormatter::EnterRecord()
{
	WriteType(EArchiveValueType::Record);

	verify(Records.Emplace() == NextRecordIdx);
	Records[NextRecordIdx].StartOffset = Inner.Tell();

	RecordStack.Push(NextRecordIdx++);
}

void FTaggedBinaryArchiveOutputFormatter::LeaveRecord()
{
	FRecord& Record = Records[RecordStack.Top()];
	Record.EndOffset = Inner.Tell();

	RecordStack.Pop();
}

void FTaggedBinaryArchiveOutputFormatter::EnterField(FArchiveFieldName Name)
{
	FRecord& Record = Records[RecordStack.Top()];

	FField& Field = Record.Fields.Emplace_GetRef();
	Field.NameIdx = FindOrAddName(Name.Name);
	Field.Offset = Inner.Tell();
}

void FTaggedBinaryArchiveOutputFormatter::LeaveField()
{
	FField& Field = Records[RecordStack.Top()].Fields.Top();
	Field.Size = Inner.Tell() - Field.Offset;
}

bool FTaggedBinaryArchiveOutputFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving)
{
	if (bEnterWhenSaving)
	{
		EnterField(Name);
	}
	return bEnterWhenSaving;
}

void FTaggedBinaryArchiveOutputFormatter::EnterArray(int32& NumElements)
{
	WriteType(EArchiveValueType::Array);
	Inner << NumElements;
}

void FTaggedBinaryArchiveOutputFormatter::LeaveArray()
{
}

void FTaggedBinaryArchiveOutputFormatter::EnterArrayElement()
{
}

void FTaggedBinaryArchiveOutputFormatter::LeaveArrayElement()
{
}

void FTaggedBinaryArchiveOutputFormatter::EnterStream()
{
	WriteType(EArchiveValueType::Stream);

	FStream& Stream = Streams.Emplace_GetRef();
	Stream.StartOffset = Inner.Tell();
	Stream.NumItems = 0;

	int32 Length = 0;
	Inner << Length;
}

void FTaggedBinaryArchiveOutputFormatter::LeaveStream()
{
	FStream Stream = Streams.Pop();

	int64 EndOffset = Inner.Tell();
	Inner.Seek(Stream.StartOffset);
	Inner << Stream.NumItems;

	Inner.Seek(EndOffset);
}

void FTaggedBinaryArchiveOutputFormatter::EnterStreamElement()
{
	Streams.Top().NumItems++;
}

void FTaggedBinaryArchiveOutputFormatter::LeaveStreamElement()
{
}

void FTaggedBinaryArchiveOutputFormatter::EnterMap(int32& NumElements)
{
	WriteType(EArchiveValueType::Map);
	Inner << NumElements;
}

void FTaggedBinaryArchiveOutputFormatter::LeaveMap()
{
}

void FTaggedBinaryArchiveOutputFormatter::EnterMapElement(FString& Name)
{
	Inner << Name;
}

void FTaggedBinaryArchiveOutputFormatter::LeaveMapElement()
{
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(uint8& Value)
{
	WriteType(EArchiveValueType::UInt8);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(uint16& Value)
{
	WriteType(EArchiveValueType::UInt16);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(uint32& Value)
{
	WriteType(EArchiveValueType::UInt32);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(uint64& Value)
{
	WriteType(EArchiveValueType::UInt64);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(int8& Value)
{
	WriteType(EArchiveValueType::Int8);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(int16& Value)
{
	WriteType(EArchiveValueType::Int16);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(int32& Value)
{
	WriteType(EArchiveValueType::Int32);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(int64& Value)
{
	WriteType(EArchiveValueType::Int64);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(float& Value)
{
	WriteType(EArchiveValueType::Float);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(double& Value)
{
	WriteType(EArchiveValueType::Double);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(bool& Value)
{
	WriteType(EArchiveValueType::Bool);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(FString& Value)
{
	WriteType(EArchiveValueType::String);
	Inner << Value;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(FName& Value)
{
	WriteType(EArchiveValueType::Name);

	FString ValueString = Value.ToString();
	Inner << ValueString;
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(UObject*& Value)
{
	WriteType(EArchiveValueType::Object);
	SerializeObject(Inner, Value);
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(TArray<uint8>& Value) 
{
	WriteType(EArchiveValueType::RawData);

	int64 DataSize = Value.Num();
	Inner << DataSize;

	Inner.Serialize(Value.GetData(), DataSize);
}

void FTaggedBinaryArchiveOutputFormatter::Serialize(void* Data, uint64 DataSize)
{
	WriteType(EArchiveValueType::RawData);

	int64 ActualDataSize = DataSize;
	Inner << ActualDataSize;
	check(ActualDataSize == DataSize);

	Inner.Serialize(Data, DataSize);
}

void FTaggedBinaryArchiveOutputFormatter::WriteType(EArchiveValueType Type)
{
	uint8 TypeValue = (uint8)Type;
	Inner << TypeValue;
}

void FTaggedBinaryArchiveOutputFormatter::WriteSize(uint64 Size)
{
	if (Size < 253)
	{
		uint8 FirstByte = Size;
		Inner << FirstByte;
	}
	else if ((uint16)Size == Size)
	{
		uint8 FirstByte = 253;
		Inner << FirstByte;

		uint16 Size16 = Size;
		Inner << Size16;
	}
	else if ((uint32)Size == Size)
	{
		uint8 FirstByte = 254;
		Inner << FirstByte;

		uint16 Size32 = Size;
		Inner << Size32;
	}
	else
	{
		uint8 FirstByte = 255;
		Inner << FirstByte;
		Inner << Size;
	}
}

int32 FTaggedBinaryArchiveOutputFormatter::FindOrAddName(FString Name)
{
	int32* NameIdxPtr = NameToIndex.Find(Name);
	if (NameIdxPtr == nullptr)
	{
		NameIdxPtr = &(NameToIndex.Add(Name));
		*NameIdxPtr = Names.Add(MoveTemp(Name));
	}
	return *NameIdxPtr;
}

#endif

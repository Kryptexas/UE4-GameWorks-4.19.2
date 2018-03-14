// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "TaggedBinaryArchiveInputFormatter.h"

#if WITH_TEXT_ARCHIVE_SUPPORT

FTaggedBinaryArchiveInputFormatter::FTaggedBinaryArchiveInputFormatter(FArchive& InInner, TFunction<void (FArchive&,UObject*&)> InSerializeObject)
	: Inner(InInner)
	, SerializeObject(InSerializeObject)
	, NextRecordIdx(0)
	, CurrentType(EArchiveValueType::None)
{
	Inner.ArIsTextFormat = true;

	int64 StartOffset = Inner.Tell();

	int64 NamesOffset = 0;
	Inner << NamesOffset;
	Inner.Seek(NamesOffset);

	// Read all the names in this archive
	int32 NumNames = 0;
	Inner << NumNames;

	Names.Reserve(NumNames);
	for (int32 Idx = 0; Idx < NumNames; Idx++)
	{
		FString Name;
		Inner << Name;

		int32 NameIdx = Names.Add(Name);
		NameToIndex.Add(MoveTemp(Name)) = NameIdx;
	}

	// Read all the records in the archive
	int32 NumRecords = 0;
	Inner << NumRecords;

	Records.Reserve(NumRecords);
	for (int32 Idx = 0; Idx < NumRecords; Idx++)
	{
		int32 NumFields = 0;
		Inner << NumFields;

		FRecord& Record = Records.Emplace_GetRef();
		Record.Fields.Reserve(NumFields);

		Inner << Record.StartOffset;
		Record.EndOffset = Record.StartOffset;

		for (int32 FieldIdx = 0; FieldIdx < NumFields; FieldIdx++)
		{
			FField& Field = Record.Fields.Emplace_GetRef();

			Inner << Field.NameIdx;
			Field.Size = ReadSize();

			Field.Offset = Record.EndOffset;
			Record.EndOffset += Field.Size;
		}
	}

	// Seek back to the start of the file
	Inner.Seek(StartOffset + sizeof(NamesOffset));
}

FTaggedBinaryArchiveInputFormatter::~FTaggedBinaryArchiveInputFormatter()
{
}

FArchive& FTaggedBinaryArchiveInputFormatter::GetUnderlyingArchive()
{
	return Inner;
}

void FTaggedBinaryArchiveInputFormatter::EnterRecord()
{
	ExpectType(EArchiveValueType::Record);
	RecordStack.Push(NextRecordIdx++);
}

void FTaggedBinaryArchiveInputFormatter::EnterRecord(TArray<FString>& OutKeys)
{
	EnterRecord();

	FRecord& Record = Records[RecordStack.Top()];

	OutKeys.Reset();
	for (const FField& Field : Record.Fields)
	{
		OutKeys.Add(Names[Field.NameIdx]);
	}
}

void FTaggedBinaryArchiveInputFormatter::LeaveRecord()
{
	FRecord& Record = Records[RecordStack.Top()];
	Inner.Seek(Record.EndOffset);

	RecordStack.Pop();
}

void FTaggedBinaryArchiveInputFormatter::EnterField(FArchiveFieldName Name)
{
	FRecord& Record = Records[RecordStack.Top()];

	int32 NameIdx = NameToIndex.FindChecked(Name.Name);
	const FField* Field = Record.Fields.FindByPredicate([NameIdx](const FField& TestField) -> bool { return TestField.NameIdx == NameIdx; });
	Inner.Seek(Field->Offset);
}

void FTaggedBinaryArchiveInputFormatter::EnterField(FArchiveFieldName Name, EArchiveValueType& OutType)
{
	EnterField(Name);
	OutType = PeekType();
}

void FTaggedBinaryArchiveInputFormatter::LeaveField()
{
	CurrentType = EArchiveValueType::None;
}

bool FTaggedBinaryArchiveInputFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving)
{
	FRecord& Record = Records[RecordStack.Top()];
	int32 NameIdx = NameToIndex.FindChecked(Name.Name);
	const FField* Field = Record.Fields.FindByPredicate([NameIdx](const FField& TestField) -> bool { return TestField.NameIdx == NameIdx; });
	if (Field != nullptr)
	{
		Inner.Seek(Field->Offset);
		return true;
	}
	return false;
}

void FTaggedBinaryArchiveInputFormatter::EnterArray(int32& NumElements)
{
	ExpectType(EArchiveValueType::Array);
	Inner << NumElements;
}

void FTaggedBinaryArchiveInputFormatter::LeaveArray()
{
}

void FTaggedBinaryArchiveInputFormatter::EnterArrayElement()
{
}

void FTaggedBinaryArchiveInputFormatter::EnterArrayElement(EArchiveValueType& OutType)
{
	OutType = PeekType();
}

void FTaggedBinaryArchiveInputFormatter::LeaveArrayElement()
{
	CurrentType = EArchiveValueType::None;
}

void FTaggedBinaryArchiveInputFormatter::EnterStream()
{
	ExpectType(EArchiveValueType::Stream);

	int32 Length = 0;
	Inner << Length;
}

void FTaggedBinaryArchiveInputFormatter::EnterStream(int32& NumElements)
{
	ExpectType(EArchiveValueType::Stream);
	Inner << NumElements;
}

void FTaggedBinaryArchiveInputFormatter::LeaveStream()
{
}

void FTaggedBinaryArchiveInputFormatter::EnterStreamElement()
{
}

void FTaggedBinaryArchiveInputFormatter::EnterStreamElement(EArchiveValueType& OutType)
{
	OutType = PeekType();
}

void FTaggedBinaryArchiveInputFormatter::LeaveStreamElement()
{
	CurrentType = EArchiveValueType::None;
}

void FTaggedBinaryArchiveInputFormatter::EnterMap(int32& NumElements)
{
	ExpectType(EArchiveValueType::Map);
	Inner << NumElements;
}

void FTaggedBinaryArchiveInputFormatter::LeaveMap()
{
}

void FTaggedBinaryArchiveInputFormatter::EnterMapElement(FString& OutName)
{
	Inner << OutName;
}

void FTaggedBinaryArchiveInputFormatter::EnterMapElement(FString& OutName, EArchiveValueType& OutType)
{
	EnterMapElement(OutName);
	OutType = PeekType();
}

void FTaggedBinaryArchiveInputFormatter::LeaveMapElement()
{
	CurrentType = EArchiveValueType::None;
}

void FTaggedBinaryArchiveInputFormatter::Serialize(uint8& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(uint16& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(uint32& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(uint64& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(int8& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(int16& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(int32& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(int64& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(float& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(double& Value)
{
	ReadNumericValue(Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(bool& Value)
{
	ExpectType(EArchiveValueType::Bool);
	Inner << Value;
}

void FTaggedBinaryArchiveInputFormatter::Serialize(FString& Value)
{
	ExpectType(EArchiveValueType::String);
	Inner << Value;
}

void FTaggedBinaryArchiveInputFormatter::Serialize(FName& Value)
{
	ExpectType(EArchiveValueType::Name);

	FString ValueString;
	Inner << ValueString;
	Value = FName(*ValueString);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(UObject*& Value)
{
	ExpectType(EArchiveValueType::Object);
	SerializeObject(Inner, Value);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(TArray<uint8>& Value) 
{
	ExpectType(EArchiveValueType::RawData);

	int64 DataSize = Value.Num();
	Inner << DataSize;

	Value.SetNum(DataSize);
	Inner.Serialize(Value.GetData(), DataSize);
}

void FTaggedBinaryArchiveInputFormatter::Serialize(void* Data, uint64 DataSize)
{
	ExpectType(EArchiveValueType::RawData);

	int64 ActualDataSize = DataSize;
	Inner << ActualDataSize;
	check(ActualDataSize == DataSize);

	Inner.Serialize(Data, DataSize);
}

EArchiveValueType FTaggedBinaryArchiveInputFormatter::PeekType()
{
	if (CurrentType == EArchiveValueType::None)
	{
		uint8 TypeValue = 0;
		Inner << TypeValue;
		CurrentType = (EArchiveValueType)TypeValue;
	}
	return CurrentType;
}

EArchiveValueType FTaggedBinaryArchiveInputFormatter::ReadType()
{
	EArchiveValueType Type = PeekType();
	CurrentType = EArchiveValueType::None;
	return Type;
}

void FTaggedBinaryArchiveInputFormatter::ExpectType(EArchiveValueType Type)
{
	verify(ReadType() == Type);
}

template<typename NumericType> void FTaggedBinaryArchiveInputFormatter::ReadNumericValue(NumericType& OutValue)
{
	constexpr NumericType MinValue = TNumericLimits<NumericType>::Lowest();
	constexpr NumericType MaxValue = TNumericLimits<NumericType>::Max();

	EArchiveValueType Type = ReadType();
	switch(Type)
	{
	case EArchiveValueType::Int8:
		{
			int8 Value;
			Inner << Value;
			checkf(Value >= MinValue && Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::Int16:
		{
			int16 Value;
			Inner << Value;
			checkf(Value >= MinValue && Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::Int32:
		{
			int32 Value;
			Inner << Value;
			checkf(Value >= MinValue && Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::Int64:
		{
			int64 Value;
			Inner << Value;
			checkf(Value >= MinValue && Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::UInt8:
		{
			uint8 Value;
			Inner << Value;
			checkf(Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::UInt16:
		{
			uint16 Value;
			Inner << Value;
			checkf(Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::UInt32:
		{
			uint32 Value;
			Inner << Value;
			checkf(Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::UInt64:
		{
			uint64 Value;
			Inner << Value;
			checkf(Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::Float:
		{
			float Value;
			Inner << Value;
			checkf(Value >= MinValue && Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	case EArchiveValueType::Double:
		{
			double Value;
			Inner << Value;
			checkf(Value >= MinValue && Value <= MaxValue, TEXT("Serialized value does not fit within target type"));
			OutValue = (NumericType)Value;
			break;
		}
	default:
		checkf(false, TEXT("Cannot serialize value as numeric type"));
		OutValue = (NumericType)0;
		break;
	}
}

uint64 FTaggedBinaryArchiveInputFormatter::ReadSize()
{
	uint8 FirstByte = 0;
	Inner << FirstByte;

	if (FirstByte < 253)
	{
		return FirstByte;
	}
	else if (FirstByte == 253)
	{
		uint16 Offset16;
		Inner << Offset16;
		return Offset16;
	}
	else if (FirstByte == 254)
	{
		uint32 Offset32;
		Inner << Offset32;
		return Offset32;
	}
	else
	{
		uint64 Offset64;
		Inner << Offset64;
		return Offset64;
	}
}

int32 FTaggedBinaryArchiveInputFormatter::FindOrAddName(FString Name)
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

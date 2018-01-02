// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Serialization/StructuredArchiveFormatter.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"

//////////// FStructuredArchiveFormatter ////////////

FStructuredArchiveFormatter::~FStructuredArchiveFormatter()
{
}

//////////// Functions ////////////

#if WITH_TEXT_ARCHIVE_SUPPORT
	static void CopyFormattedSlot(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter, EArchiveValueType Type);

	template<typename Type> static void CopyFormattedValue(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter)
	{
		Type Value;
		InputFormatter.Serialize(Value);
		OutputFormatter.Serialize(Value);
	}

	static void CopyFormattedRecord(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter)
	{
		TArray<FString> Fields;
		InputFormatter.EnterRecord(Fields);
		OutputFormatter.EnterRecord();

		for (const FString& Field : Fields)
		{
			EArchiveValueType Type;
			InputFormatter.EnterField(FArchiveFieldName(*Field), Type);
			OutputFormatter.EnterField(FArchiveFieldName(*Field));

			CopyFormattedSlot(InputFormatter, OutputFormatter, Type);

			OutputFormatter.LeaveField();
			InputFormatter.LeaveField();
		}

		OutputFormatter.LeaveRecord();
		InputFormatter.LeaveRecord();
	}

	static void CopyFormattedArray(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter)
	{
		int32 NumElements = 0;
		InputFormatter.EnterArray(NumElements);
		OutputFormatter.EnterArray(NumElements);

		for (int32 Idx = 0; Idx < NumElements; Idx++)
		{
			EArchiveValueType Type;
			InputFormatter.EnterArrayElement(Type);
			OutputFormatter.EnterArrayElement();

			CopyFormattedSlot(InputFormatter, OutputFormatter, Type);

			OutputFormatter.LeaveArrayElement();
			InputFormatter.LeaveArrayElement();
		}

		OutputFormatter.LeaveArray();
		InputFormatter.LeaveArray();
	}

	static void CopyFormattedStream(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter)
	{
		int32 NumElements = 0;
		InputFormatter.EnterStream(NumElements);
		OutputFormatter.EnterStream();

		for (int32 Idx = 0; Idx < NumElements; Idx++)
		{
			EArchiveValueType Type;
			InputFormatter.EnterStreamElement(Type);
			OutputFormatter.EnterStreamElement();

			CopyFormattedSlot(InputFormatter, OutputFormatter, Type);

			OutputFormatter.LeaveStreamElement();
			InputFormatter.LeaveStreamElement();
		}

		OutputFormatter.LeaveStream();
		InputFormatter.LeaveStream();
	}

	static void CopyFormattedMap(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter)
	{
		int32 NumElements = 0;
		InputFormatter.EnterMap(NumElements);
		OutputFormatter.EnterMap(NumElements);

		for (int32 Idx = 0; Idx < NumElements; Idx++)
		{
			FString Key;
			EArchiveValueType Type;
			InputFormatter.EnterMapElement(Key, Type);
			OutputFormatter.EnterMapElement(Key);

			CopyFormattedSlot(InputFormatter, OutputFormatter, Type);

			OutputFormatter.LeaveMapElement();
			InputFormatter.LeaveMapElement();
		}

		OutputFormatter.LeaveMap();
		InputFormatter.LeaveMap();
	}

	static void CopyFormattedSlot(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter, EArchiveValueType Type)
	{
		switch (Type)
		{
		case EArchiveValueType::Record:
			CopyFormattedRecord(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Array:
			CopyFormattedArray(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Stream:
			CopyFormattedStream(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Map:
			CopyFormattedMap(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Int8:
			CopyFormattedValue<int8>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Int16:
			CopyFormattedValue<int16>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Int32:
			CopyFormattedValue<int32>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Int64:
			CopyFormattedValue<int64>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::UInt8:
			CopyFormattedValue<uint8>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::UInt16:
			CopyFormattedValue<uint16>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::UInt32:
			CopyFormattedValue<uint32>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::UInt64:
			CopyFormattedValue<uint64>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Float:
			CopyFormattedValue<float>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Double:
			CopyFormattedValue<double>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Bool:
			CopyFormattedValue<bool>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::String:
			CopyFormattedValue<FString>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Name:
			CopyFormattedValue<FName>(InputFormatter, OutputFormatter);
			break;
		case EArchiveValueType::Object:
			checkf(false, TEXT("Converting object types is not currently supported"));
			break;
		case EArchiveValueType::RawData:
			CopyFormattedValue<TArray<uint8>>(InputFormatter, OutputFormatter);
			break;
		}
	}

	CORE_API void CopyFormattedData(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter)
	{
		CopyFormattedRecord(InputFormatter, OutputFormatter);
	}
#endif

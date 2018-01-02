// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "JsonArchiveInputFormatter.h"
#include "JsonReader.h"
#include "JsonObject.h"
#include "JsonSerializer.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"

#if WITH_TEXT_ARCHIVE_SUPPORT

FJsonArchiveInputFormatter::FJsonArchiveInputFormatter(FArchive& InInner, TFunction<UObject* (const FString&)> InResolveObjectName) 
	: Inner(InInner)
	, ResolveObjectName(InResolveObjectName)
{
	Inner.ArIsTextFormat = true;

	TSharedPtr< FJsonObject > RootObject;
	TSharedRef< TJsonReader<char> > Reader = TJsonReaderFactory<char>::Create(&InInner);
	ensure(FJsonSerializer::Deserialize(Reader, RootObject));
	ValueStack.Add(MakeShared<FJsonValueObject>(RootObject));
}

FJsonArchiveInputFormatter::~FJsonArchiveInputFormatter()
{
}

FArchive& FJsonArchiveInputFormatter::GetUnderlyingArchive()
{
	return Inner;
}

void FJsonArchiveInputFormatter::EnterRecord()
{
	TSharedPtr<FJsonValue>& Value = ValueStack.Top();
	ObjectStack.Add(Value->AsObject());
}

void FJsonArchiveInputFormatter::EnterRecord(TArray<FString>& OutKeys)
{
	EnterRecord();
	ObjectStack.Top()->Values.GetKeys(OutKeys);
	for(int32 Idx = 0; Idx < OutKeys.Num(); Idx++)
	{
		OutKeys[Idx] = UnescapeFieldName(*OutKeys[Idx]);
	}
}

void FJsonArchiveInputFormatter::LeaveRecord()
{
	ObjectStack.Pop();
}

void FJsonArchiveInputFormatter::EnterField(FArchiveFieldName Name)
{
	TSharedPtr<FJsonObject>& NewObject = ObjectStack.Top();
	TSharedPtr<FJsonValue> Field = NewObject->TryGetField(EscapeFieldName(Name.Name));
	check(Field.IsValid());
	ValueStack.Add(Field);
}

void FJsonArchiveInputFormatter::EnterField(FArchiveFieldName Name, EArchiveValueType& OutType)
{
	EnterField(Name);
	OutType = GetValueType(*ValueStack.Top());
}

void FJsonArchiveInputFormatter::LeaveField()
{
	ValueStack.Pop();
}

bool FJsonArchiveInputFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving)
{
	TSharedPtr<FJsonObject>& NewObject = ObjectStack.Top();
	TSharedPtr<FJsonValue> Field = NewObject->TryGetField(EscapeFieldName(Name.Name));
	if (Field.IsValid())
	{
		ValueStack.Add(Field);
		return true;
	}
	return false;
}

void FJsonArchiveInputFormatter::EnterArray(int32& NumElements)
{
	TSharedPtr<FJsonValue>& Value = ValueStack.Top();

	const TArray<TSharedPtr<FJsonValue>>& ArrayValue = Value->AsArray();
	for(int Idx = ArrayValue.Num() - 1; Idx >= 0; Idx--)
	{
		ValueStack.Add(ArrayValue[Idx]);
	}

	NumElements = ArrayValue.Num();
}

void FJsonArchiveInputFormatter::LeaveArray()
{
}

void FJsonArchiveInputFormatter::EnterArrayElement()
{
}

void FJsonArchiveInputFormatter::EnterArrayElement(EArchiveValueType& OutType)
{
	OutType = GetValueType(*ValueStack.Top());
}

void FJsonArchiveInputFormatter::LeaveArrayElement()
{
	ValueStack.Pop();
}

void FJsonArchiveInputFormatter::EnterStream()
{
	int32 NumElements = 0;
	EnterArray(NumElements);
}

void FJsonArchiveInputFormatter::EnterStream(int32& NumElements)
{
	EnterArray(NumElements);
}

void FJsonArchiveInputFormatter::LeaveStream()
{
}

void FJsonArchiveInputFormatter::EnterStreamElement()
{
}

void FJsonArchiveInputFormatter::EnterStreamElement(EArchiveValueType& OutType)
{
	OutType = GetValueType(*ValueStack.Top());
}

void FJsonArchiveInputFormatter::LeaveStreamElement()
{
	LeaveArrayElement();
}

void FJsonArchiveInputFormatter::EnterMap(int32& NumElements)
{
	TSharedPtr<FJsonValue>& Value = ValueStack.Top();
	TSharedPtr<FJsonObject> Object = Value->AsObject();
	MapIteratorStack.Add(Object->Values);
	NumElements = Object->Values.Num();
}

void FJsonArchiveInputFormatter::LeaveMap()
{
	MapIteratorStack.Pop();
}

void FJsonArchiveInputFormatter::EnterMapElement(FString& OutName)
{
	OutName = UnescapeFieldName(*MapIteratorStack.Top()->Key);
	ValueStack.Add(MapIteratorStack.Top()->Value);
}

void FJsonArchiveInputFormatter::EnterMapElement(FString& OutName, EArchiveValueType& OutType)
{
	EnterMapElement(OutName);
	OutType = GetValueType(*ValueStack.Top());
}

void FJsonArchiveInputFormatter::LeaveMapElement()
{
	ValueStack.Pop();
	++MapIteratorStack.Top();
}

void FJsonArchiveInputFormatter::Serialize(uint8& Value)
{
	Value = (uint8)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(uint16& Value)
{
	Value = (uint16)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(uint32& Value)
{
	Value = (uint32)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(uint64& Value)
{
	Value = (uint64)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(int8& Value)
{
	Value = (int8)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(int16& Value)
{
	Value = (int16)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(int32& Value)
{
	Value = (int32)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(int64& Value)
{
	Value = (uint64)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(float& Value)
{
	Value = (float)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(double& Value)
{
	Value = (double)ValueStack.Top()->AsNumber();
}

void FJsonArchiveInputFormatter::Serialize(bool& Value)
{
	Value = ValueStack.Top()->AsBool();
}

void FJsonArchiveInputFormatter::Serialize(FString& Value)
{
	Value = ValueStack.Top()->AsString();
	Value.RemoveFromStart(TEXT("String:"));
}

void FJsonArchiveInputFormatter::Serialize(FName& Value)
{
	FString StringValue = ValueStack.Top()->AsString();

	const TCHAR Prefix[] = TEXT("Name:");
	if (ensure(StringValue.StartsWith(Prefix)))
	{
		Value = FName(*StringValue + ARRAY_COUNT(Prefix) - 1);
	}
	else
	{
		Value = NAME_None;
	}
}

void FJsonArchiveInputFormatter::Serialize(UObject*& Value)
{
	FString StringValue = ValueStack.Top()->AsString();

	const TCHAR Prefix[] = TEXT("Object:");
	if (ensure(StringValue.StartsWith(Prefix)))
	{
		Value = ResolveObjectName(*StringValue + ARRAY_COUNT(Prefix) - 1);
	}
	else
	{
		Value = nullptr;
	}
}

void FJsonArchiveInputFormatter::Serialize(TArray<uint8>& Data)
{
	const FJsonValue& Value = *ValueStack.Top();
	if(Value.Type == EJson::String)
	{
		// Single line string
		FString RawData = ValueStack.Top()->AsString();
		ensure(RawData.RemoveFromStart(TEXT("Base64:")));
		FBase64::Decode(RawData, Data);
	}
	else if(Value.Type == EJson::Object)
	{
		// Multi-line string
		FJsonObject& Object = *Value.AsObject();

		// Read the digest
		TSharedPtr<FJsonValue> DigestField = Object.TryGetField(TEXT("Digest"));
		checkf(DigestField.IsValid() && DigestField->Type == EJson::String, TEXT("Missing or invalid 'Digest' field for raw data"));
		FString Digest = DigestField->AsString();

		// Read the base64 array
		TSharedPtr<FJsonValue> Base64Field = Object.TryGetField(TEXT("Base64"));
		checkf(Base64Field.IsValid() && Base64Field->Type == EJson::Array, TEXT("Missing or invalid 'Base64' field for raw data"));
		const TArray<TSharedPtr<FJsonValue>>& Base64Array = Base64Field->AsArray();

		// Parse the digest
		uint8 ExpectedDigest[FSHA1::DigestSize];
		verify(FString::ToHexBlob(DigestField->AsString(), ExpectedDigest, sizeof(ExpectedDigest)));

		// Get the size of the encoded data
		uint32 DecodedSize = 0;
		for(const TSharedPtr<FJsonValue>& Base64Line : Base64Array)
		{
			DecodedSize += FBase64::GetDecodedDataSize(Base64Line->AsString());
		}

		// Read the encoded data
		Data.SetNum(DecodedSize);

		// Read each line
		uint32 DecodedPos = 0;
		for(const TSharedPtr<FJsonValue>& Base64Line : Base64Array)
		{
			FString Base64String = Base64Line->AsString();
			verify(FBase64::Decode(*Base64String, Base64String.Len(), Data.GetData() + DecodedPos));
			DecodedPos += FBase64::GetDecodedDataSize(Base64String);
		}

		// Make sure the digest matches
		uint8 ActualDigest[FSHA1::DigestSize];
		FSHA1::HashBuffer(Data.GetData(), Data.Num(), ActualDigest);
		checkf(FMemory::Memcmp(ExpectedDigest, ActualDigest, FSHA1::DigestSize) == 0, TEXT("Digest does not match for raw data. Check that this file was merged correctly."));
	}
	else
	{
		checkf(false, TEXT("Invalid value type for raw data"));
	}
}

void FJsonArchiveInputFormatter::Serialize(void* Data, uint64 DataSize)
{
	TArray<uint8> Buffer;
	Serialize(Buffer);
	check(Buffer.Num() == DataSize);
	memcpy(Data, Buffer.GetData(), DataSize);
}

FString FJsonArchiveInputFormatter::EscapeFieldName(const TCHAR* Name)
{
	if(FCString::Stricmp(Name, TEXT("Base64")) == 0 || FCString::Stricmp(Name, TEXT("Digest")) == 0 || Name[0] == '_')
	{
		return FString::Printf(TEXT("_%s"), Name);
	}
	else
	{
		return Name;
	}
}

FString FJsonArchiveInputFormatter::UnescapeFieldName(const TCHAR* Name)
{
	if(Name[0] == '_')
	{
		return Name + 1;
	}
	else
	{
		return Name;
	}
}

EArchiveValueType FJsonArchiveInputFormatter::GetValueType(const FJsonValue& Value)
{
	switch (Value.Type)
	{
	case EJson::String:
		if (Value.AsString().StartsWith(TEXT("Name:")))
		{
			return EArchiveValueType::Name;
		}
		else if (Value.AsString().StartsWith(TEXT("Object:")))
		{
			return EArchiveValueType::Object;
		}
		else if (Value.AsString().StartsWith(TEXT("Base64:")))
		{
			return EArchiveValueType::RawData;
		}
		else
		{
			return EArchiveValueType::String;
		}
	case EJson::Number:
		{
			double Number = Value.AsNumber();
			int64 NumberInt64 = (int64)Number;
			if((double)NumberInt64 == Number)
			{
				if((int16)NumberInt64 == NumberInt64)
				{
					if((int8)NumberInt64 == NumberInt64)
					{
						return EArchiveValueType::Int8;
					}
					else
					{
						return EArchiveValueType::Int16;
					}
				}
				else
				{
					if((int32)NumberInt64 == NumberInt64)
					{
						return EArchiveValueType::Int32;
					}
					else
					{
						return EArchiveValueType::Int64;
					}
				}
			}
			else
			{
				if((double)(float)Number == Number)
				{
					return EArchiveValueType::Float;
				}
				else
				{
					return EArchiveValueType::Double;
				}
			}
		}
	case EJson::Boolean:
		return EArchiveValueType::Bool;
	case EJson::Array:
		return EArchiveValueType::Array;
	case EJson::Object:
		if(Value.AsObject()->HasField(TEXT("Base64")))
		{
			return EArchiveValueType::RawData;
		}
		else
		{
			return EArchiveValueType::Record;
		}
	case EJson::Null:
		return EArchiveValueType::Object;
	default:
		checkf(false, TEXT("Unhandled value type in JSON archive (%d)"), (int)Value.Type);
		return EArchiveValueType::None;
	}
}

#endif

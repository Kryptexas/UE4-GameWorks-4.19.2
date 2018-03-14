// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "JsonArchiveOutputFormatter.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "Object.h"

#if WITH_TEXT_ARCHIVE_SUPPORT

FJsonArchiveOutputFormatter::FJsonArchiveOutputFormatter(FArchive& InInner) 
	: Inner(InInner)
	, Newline(LINE_TERMINATOR_ANSI, ARRAY_COUNT(LINE_TERMINATOR_ANSI) - 1)
{
	Inner.ArIsTextFormat = true;

	bNeedsComma = false;
	bNeedsNewline = false;
}

FJsonArchiveOutputFormatter::~FJsonArchiveOutputFormatter()
{
}

FArchive& FJsonArchiveOutputFormatter::GetUnderlyingArchive()
{
	return Inner;
}

void FJsonArchiveOutputFormatter::EnterRecord()
{
	WriteOptionalComma();
	WriteOptionalNewline();
	Write("{");
	Newline.Add('\t');
	bNeedsNewline = true;
}

void FJsonArchiveOutputFormatter::LeaveRecord()
{
	Newline.Pop(false);
	WriteOptionalNewline();
	Write("}");
	bNeedsComma = true;
	bNeedsNewline = true;
}

void FJsonArchiveOutputFormatter::EnterField(FArchiveFieldName Name)
{
	WriteOptionalComma();
	WriteOptionalNewline();
	WriteFieldName(Name.Name);
}

void FJsonArchiveOutputFormatter::LeaveField()
{
	bNeedsComma = true;
	bNeedsNewline = true;
}

bool FJsonArchiveOutputFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving)
{
	if (bEnterWhenSaving)
	{
		EnterField(Name);
	}
	return bEnterWhenSaving;
}

void FJsonArchiveOutputFormatter::EnterArray(int32& NumElements)
{
	EnterStream();
}

void FJsonArchiveOutputFormatter::LeaveArray()
{
	LeaveStream();
}

void FJsonArchiveOutputFormatter::EnterArrayElement()
{
	EnterStreamElement();
}

void FJsonArchiveOutputFormatter::LeaveArrayElement()
{
	LeaveStreamElement();
}

void FJsonArchiveOutputFormatter::EnterStream()
{
	WriteOptionalComma();
	WriteOptionalNewline();
	Write("[");
	Newline.Add('\t');
	bNeedsNewline = true;
}

void FJsonArchiveOutputFormatter::LeaveStream()
{
	Newline.Pop(false);
	WriteOptionalNewline();
	Write("]");
	bNeedsComma = true;
	bNeedsNewline = true;
}

void FJsonArchiveOutputFormatter::EnterStreamElement()
{
	WriteOptionalComma();
	WriteOptionalNewline();
}

void FJsonArchiveOutputFormatter::LeaveStreamElement()
{
	bNeedsComma = true;
	bNeedsNewline = true;
}

void FJsonArchiveOutputFormatter::EnterMap(int32& NumElements)
{
	EnterRecord();
}

void FJsonArchiveOutputFormatter::LeaveMap()
{
	LeaveRecord();
}

void FJsonArchiveOutputFormatter::EnterMapElement(FString& Name)
{
	EnterField(FArchiveFieldName(*Name));
}

void FJsonArchiveOutputFormatter::LeaveMapElement()
{
	LeaveField();
}

void FJsonArchiveOutputFormatter::Serialize(uint8& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(uint16& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(uint32& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(uint64& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(int8& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(int16& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(int32& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(int64& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(float& Value)
{
	if((float)(int)Value == Value)
	{
		WriteValue(Lex::ToString((int)Value));
	}
	else
	{
		WriteValue(Lex::ToString(Value));
	}
}

void FJsonArchiveOutputFormatter::Serialize(double& Value)
{
	if((double)(int)Value == Value)
	{
		WriteValue(Lex::ToString((int)Value));
	}
	else
	{
		WriteValue(Lex::ToString(Value));
	}
}

void FJsonArchiveOutputFormatter::Serialize(bool& Value)
{
	WriteValue(Lex::ToString(Value));
}

void FJsonArchiveOutputFormatter::Serialize(FString& Value)
{
	FString Result = TEXT("\"");

	// Insert a "String:" prefix to prevent incorrect interpretation as another explicit type
	if (Value.StartsWith(TEXT("Name:")) || Value.StartsWith(TEXT("Object:")) || Value.StartsWith(TEXT("String:")) || Value.StartsWith(TEXT("Base64:")))
	{
		Result += TEXT("String:");
	}

	// Escape the string characters
	for (int32 Idx = 0; Idx < Value.Len(); Idx++)
	{
		switch (Value[Idx])
		{
		case '\"':
			Result += "\\\"";
			break;
		case '\\':
			Result += "\\\\";
			break;
		case '\b':
			Result += "\\b";
			break;
		case '\f':
			Result += "\\f";
			break;
		case '\n':
			Result += "\\n";
			break;
		case '\r':
			Result += "\\r";
			break;
		case '\t':
			Result += "\\t";
			break;
		default:
			if (Value[Idx] <= 0x1f || Value[Idx] >= 0x7f)
			{
				Result += FString::Printf(TEXT("\\u%04x"), Value[Idx]);
			}
			else
			{
				Result.AppendChar(Value[Idx]);
			}
			break;
		}
	}
	Result += TEXT("\"");

	WriteValue(Result);
}

void FJsonArchiveOutputFormatter::Serialize(FName& Value)
{
	WriteValue(FString::Printf(TEXT("\"Name:%s\""), *Value.ToString()));
}

void FJsonArchiveOutputFormatter::Serialize(UObject*& Value)
{
	if (Value == nullptr)
	{
		WriteValue(TEXT("null"));
	}
	else
	{
		WriteValue(FString::Printf(TEXT("\"Object:%s\""), *Value->GetFullName()));
	}
}

void FJsonArchiveOutputFormatter::Serialize(TArray<uint8>& Data)
{
	Serialize(Data.GetData(), Data.Num());
}

void FJsonArchiveOutputFormatter::Serialize(void* Data, uint64 DataSize)
{
	static const int32 MaxLineChars = 120;
	static const int32 MaxLineBytes = FBase64::GetMaxDecodedDataSize(MaxLineChars);

	if(DataSize < MaxLineBytes)
	{
		// Encode the data on a single line. No need for hashing; intra-line merge conflicts are rare.
		WriteValue(FString::Printf(TEXT("\"Base64:%s\""), *FBase64::Encode((const uint8*)Data, DataSize)));
	}
	else
	{
		// Encode the data as a record containing a digest and array of base-64 encoded lines
		EnterRecord();
		Inner.Serialize(Newline.GetData(), Newline.Num());

		// Compute a SHA digest for the raw data, so we can check if it's corrupted
		uint8 Digest[FSHA1::DigestSize];
		FSHA1::HashBuffer(Data, DataSize, Digest);

		// Convert the hash to a string
		ANSICHAR DigestString[(FSHA1::DigestSize * 2) + 1];
		for(int32 Idx = 0; Idx < ARRAY_COUNT(Digest); Idx++)
		{
			static const ANSICHAR HexDigits[] = "0123456789abcdef";
			DigestString[(Idx * 2) + 0] = HexDigits[Digest[Idx] >> 4];
			DigestString[(Idx * 2) + 1] = HexDigits[Digest[Idx] & 15];
		}
		DigestString[ARRAY_COUNT(DigestString) - 1] = 0;

		// Write the digest
		Write("\"Digest\": \"");
		Write(DigestString);
		Write("\",");
		Inner.Serialize(Newline.GetData(), Newline.Num());

		// Write the base64 data
		Write("\"Base64\": ");
		for(uint64 DataPos = 0; DataPos < DataSize; DataPos += MaxLineBytes)
		{
			Write((DataPos > 0)? ',' : '[');
			Inner.Serialize(Newline.GetData(), Newline.Num());
			Write("\t\"");

			ANSICHAR LineData[MaxLineChars + 1];
			uint64 NumLineChars = FBase64::Encode((const uint8*)Data + DataPos, FMath::Min<uint64>(DataSize - DataPos, MaxLineBytes), LineData);
			Inner.Serialize(LineData, NumLineChars);

			Write("\"");
		}

		// Close the array
		Inner.Serialize(Newline.GetData(), Newline.Num());
		Write(']');
		bNeedsNewline = true;

		// Close the record
		LeaveRecord();
	}
}

void FJsonArchiveOutputFormatter::Write(ANSICHAR Character)
{
	Inner.Serialize((void*)&Character, 1);
}

void FJsonArchiveOutputFormatter::Write(const ANSICHAR* Text)
{
	Inner.Serialize((void*)Text, TCString<ANSICHAR>::Strlen(Text));
}

void FJsonArchiveOutputFormatter::Write(const FString& Text)
{
	Write(TCHAR_TO_UTF8(*Text));
}

void FJsonArchiveOutputFormatter::WriteFieldName(const TCHAR* Name)
{
	if(FCString::Stricmp(Name, TEXT("Base64")) == 0 || FCString::Stricmp(Name, TEXT("Digest")) == 0)
	{
		Write(FString::Printf(TEXT("\"_%s\": "), Name));
	}
	else if(Name[0] == '_')
	{
		Write(FString::Printf(TEXT("\"_%s\": "), Name));
	}
	else
	{
		Write(FString::Printf(TEXT("\"%s\": "), Name));
	}
}

void FJsonArchiveOutputFormatter::WriteValue(const FString& Text)
{
	Write(Text);
}

void FJsonArchiveOutputFormatter::WriteOptionalComma()
{
	if (bNeedsComma)
	{
		Write(',');
		bNeedsComma = false;
	}
}

void FJsonArchiveOutputFormatter::WriteOptionalNewline()
{
	if (bNeedsNewline)
	{
		Inner.Serialize(Newline.GetData(), Newline.Num());
		bNeedsNewline = false;
	}
}

#endif

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineKismetLibraryClasses.h"

//////////////////////////////////////////////////////////////////////////
// UKismetStringLibrary

UKismetStringLibrary::UKismetStringLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UKismetStringLibrary::Concat_StrStr(const FString& A, const FString& B)
{
	// faster, preallocating method
	FString StringResult;
	StringResult.Empty(A.Len() + B.Len() + 1); // adding one for the string terminator
	StringResult += A;
	StringResult += B;

	return StringResult;
}

bool UKismetStringLibrary::EqualEqual_StriStri(const FString& A, const FString& B)
{
	return FCString::Stricmp(*A, *B) == 0;
}

bool UKismetStringLibrary::EqualEqual_StrStr(const FString& A, const FString& B)
{
	return FCString::Strcmp(*A, *B) == 0;
}

bool UKismetStringLibrary::NotEqual_StriStri(const FString& A, const FString& B)
{
	return FCString::Stricmp(*A, *B) != 0;
}

bool UKismetStringLibrary::NotEqual_StrStr(const FString& A, const FString& B)
{
	return FCString::Strcmp(*A, *B) != 0;
}

int32 UKismetStringLibrary::Len(const FString& S)
{
	return S.Len();
}

FString UKismetStringLibrary::Conv_FloatToString(float InFloat)
{
	return FString::SanitizeFloat(InFloat);
}

FString UKismetStringLibrary::Conv_IntToString(int32 InInt)
{
	return FString::Printf(TEXT("%d"), InInt);	
}

FString UKismetStringLibrary::Conv_ByteToString(uint8 InByte)
{
	return FString::Printf(TEXT("%d"), InByte);	
}

FString UKismetStringLibrary::Conv_BoolToString(bool InBool)
{
	return InBool ? TEXT("true") : TEXT("false");	
}

FString UKismetStringLibrary::Conv_VectorToString(FVector InVec)
{
	return InVec.ToString();	
}

FString UKismetStringLibrary::Conv_Vector2dToString(FVector2D InVec)
{
	return InVec.ToString();	
}

FString UKismetStringLibrary::Conv_RotatorToString(FRotator InRot)
{
	return InRot.ToString();	
}

FString UKismetStringLibrary::Conv_ObjectToString(class UObject* InObj)
{
	return (InObj != NULL) ? InObj->GetName() : FString(TEXT("None"));
}

FString UKismetStringLibrary::Conv_ColorToString(FLinearColor C)
{
	return C.ToString();
}

FString UKismetStringLibrary::Conv_NameToString(FName InName)
{
	return InName.ToString();
}

FName UKismetStringLibrary::Conv_StringToName(const FString& InString)
{
	return FName(*InString);
}

int32 UKismetStringLibrary::Conv_StringToInt(const FString& InString)
{
	return FCString::Atoi(*InString);
}

float UKismetStringLibrary::Conv_StringToFloat(const FString& InString)
{
	return FCString::Atof(*InString);
}

FString UKismetStringLibrary::BuildString_Float(const FString& AppendTo, const FString& Prefix, float InFloat, const FString& Suffix)
{
	// faster, preallocating method
	FString const FloatStr = FString::SanitizeFloat(InFloat);

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+FloatStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += FloatStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Int(const FString& AppendTo, const FString& Prefix, int32 InInt, const FString& Suffix)
{
	// faster, preallocating method
	FString const IntStr = FString::Printf(TEXT("%d"), InInt);

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+IntStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += IntStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Bool(const FString& AppendTo, const FString& Prefix, bool InBool, const FString& Suffix)
{
	// faster, preallocating method
	FString const BoolStr = InBool ? TEXT("true") : TEXT("false");	

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+BoolStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += BoolStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Vector(const FString& AppendTo, const FString& Prefix, FVector InVector, const FString& Suffix)
{
	// faster, preallocating method
	FString const VecStr = InVector.ToString();

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+VecStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += VecStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Vector2d(const FString& AppendTo, const FString& Prefix, FVector2D InVector2d, const FString& Suffix)
{
	// faster, preallocating method
	FString const VecStr = InVector2d.ToString();

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+VecStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += VecStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Rotator(const FString& AppendTo, const FString& Prefix, FRotator InRot, const FString& Suffix)
{
	// faster, preallocating method
	FString const RotStr = InRot.ToString();

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+RotStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += RotStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Object(const FString& AppendTo, const FString& Prefix, class UObject* InObj, const FString& Suffix)
{
	// faster, preallocating method
	FString const ObjStr = (InObj != NULL) ? InObj->GetName() : FString(TEXT("None"));

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+ObjStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += ObjStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Color(const FString& AppendTo, const FString& Prefix, FLinearColor InColor, const FString& Suffix)
{
	// faster, preallocating method
	FString const ColorStr = InColor.ToString();

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+ColorStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += ColorStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::BuildString_Name(const FString& AppendTo, const FString& Prefix, FName InName, const FString& Suffix)
{
	// faster, preallocating method
	FString const NameStr = InName.ToString();

	FString StringResult;
	StringResult.Empty(AppendTo.Len()+Prefix.Len()+NameStr.Len()+Suffix.Len()+1); // adding one for the string terminator
	StringResult += AppendTo;
	StringResult += Prefix;
	StringResult += NameStr;
	StringResult += Suffix;

	return StringResult;
}

FString UKismetStringLibrary::GetSubstring(const FString& SourceString, int32 StartIndex, int32 Length)
{
	return SourceString.Mid(StartIndex, Length);
}

int32 UKismetStringLibrary::GetCharacterAsNumber(const FString& SourceString, int32 Index)
{
	if ((Index >= 0) && (Index < SourceString.Len()))
	{
		return (int32)(SourceString.GetCharArray()[Index]);
	}
	else
	{
		//@TODO: Script error
		return 0;
	}
}

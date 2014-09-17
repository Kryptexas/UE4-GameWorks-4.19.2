// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeSpawnerSignature.h"

/*******************************************************************************
 * Static FBlueprintNodeSpawnerSignature Helpers
 ******************************************************************************/

namespace BlueprintNodeSpawnerSignatureImpl
{
	static FString const SignatureOpeningStr("(");
	static FString const SignatureElementDelim(",");
	static FString const SignatureClosingStr(")");
	static FString const SignatureKeyDelim("=");
}

/*******************************************************************************
 * FBlueprintNodeSpawnerSignature
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintNodeSpawnerSignature::FBlueprintNodeSpawnerSignature(FString const& UserString)
{
	using namespace BlueprintNodeSpawnerSignatureImpl;

	FString SanitizedSignature = UserString;
	SanitizedSignature.RemoveFromStart(SignatureOpeningStr);
	SanitizedSignature.RemoveFromEnd(SignatureClosingStr);

	TArray<FString> SignatureElements;
	SanitizedSignature.ParseIntoArray(&SignatureElements, *SignatureElementDelim, 1);

	for (FString& SignatureElement : SignatureElements)
	{
		SignatureElement.Trim();

		FString SignatureKey, SignatureValue;
		SignatureElement.Split(SignatureKeyDelim, &SignatureKey, &SignatureValue);
		// @TODO: look for UObject redirects with SignatureValue

		SignatureValue.RemoveFromStart(TEXT("\""));
		SignatureValue.RemoveFromEnd(TEXT("\""));
		AddKeyValue(FName(*SignatureKey), SignatureValue);
	}
}

//------------------------------------------------------------------------------
FBlueprintNodeSpawnerSignature::FBlueprintNodeSpawnerSignature(TSubclassOf<UEdGraphNode> NodeClass)
{
	SetNodeClass(NodeClass);
}

//------------------------------------------------------------------------------
void FBlueprintNodeSpawnerSignature::SetNodeClass(TSubclassOf<UEdGraphNode> NodeClass)
{
	static const FName NodeClassSignatureKey(TEXT("NodeName"));

	if (NodeClass != nullptr)
	{
		AddKeyValue(NodeClassSignatureKey, NodeClass->GetPathName());
		MarkDirty();
	}
	else
	{
		SignatureSet.Remove(NodeClassSignatureKey);
	}
}

//------------------------------------------------------------------------------
void FBlueprintNodeSpawnerSignature::AddSubObject(UObject const* SignatureObj)
{
	// not ideal for generic "objects", but we have to keep in sync with the 
	// old favorites system (for backwards compatibility)
	static FName const BaseSubObjectSignatureKey(TEXT("FieldName"));
	FName SubObjectSignatureKey = BaseSubObjectSignatureKey;

	int32 FNameIndex = 0;
	while (SignatureSet.Find(SubObjectSignatureKey) != nullptr)
	{
		SubObjectSignatureKey = FName(BaseSubObjectSignatureKey, ++FNameIndex);
	}

	AddKeyValue(SubObjectSignatureKey, SignatureObj->GetPathName());
	MarkDirty();
}

//------------------------------------------------------------------------------
void FBlueprintNodeSpawnerSignature::AddKeyValue(FName SignatureKey, FString const& Value)
{
	SignatureSet.Add(SignatureKey, Value);
	MarkDirty();
}

//------------------------------------------------------------------------------
bool FBlueprintNodeSpawnerSignature::IsValid() const
{
	return (SignatureSet.Num() > 0);
}

//------------------------------------------------------------------------------
FString const& FBlueprintNodeSpawnerSignature::ToString() const
{
	using namespace BlueprintNodeSpawnerSignatureImpl;

	if (CachedSignatureString.IsEmpty() && IsValid())
	{
		TArray<FString> SignatureElements;
		for (auto& SignatureElement : SignatureSet)
		{
			FString ObjSignature = SignatureElement.Key.ToString() + SignatureKeyDelim + TEXT("\"") + SignatureElement.Value + TEXT("\"");
			SignatureElements.Add(ObjSignature);
		}
		SignatureElements.Sort();

		CachedSignatureString = SignatureOpeningStr;
		for (FString& SignatureElement : SignatureElements)
		{
			CachedSignatureString += SignatureElement + SignatureElementDelim;
		}
		CachedSignatureString.RemoveFromEnd(SignatureElementDelim);
		CachedSignatureString += SignatureClosingStr;
	}
	
	return CachedSignatureString;
}

//------------------------------------------------------------------------------
FGuid const& FBlueprintNodeSpawnerSignature::AsGuid() const
{
	static const int32 BytesPerMd5Hash = 16;
	static const int32 BytesPerGuidVal =  4;
	static const int32 MembersPerGuid  =  4;
	static const int32 BitsPerByte     =  8;

	if (!CachedSignatureGuid.IsValid() && IsValid())
	{
		FString const& SignatureString = ToString();

		FMD5  Md5Gen;
		uint8 HashedBytes[BytesPerMd5Hash];
		Md5Gen.Update((unsigned char*)TCHAR_TO_ANSI(*SignatureString), SignatureString.Len());
		Md5Gen.Final(HashedBytes);
		//FString HashedStr = FMD5::HashAnsiString(*SignatureString); // to check against

		for (int32 GuidIndex = 0; GuidIndex < MembersPerGuid; ++GuidIndex)
		{
			int32 const Md5UpperIndex = (GuidIndex + 1) * BytesPerGuidVal - 1;
			int32 const Md5LowerIndex = Md5UpperIndex - BytesPerGuidVal;

			int32 GuidValue = 0;

			int32 BitOffset = 1;
			for (int32 Md5ByteIndex = Md5UpperIndex; Md5ByteIndex >= Md5LowerIndex; --Md5ByteIndex)
			{
				GuidValue += HashedBytes[Md5ByteIndex] * BitOffset;
				BitOffset <<= BitsPerByte;
			}
			CachedSignatureGuid[GuidIndex] = GuidValue;
		}
	}
	return CachedSignatureGuid;
}

//------------------------------------------------------------------------------
void FBlueprintNodeSpawnerSignature::MarkDirty()
{
	CachedSignatureGuid.Invalidate();
	CachedSignatureString.Empty();
}

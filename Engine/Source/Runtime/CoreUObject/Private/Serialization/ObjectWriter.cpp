// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Serialization/ObjectWriter.h"
#include "UObject/LazyObjectPtr.h"
#include "Misc/StringAssetReference.h"
#include "UObject/AssetPtr.h"

///////////////////////////////////////////////////////
// FObjectWriter

FArchive& FObjectWriter::operator<<( class FName& N )
{
	NAME_INDEX ComparisonIndex = N.GetComparisonIndex();
	NAME_INDEX DisplayIndex = N.GetDisplayIndex();
	int32 Number = N.GetNumber();
	ByteOrderSerialize(&ComparisonIndex, sizeof(ComparisonIndex));
	ByteOrderSerialize(&DisplayIndex, sizeof(DisplayIndex));
	ByteOrderSerialize(&Number, sizeof(Number));
	return *this;
}

FArchive& FObjectWriter::operator<<( class UObject*& Res )
{
	ByteOrderSerialize(&Res, sizeof(Res));
	return *this;
}

FArchive& FObjectWriter::operator<<( class FLazyObjectPtr& LazyObjectPtr )
{
	FUniqueObjectGuid ID = LazyObjectPtr.GetUniqueID();
	return *this << ID;
}

FArchive& FObjectWriter::operator<<( class FAssetPtr& AssetPtr )
{
	AssetPtr.ResetWeakPtr();
	return *this << AssetPtr.GetUniqueID();
}

FArchive& FObjectWriter::operator<<(FStringAssetReference& Value)
{
	Value.SerializePath(*this);
	return *this;
}

FArchive& FObjectWriter::operator<< (struct FWeakObjectPtr& Value)
{
	Value.Serialize(*this);
	return *this;
}

FString FObjectWriter::GetArchiveName() const
{
	return TEXT("FObjectWriter");
}

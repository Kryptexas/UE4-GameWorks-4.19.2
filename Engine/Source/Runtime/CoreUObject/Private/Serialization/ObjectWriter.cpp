// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

///////////////////////////////////////////////////////
// FObjectWriter

FArchive& FObjectWriter::operator<<( class FName& N )
{
	NAME_INDEX Name = N.GetIndex();
	int32 Number = N.GetNumber();
	ByteOrderSerialize(&Name, sizeof(Name));
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
	FStringAssetReference ID = AssetPtr.GetUniqueID();
	return *this << ID;
}

FString FObjectWriter::GetArchiveName() const
{
	return TEXT("FObjectWriter");
}

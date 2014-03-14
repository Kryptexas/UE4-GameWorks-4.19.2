// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

/*----------------------------------------------------------------------------
	FDuplicateDataReader.
----------------------------------------------------------------------------*/
/**
 * Constructor
 *
 * @param	InDuplicatedObjects		map of original object to copy of that object
 * @param	InObjectData			object data to read from
 */
FDuplicateDataReader::FDuplicateDataReader( class FUObjectAnnotationSparse<FDuplicatedObject,false>& InDuplicatedObjects ,const TArray<uint8>& InObjectData, uint32 InPortFlags )
: DuplicatedObjectAnnotation(InDuplicatedObjects)
, ObjectData(InObjectData)
, Offset(0)
{
	ArIsLoading			= true;
	ArIsPersistent		= true;
	ArPortFlags |= PPF_Duplicate | InPortFlags;
}

void FDuplicateDataReader::SerializeFail()
{
	extern UObject* GSerializedObject;
	UE_LOG(LogObj, Fatal, TEXT("FDuplicateDataReader Overread. GSerializedObject = %s GSerializedProperty = %s"), *GetFullNameSafe(GSerializedObject), *GetFullNameSafe(GSerializedProperty));
}


FArchive& FDuplicateDataReader::operator<<( UObject*& Object )
{
	UObject*	SourceObject = Object;
	Serialize(&SourceObject,sizeof(UObject*));

	FDuplicatedObject ObjectInfo = SourceObject ? DuplicatedObjectAnnotation.GetAnnotation(SourceObject) : FDuplicatedObject();
	if( !ObjectInfo.IsDefault() )
	{
		Object = ObjectInfo.DuplicatedObject;
	}
	else
	{
		Object = SourceObject;
	}

	return *this;
}

FArchive& FDuplicateDataReader::operator<<( FLazyObjectPtr& LazyObjectPtr)
{
	FArchive& Ar = *this;
	FUniqueObjectGuid ID;
	Ar << ID;

	if (Ar.GetPortFlags()&PPF_DuplicateForPIE)
	{
		// Remap unique ID if necessary
		ID = ID.FixupForPIE();
	}
	LazyObjectPtr = ID;
	return Ar;
}

FArchive& FDuplicateDataReader::operator<<( FAssetPtr& AssetPtr)
{
	FArchive& Ar = *this;
	FStringAssetReference ID;
	Ar << ID;

	if (Ar.GetPortFlags()&PPF_DuplicateForPIE)
	{
		// Remap unique ID if necessary
		ID = ID.FixupForPIE();
	}
	AssetPtr = ID;
	return Ar;
}

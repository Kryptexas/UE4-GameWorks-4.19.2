// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*====================================================================================
	ArchiveUObjectBase.h: Implements the FArchiveUObject class for serializing UObjects
======================================================================================*/

#pragma once

/**
 * Base FArchive for serializing UObjects. Supports FLazyObjectPtr and FAssetPtr serialization.
 */
class COREUOBJECT_API FArchiveUObject : public FArchive
{
public:

	// Begin FArchive Interface
	virtual FArchive& operator<< (class FLazyObjectPtr& Value) OVERRIDE;
	virtual FArchive& operator<< (class FAssetPtr& Value) OVERRIDE;
	// End FArchive Interface
};

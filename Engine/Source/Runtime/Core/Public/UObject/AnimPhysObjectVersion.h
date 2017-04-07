// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes made in Dev-AnimPhys stream
struct CORE_API FAnimPhysObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,
		// convert animnode look at to use just default axis instead of enum, which doesn't do much
		ConvertAnimNodeLookAtAxis = 1,
		// Change FKSphylElem and FKBoxElem to use Rotators not Quats for easier editing
		BoxSphylElemsUseRotators = 2,
		// Change thumbnail scene info and asset import data to be transactional
		ThumbnailSceneInfoAndAssetImportDataAreTransactional = 3,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FAnimPhysObjectVersion() {}
};

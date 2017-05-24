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
		BeforeCustomVersionWasAdded,
		// convert animnode look at to use just default axis instead of enum, which doesn't do much
		ConvertAnimNodeLookAtAxis,
		// Change FKSphylElem and FKBoxElem to use Rotators not Quats for easier editing
		BoxSphylElemsUseRotators,
		// Change thumbnail scene info and asset import data to be transactional
		ThumbnailSceneInfoAndAssetImportDataAreTransactional,
		// Enabled clothing masks rather than painting parameters directly
		AddedClothingMaskWorkflow,
		// Remove UID from smart name serialize, it just breaks determinism 
		RemoveUIDFromSmartNameSerialize,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FAnimPhysObjectVersion() {}
};

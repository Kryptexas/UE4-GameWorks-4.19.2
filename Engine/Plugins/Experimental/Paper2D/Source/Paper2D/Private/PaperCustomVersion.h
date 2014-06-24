// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Custom serialization version for all packages containing Paper2D asset types
struct FPaperCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,

		// Fixed negative volume in UPaperSprite 3D aggregate geometry (caused collision issues)
		FixedNegativeVolume = 1,

		// Removed FBodyInstance2D and moved 2D physics support into Engine (requires regenerating 2D agg geom)
		RemovedBodyInstance2D = 2,

		// Moved tile map data out of the component into UPaperTileMap
		MovedTileMapDataToSeparateClass = 3,

		// Marked sprite UBodySetup to use simple as complex, allowing 3D raycasts to succeed
		MarkSpriteBodySetupToUseSimpleAsComplex = 4,

		// Added PixelsPerUnrealUnit to move away from a 1:1 relationship between pixels and uu
		AddPixelsPerUnrealUnit = 5,

		// Fixed typo in convex hull generation code for 3D custom collision
		FixTypoIn3DConvexHullCollisionGeneration = 6,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FPaperCustomVersion() {}
};

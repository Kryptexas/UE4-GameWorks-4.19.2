// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

// Custom serialization version for changes made in Dev-Rendering stream
struct CORE_API FRenderingObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,

		// Added support for 3 band SH in the ILC
		IndirectLightingCache3BandSupport,

		// Allows specifying resolution for reflecture capture probes
		CustomReflectionCaptureResolutionSupport,

		// Allows specifying resolution for reflecture capture probes
		RemovedTextureStreamingLevelData,

		// translucency is now a property which matters for materials with the decal domain
		IntroducedMeshDecals,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FRenderingObjectVersion() {}
};

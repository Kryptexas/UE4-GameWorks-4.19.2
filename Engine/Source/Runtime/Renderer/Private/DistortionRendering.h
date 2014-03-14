// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistortionRendering.h: Distortion rendering implementation.
=============================================================================*/

#pragma once

/** 
* Set of distortion scene prims  
*/
class FDistortionPrimSet
{
public:

	/** 
	* Iterate over the distortion prims and draw their accumulated offsets
	* @param ViewInfo - current view used to draw items
	* @param DPGIndex - current DPG used to draw items
	* @return true if anything was drawn
	*/
	bool DrawAccumulatedOffsets(const class FViewInfo* ViewInfo,bool bInitializeOffsets);

	/**
	* Add a new primitive to the list of distortion prims
	* @param PrimitiveSceneProxy - primitive info to add.
	* @param ViewInfo - used to transform bounds to view space
	*/
	void AddScenePrimitive(FPrimitiveSceneProxy* PrimitiveSceneProxy,const FViewInfo& ViewInfo);

	/** 
	* @return number of prims to render
	*/
	int32 NumPrims() const
	{
		return Prims.Num();
	}

	/** 
	* @return a prim currently set to render
	*/
	const FPrimitiveSceneProxy* GetPrim(int32 i)const
	{
		check(i>=0 && i<NumPrims());
		return Prims[i];
	}

private:
	/** list of distortion prims added from the scene */
	TArray<FPrimitiveSceneProxy*> Prims;
};

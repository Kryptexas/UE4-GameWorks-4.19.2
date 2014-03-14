// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class USkeletalMesh;

//////////////////////////////////////////////////////////////////////////
// FSkeletalMeshUpdateContext


struct FSkeletalMeshUpdateContext
{
	USkeletalMesh*				SkeletalMesh;
	TArray<UActorComponent*>	AssociatedComponents;

	FExecuteAction				OnLODChanged;
};

//////////////////////////////////////////////////////////////////////////
// FLODUtilities

class UNREALED_API FLODUtilities
{
public:
	/** Removes a particular LOD from the SkeletalMesh. 
	*
	* @param UpdateContext - The skeletal mesh and actor components to operate on.
	* @param DesiredLOD   - The LOD index to remove the LOD from.
	*/
	static void RemoveLOD( FSkeletalMeshUpdateContext& UpdateContext, int32 DesiredLOD );

	/**
	* Import a .FBX file as an LOD of the SkeletalMesh.
	*
	* @param UpdateContext - The skeletal mesh and actor components to operate on.
	* @param DesiredLOD   - the LOD index to import into. A new LOD entry is created if one doesn't exist
	*/
	static void ImportMeshLOD( FSkeletalMeshUpdateContext& UpdateContext, int32 DesiredLOD);

	/**
	* Import the temporary skeletal mesh into the specified LOD of the destination skeletal mesh
	*
	* @param UpdateContext - The skeletal mesh and actor components to operate on.
	* @param InputSkeletalMesh - newly created mesh used as LOD
	* @param DesiredLOD - the LOD index to import into. A new LOD entry is created if one doesn't exist
	* @return If true, import succeeded
	*/
	static bool ImportMeshLOD(FSkeletalMeshUpdateContext& UpdateContext, USkeletalMesh* InputSkeletalMesh, int32 DesiredLOD);
	
	/**
	 *	Simplifies the static mesh based upon various user settings.
	 *
	 * @param UpdateContext - The skeletal mesh and actor components to operate on.
	 * @param InSettings   - The user settings passed from the Simplification tool.
	 */
	static void SimplifySkeletalMesh( FSkeletalMeshUpdateContext& UpdateContext, TArray<FSkeletalMeshOptimizationSettings> &InSettings );

	/**
	 * Refresh LOD Change
	 * 
	 * LOD has changed, it will have to notify all SMC that uses this SM
	 * and ask them to refresh LOD
	 *
	 * @param	InSkeletalMesh	SkeletalMesh that LOD has been changed for
	 */
	static void RefreshLODChange(const USkeletalMesh * SkeletalMesh);

	/**
	 * Create LOD Settings Dialog
	 * 
	 * Spawns a Slate dialog for editing LOD settings on a USkeletalMesh
	 *
	 * @param	UpdateContext	Context to be acted upon
	 */
	static void CreateLODSettingsDialog(FSkeletalMeshUpdateContext& UpdateContext);

private:
	FLODUtilities() {}
};
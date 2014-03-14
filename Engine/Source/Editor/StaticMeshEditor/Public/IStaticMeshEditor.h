// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"

/**
 * Public interface to Static Mesh Editor
 */
class IStaticMeshEditor : public FAssetEditorToolkit
{
public:
	/** Called after an undo is performed to give child widgets a chance to refresh. */
	DECLARE_MULTICAST_DELEGATE( FOnPostUndoMulticaster );

	// Post undo 
	typedef FOnPostUndoMulticaster::FDelegate FOnPostUndo;

	/** Retrieves the current static mesh displayed in the Static Mesh Editor. */
	virtual UStaticMesh* GetStaticMesh() = 0;

	/** Retrieves the static mesh component. */
	virtual UStaticMeshComponent* GetStaticMeshComponent() const = 0;

	/** Retrieves the currently selected socket from the Socket Manager. */
	virtual UStaticMeshSocket* GetSelectedSocket() const = 0;

	/** 
	 *	Set the currently selected socket in the Socket Manager. 
	 *
	 *	@param	InSelectedSocket			The selected socket to pass on to the Socket Manager.
	 */
	virtual void SetSelectedSocket(UStaticMeshSocket* InSelectedSocket) = 0;

	/** Duplicate the selected socket */
	virtual void DuplicateSelectedSocket() = 0;

	/** Requests to rename selected socket */
	virtual void RequestRenameSelectedSocket() = 0;

	/** 	Retrieves the number of triangles in the current static mesh or it's forced LOD. 
	 *
	 *  @param  LODLevel			The desired LOD to retrieve the number of triangles for.
	 *	@returns					The number of triangles for the specified LOD level.
	 */
	virtual int32 GetNumTriangles(int32 LODLevel = 0) const = 0;

	/** Retrieves the number of vertices in the current static mesh or it's forced LOD. 
	 *
	 *  @param  LODLevel			The desired LOD to retrieve the number of vertices for.	 
	 *	@returns					The number of vertices for the specified LOD level.
	 */
	virtual int32 GetNumVertices(int32 LODLevel = 0) const = 0;

	/** Retrieves the number of UV channels available.
	 *
	 *  @param  LODLevel			The desired LOD to retrieve the number of UV channels for.
	 *	@returns					The number of triangles for the specified LOD level.
	 */
	virtual int32 GetNumUVChannels(int32 LODLevel = 0) const = 0;

	/** Retrieves the currently selected UV channel. */
	virtual int32 GetCurrentUVChannel() = 0;

	/** Retrieves the current LOD level. 0 is auto, 1 is base. */
	virtual int32 GetCurrentLODLevel() = 0;

	/** Retrieves the current LOD index */
	virtual int32 GetCurrentLODIndex() = 0;

	/** Refreshes the Static Mesh Editor's viewport. */
	virtual void RefreshViewport() = 0;

	/** Refreshes everything in the Static Mesh Editor. */
	virtual void RefreshTool() = 0;

	/** 
	 *	This is called when Apply is pressed in the dialog. Does the actual processing.
	 *
	 *	@param	InMaxHullCount			The max hull count allowed. 
	 *	@param	InMaxHullVerts			The max number of verts per hull allowed. 
	 */
	virtual void DoDecomp(int32 InMaxHullCount, int32 InMaxHullVerts) = 0;

	/** Retrieves the selected edge set. */
	virtual TSet< int32 >& GetSelectedEdges() = 0;

	/** Registers a delegate to be called after an Undo operation */
	virtual void RegisterOnPostUndo( const FOnPostUndo& Delegate ) = 0;

	/** Unregisters a delegate to be called after an Undo operation */
	virtual void UnregisterOnPostUndo( SWidget* Widget ) = 0;
};



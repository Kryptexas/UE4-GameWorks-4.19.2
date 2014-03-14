// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	MeshPaintEdMode.h: Mesh paint tool
================================================================================*/


#ifndef __MeshPaintEdMode_h__
#define __MeshPaintEdMode_h__

#pragma once

#include "GenericOctree.h"


/** Mesh paint resource types */
namespace EMeshPaintResource
{
	enum Type
	{
		/** Editing vertex colors */
		VertexColors,

		/** Editing textures */
		Texture
	};
}



/** Mesh paint mode */
namespace EMeshPaintMode
{
	enum Type
	{
		/** Painting colors directly */
		PaintColors,

		/** Painting texture blend weights */
		PaintWeights
	};
}



/** Vertex paint target */
namespace EMeshVertexPaintTarget
{
	enum Type
	{
		/** Paint the static mesh component instance in the level */
		ComponentInstance,

		/** Paint the actual static mesh asset */
		Mesh
	};
}




/** Mesh paint color view modes (somewhat maps to EVertexColorViewMode engine enum.) */
namespace EMeshPaintColorViewMode
{
	enum Type
	{
		/** Normal view mode (vertex color visualization off) */
		Normal,

		/** RGB only */
		RGB,
		
		/** Alpha only */
		Alpha,

		/** Red only */
		Red,

		/** Green only */
		Green,

		/** Blue only */
		Blue,
	};
}


/**
 * Mesh Paint settings
 */
class FMeshPaintSettings
{

public:

	/** Static: Returns global mesh paint settings */
	static FMeshPaintSettings& Get()
	{
		return StaticMeshPaintSettings;
	}


protected:

	/** Static: Global mesh paint settings */
	static FMeshPaintSettings StaticMeshPaintSettings;



public:

	/** Constructor */
	FMeshPaintSettings()
		: BrushRadius( 32.0f ),
		  BrushFalloffAmount( 1.0f ),
		  BrushStrength( 0.2f ),
		  bEnableFlow( true ),
		  FlowAmount( 1.0f ),
		  bOnlyFrontFacingTriangles( true ),
		  ResourceType( EMeshPaintResource::VertexColors ),
		  PaintMode( EMeshPaintMode::PaintColors ),
		  PaintColor( 1.0f, 1.0f, 1.0f, 1.0f ),
		  EraseColor( 0.0f, 0.0f, 0.0f, 0.0f ),
		  bWriteRed( true ),
		  bWriteGreen( true ),
		  bWriteBlue( true ),
		  bWriteAlpha( false ),		// NOTE: Don't write alpha by default
		  TotalWeightCount( 2 ),
		  PaintWeightIndex( 0 ),
		  EraseWeightIndex( 1 ),
		  VertexPaintTarget( EMeshVertexPaintTarget::ComponentInstance ),
		  UVChannel( 0 ),
		  ColorViewMode( EMeshPaintColorViewMode::Normal ),
		  bEnableSeamPainting( true )
	{
	}


public:

	/** Radius of the brush (world space units) */
	float BrushRadius;

	/** Amount of falloff to apply (0.0 - 1.0) */
	float BrushFalloffAmount;

	/** Strength of the brush (0.0 - 1.0) */
	float BrushStrength;

	/** Enables "Flow" painting where paint is continually applied from the brush every tick */
	bool bEnableFlow;

	/** When "Flow" is enabled, this scale is applied to the brush strength for paint applied every tick (0.0-1.0) */
	float FlowAmount;

	/** Whether back-facing triangles should be ignored */
	bool bOnlyFrontFacingTriangles;

	/** Resource type we're editing */
	EMeshPaintResource::Type ResourceType;


	/** Mode we're painting in.  If this is set to PaintColors we're painting colors directly; if it's set
	    to PaintWeights we're painting texture blend weights using a single channel. */
	EMeshPaintMode::Type PaintMode;


	/** Colors Mode: Paint color */
	FLinearColor PaintColor;

	/** Colors Mode: Erase color */
	FLinearColor EraseColor;

	/** Colors Mode: True if red colors values should be written */
	bool bWriteRed;

	/** Colors Mode: True if green colors values should be written */
	bool bWriteGreen;

	/** Colors Mode: True if blue colors values should be written */
	bool bWriteBlue;

	/** Colors Mode: True if alpha colors values should be written */
	bool bWriteAlpha;


	/** Weights Mode: Total weight count */
	int32 TotalWeightCount;

	/** Weights Mode: Weight index that we're currently painting */
	int32 PaintWeightIndex;

	/** Weights Mode: Weight index that we're currently using to erase with */
	int32 EraseWeightIndex;


	/**
	 * Vertex paint settings
	 */
	
	/** Vertex paint target */
	EMeshVertexPaintTarget::Type VertexPaintTarget;


	/**
	 * Texture paint settings
	 */
	
	/** UV channel to paint textures using */
	int32 UVChannel;


	/**
	 * View settings
	 */

	/** Color visualization mode */
	EMeshPaintColorViewMode::Type ColorViewMode;

	/** Seam painting flag, True if we should enable dilation to allow the painting of texture seams */
	bool bEnableSeamPainting; 
};



/** Mesh painting action (paint, erase) */
namespace EMeshPaintAction
{
	enum Type
	{
		/** Paint (add color or increase blending weight) */
		Paint,

		/** Erase (remove color or decrease blending weight) */
		Erase,

		/** Fill with the active paint color */
		Fill,

		/** Push instance colors to mesh color */
		PushInstanceColorsToMesh
	};
}



namespace MeshPaintDefs
{
	// Design constraints

	// Currently we never support more than five channels (R, G, B, A, OneMinusTotal)
	static const int32 MaxSupportedPhysicalWeights = 4;
	static const int32 MaxSupportedWeights = MaxSupportedPhysicalWeights + 1;
}


/**
 *  Wrapper to expose texture targets to WPF code.
 */
struct FTextureTargetListInfo
{
	UTexture2D* TextureData;
	bool bIsSelected;
	uint32 UndoCount;
	uint32 UVChannelIndex;
	FTextureTargetListInfo(UTexture2D* InTextureData, int32 InUVChannelIndex, bool InbIsSelected = false)
		:	TextureData(InTextureData)
		,	UVChannelIndex(InUVChannelIndex)
		,	bIsSelected(InbIsSelected)
		,	UndoCount(0)
	{}
};


/**
 * Mesh Paint editor mode
 */
class FEdModeMeshPaint : public FEdMode
{
private:
	/** struct used to store the color data copied from mesh instance to mesh instance */
	struct FPerLODVertexColorData
	{
		TArray< FColor > ColorsByIndex;
		TMap<FVector, FColor> ColorsByPosition;
	};

public:

	struct PaintTexture2DData
	{
		/** The original texture that we're painting */
		UTexture2D* PaintingTexture2D;
		bool bIsPaintingTexture2DModified;

		/** A copy of the original texture we're painting, used for restoration. */
		UTexture2D* PaintingTexture2DDuplicate;

		/** Render target texture for painting */
		UTextureRenderTarget2D* PaintRenderTargetTexture;

		/** Render target texture used as an input while painting that contains a clone of the original image */
		UTextureRenderTarget2D* CloneRenderTargetTexture;

		/** List of materials we are painting on */
		TArray< UMaterialInterface* > PaintingMaterials;

		/** Default ctor */
		PaintTexture2DData() :
			PaintingTexture2D( NULL ),
			PaintingTexture2DDuplicate ( NULL ),
			bIsPaintingTexture2DModified( false ),
			PaintRenderTargetTexture( NULL ),
			CloneRenderTargetTexture( NULL )
		{}

		PaintTexture2DData( UTexture2D* InPaintingTexture2D, bool InbIsPaintingTexture2DModified = false ) :
			PaintingTexture2D( InPaintingTexture2D ),
			bIsPaintingTexture2DModified( InbIsPaintingTexture2DModified ),
			PaintRenderTargetTexture( NULL ),
			CloneRenderTargetTexture( NULL )
		{}

		/** Serializer */
		void AddReferencedObjects( FReferenceCollector& Collector )
		{
			// @todo MeshPaint: We're relying on GC to clean up render targets, can we free up remote memory more quickly?
			Collector.AddReferencedObject( PaintingTexture2D );
			Collector.AddReferencedObject( PaintRenderTargetTexture );
			Collector.AddReferencedObject( CloneRenderTargetTexture );
			for( int32 Index = 0; Index < PaintingMaterials.Num(); Index++ )
			{
				Collector.AddReferencedObject( PaintingMaterials[ Index ] );
			}
		}
	};

	struct FTexturePaintTriangleInfo
	{
		FVector TriVertices[ 3 ];
		FVector2D TrianglePoints[ 3 ];
		FVector2D TriUVs[ 3 ];
	};

	/** Constructor */
	FEdModeMeshPaint();

	/** Destructor */
	virtual ~FEdModeMeshPaint();

	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	// FEdMode interface
	virtual bool UsesToolkits() const OVERRIDE;
	virtual void Enter() OVERRIDE;
	virtual void Exit() OVERRIDE;
	virtual bool MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y ) OVERRIDE;
	virtual bool CapturedMouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY ) OVERRIDE;
	virtual bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) OVERRIDE;
	virtual bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) OVERRIDE;
	virtual bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) OVERRIDE;
	virtual bool InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale ) OVERRIDE;
	virtual void PostUndo() OVERRIDE;
	virtual void Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI ) OVERRIDE;
	virtual bool Select( AActor* InActor, bool bInSelected ) OVERRIDE;
	virtual bool IsSelectionAllowed( AActor* InActor, bool bInSelection ) const OVERRIDE;
	virtual void ActorSelectionChangeNotify() OVERRIDE;
	virtual bool AllowWidgetMove() OVERRIDE { return false; }
	virtual bool ShouldDrawWidget() const OVERRIDE { return false; }
	virtual bool UsesTransformWidget() const OVERRIDE { return false; }
	virtual void Tick( FLevelEditorViewportClient* ViewportClient, float DeltaTime ) OVERRIDE;
	// End of FEdMode interface


	/** Returns true if we need to force a render/update through based fill/copy */
	bool IsForceRendered (void) const;

	/** Saves out cached mesh settings for the given actor */
	void SaveSettingsForActor( AActor* InActor );

	/** Updates static Mesh with settings for updated textures*/
	void UpdateSettingsForStaticMeshComponent( UStaticMeshComponent* InStaticMeshComponent, UTexture2D* InOldTexture, UTexture2D* InNewTexture );

	/** Helper function to get the current paint action for use in DoPaint */
	EMeshPaintAction::Type GetPaintAction(FViewport* InViewport);

	/** Removes vertex colors associated with the object (if it's a StaticMeshActor) */
	void RemoveInstanceVertexColors(UObject* Obj) const;

	/** Removes vertex colors associated with the static mesh component */
	void RemoveComponentInstanceVertexColors(UStaticMeshComponent* StaticMeshComponent) const;

	/** Removes vertex colors associated with the currently selected mesh */
	void RemoveInstanceVertexColors() const;

	/** Copies vertex colors associated with the currently selected mesh */
	void CopyInstanceVertexColors();

	/** Pastes vertex colors to the currently selected mesh */
	void PasteInstanceVertexColors();

	/** Returns whether the instance vertex colors associated with the currently selected mesh need to be fixed up or not */
	bool RequiresInstanceVertexColorsFixup() const;

	/** Attempts to fix up the instance vertex colors associated with the currently selected mesh, if necessary */
	void FixupInstanceVertexColors() const;

	/** Applies vertex color painting on LOD0 to all lower LODs. */
	void ApplyVertexColorsToAllLODs();

	/** Forces all selected meshes to their best LOD. */
	void ForceBestLOD();

	/** Forces the passed static mesh component to it's best LOD. */
	void ForceBestLOD(UStaticMeshComponent* StaticMeshComponent);

	/** Stops forcing selected meshes to their best lod. */
	void ClearForcedLOD();

	/** Stops the passed static mesh component to it's best lod. */
	void ClearForcedLOD(UStaticMeshComponent* StaticMeshComponent);

	/** Applies vertex color painting on LOD0 to all lower LODs. */
	void ApplyVertexColorsToAllLODs(UStaticMeshComponent* StaticMeshComponent);

	/** Fills the vertex colors associated with the currently selected mesh*/
	void FillInstanceVertexColors();

	/** Pushes instance vertex colors to the  mesh*/
	void PushInstanceVertexColorsToMesh();

	/** Creates a paintable material/texture for the selected mesh */
	void CreateInstanceMaterialAndTexture() const;

	/** Removes instance of paintable material/texture for the selected mesh */
	void RemoveInstanceMaterialAndTexture() const;

	/** Returns information about the currently selected mesh */
	bool GetSelectedMeshInfo( int32& OutTotalBaseVertexColorBytes, int32& OutTotalInstanceVertexColorBytes, bool& bOutHasInstanceMaterialAndTexture ) const;

	/** Returns the min and max brush radius sizes specified in the user config */
	void GetBrushRadiiSliderLimits( float& OutMinBrushSliderRadius, float& OutMaxBrushSliderRadius ) const;

	/** Returns the min and max brush radius sizes that the user is allowed to type in */
	void GetBrushRadiiLimits( float& OutMinBrushRadius, float& OutMaxBrushRadius ) const;

	/** Returns whether there are colors in the copy buffer */
	bool CanPasteVertexColors() const;

	/** 
	 * Will update the list of available texture paint targets based on selection 
	 */
	void UpdateTexturePaintTargetList();

	/** Will update the list of available texture paint targets based on selection */
	int32 GetMaxNumUVSets() const;

	/** Will return the list of available texture paint targets */
	TArray<FTextureTargetListInfo>* GetTexturePaintTargetList();

	/** Will return the selected target paint texture if there is one. */
	UTexture2D* GetSelectedTexture();

	void SetSelectedTexture( const UTexture2D* Texture );

	/** will find the currently selected paint target texture in the content browser */
	void FindSelectedTextureInContentBrowser();

	/** 
	 * Used to commit all paint changes to corresponding target textures. 
	 * @param	bShouldTriggerUIRefresh		Flag to trigger a UI Refresh if any textures have been modified (defaults to true)
	 */	
	void CommitAllPaintedTextures();

	/** Clears all texture overrides for this static mesh. */
	void ClearStaticMeshTextureOverrides(UStaticMeshComponent* InStaticMeshComponent);

	/** Clears all texture overrides, removing any pending texture paint changes */
	void ClearAllTextureOverrides();

	void SetAllTextureOverrides(UStaticMeshComponent* InStaticMeshComponent);

	void SetSpecificTextureOverrideForMesh(UStaticMeshComponent* InStaticMeshComponent, UTexture* Texture);

	/** Used to tell the texture paint system that we will need to restore the rendertargets */
	void RestoreRenderTargets();

	/** Returns the number of texture that require a commit. */
	int32 GetNumberOfPendingPaintChanges();

	/** Returns index of the currently selected Texture Target */
	int32 GetCurrentTextureTargetIndex() const;

	bool UpdateTextureList() { return bShouldUpdateTextureList; };

	/** Duplicates the currently selected texture and material, relinking them together and assigning them to the selected meshes. */
	void DuplicateTextureMaterialCombo();

	/** Will create a new texture. */
	void CreateNewTexture();

private:

	/** Struct to hold MeshPaint settings on a per mesh basis */
	struct StaticMeshSettings
	{
		UTexture2D* SelectedTexture;
		int32 SelectedUVChannel;

		StaticMeshSettings( )
			:	SelectedTexture(NULL)
			,	SelectedUVChannel(0)
		{}
		StaticMeshSettings( UTexture2D* InSelectedTexture, int32 InSelectedUVSet )
			:	SelectedTexture(InSelectedTexture)
			,	SelectedUVChannel(InSelectedUVSet)
		{}

		void operator=(const StaticMeshSettings& SrcSettings)
		{
			SelectedTexture = SrcSettings.SelectedTexture;
			SelectedUVChannel = SrcSettings.SelectedUVChannel;
		}
	};

	/** Triangle for use in Octree for mesh paint optimisation */
	struct FMeshTriangle
	{
		uint32 Index;
		FVector Vertices[3];
		FBoxCenterAndExtent BoxCenterAndExtent;
	};

	/** Semantics for the simple mesh paint octree */
	struct FMeshTriangleOctreeSemantics
	{
		enum { MaxElementsPerLeaf = 16 };
		enum { MinInclusiveElementsPerNode = 7 };
		enum { MaxNodeDepth = 12 };

		typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

		/**
		 * Get the bounding box of the provided octree element. In this case, the box
		 * is merely the point specified by the element.
		 *
		 * @param	Element	Octree element to get the bounding box for
		 *
		 * @return	Bounding box of the provided octree element
		 */
		FORCEINLINE static FBoxCenterAndExtent GetBoundingBox( const FMeshTriangle& Element )
		{
			return Element.BoxCenterAndExtent;
		}

		/**
		 * Determine if two octree elements are equal
		 *
		 * @param	A	First octree element to check
		 * @param	B	Second octree element to check
		 *
		 * @return	true if both octree elements are equal, false if they are not
		 */
		FORCEINLINE static bool AreElementsEqual( const FMeshTriangle& A, const FMeshTriangle& B )
		{
			return ( A.Index == B.Index );
		}

		/** Ignored for this implementation */
		FORCEINLINE static void SetElementId( const FMeshTriangle& Element, FOctreeElementId Id )
		{
		}
	};
	typedef TOctree<FMeshTriangle, FMeshTriangleOctreeSemantics> FMeshTriOctree;

	/** Static: Determines if a world space point is influenced by the brush and reports metrics if so */
	static bool IsPointInfluencedByBrush( const FVector& InPosition,
										   const class FMeshPaintParameters& InParams,
										   float& OutSquaredDistanceToVertex2D,
										   float& OutVertexDepthToBrush );

	/** Static: Paints the specified vertex!  Returns true if the vertex was in range. */
	static bool PaintVertex( const FVector& InVertexPosition,
							  const class FMeshPaintParameters& InParams,
							  const bool bIsPainting,
							  FColor& InOutVertexColor );

	/** Paint the mesh that impacts the specified ray */
	void DoPaint( const FVector& InCameraOrigin,
				  const FVector& InRayOrigin,
				  const FVector& InRayDirection,
				  FPrimitiveDrawInterface* PDI,
				  const EMeshPaintAction::Type InPaintAction,
				  const bool bVisualCueOnly,
				  const float InStrengthScale,
				  OUT bool& bAnyPaintAbleActorsUnderCursor);

	/** Paints mesh vertices */
	void PaintMeshVertices( UStaticMeshComponent* StaticMeshComponent, const FMeshPaintParameters& Params, const bool bShouldApplyPaint, FStaticMeshLODResources& LODModel, const FVector& ActorSpaceCameraPosition, const FMatrix& ActorToWorldMatrix, FPrimitiveDrawInterface* PDI, const float VisualBiasDistance );

	/** Paints mesh texture */
	void PaintMeshTexture( UStaticMeshComponent* StaticMeshComponent, const FMeshPaintParameters& Params, const bool bShouldApplyPaint, FStaticMeshLODResources& LODModel, const FVector& ActorSpaceCameraPosition, const FMatrix& ActorToWorldMatrix, const float ActorSpaceSquaredBrushRadius, const FVector& ActorSpaceBrushPosition );
	
	/** Forces real-time perspective viewports */
	void ForceRealTimeViewports( const bool bEnable, const bool bStoreCurrentState );

	/** Sets show flags for perspective viewports */
	void SetViewportShowFlags( const bool bAllowColorViewModes, FLevelEditorViewportClient& Viewport );

	/** Starts painting a texture */
	void StartPaintingTexture( UStaticMeshComponent* InStaticMeshComponent );

	/** Paints on a texture */
	void PaintTexture( const FMeshPaintParameters& InParams,
					   const TArray< int32 >& InInfluencedTriangles,
					   const FMatrix& InActorToWorldMatrix );

	/** Finishes painting a texture */
	void FinishPaintingTexture();

	/** Makes sure that the render target is ready to paint on */
	void SetupInitialRenderTargetData( UTexture2D* InTextureSource, UTextureRenderTarget2D* InRenderTarget );

	/** Static: Copies a texture to a render target texture */
	static void CopyTextureToRenderTargetTexture( UTexture* SourceTexture, UTextureRenderTarget2D* RenderTargetTexture );

	/** Will generate a mask texture, used for texture dilation, and store it in the passed in rendertarget */
	bool GenerateSeamMask(UStaticMeshComponent* StaticMeshComponent, int32 UVSet, UTextureRenderTarget2D* RenderTargetTexture);

	/** Static: Creates a temporary texture used to transfer data to a render target in memory */
	UTexture2D* CreateTempUncompressedTexture( UTexture2D* SourceTexture );

	/** Used when we want to change selection to the next available paint target texture.  Will stop at the end of the list and will NOT cycle to the beginning of the list. */
	void SelectNextTexture() { ShiftSelectedTexture( true, false ); }

	/** Used to change the selected texture paint target texture. Will stop at the beginning of the list and will NOT cycle to the end of the list. */
	void SelectPrevTexture() { ShiftSelectedTexture( false, false); }

	/**
	 * Used to change the currently selected paint target texture.
	 *
	 * @param	bToTheRight 	True if a shift to next texture is desired, false if a shift to the previous texture is desired.
	 * @param	bCycle		 	If set to False, this function will stop at the first or final element.  If true the selection will cycle to the opposite end of the list.
	 */
	void ShiftSelectedTexture( bool bToTheRight = true, bool bCycle = true );

	
	/**
	 * Used to get a reference to data entry associated with the texture.  Will create a new entry if one is not found.
	 *
	 * @param	inTexture 		The texture we want to retrieve data for.
	 * @return					Returns a reference to the paint data associated with the texture.  This reference
	 *								is only valid until the next change to any key in the map.  Will return NULL only when inTexture is NULL.
	 */
	PaintTexture2DData* GetPaintTargetData(  UTexture2D* inTexture );

	/**
	 * Used to add an entry to to our paint target data.
	 *
	 * @param	inTexture 		The texture we want to create data for.
	 * @return					Returns a reference to the newly created entry.  If an entry for the input texture already exists it will be returned instead.
	 *								Will return NULL only when inTexture is NULL.   This reference is only valid until the next change to any key in the map.
	 *								 
	 */
	PaintTexture2DData* AddPaintTargetData(  UTexture2D* inTexture );

	/**
	 * Used to get the original texture that was overridden with a render target texture.
	 *
	 * @param	inTexture 		The render target that was used to override the original texture.
	 * @return					Returns a reference to texture that was overridden with the input render target texture.  Returns NULL if we don't find anything.
	 *								 
	 */
	UTexture2D* GetOriginalTextureFromRenderTarget( UTextureRenderTarget2D* inTexture );

	/**
	* Ends the outstanding transaction, if one exists.
	*/
	void EndTransaction();

	/**
	* Begins a new transaction, if no outstanding transaction exists.
	*/
	void BeginTransaction(const FText& Description);

	/** Helper function to start painting, resets timer etc. */
	void StartPainting();

	/** Helper function to end painting, finish painting textures & update transactions */
	void EndPainting();

private:

	/** Whether we're currently painting */
	bool bIsPainting;

	/** Whether we are doing a flood fill the next render */
	bool bIsFloodFill;

	/** Whether we are pushing the instance colors to the mesh next render */
	bool bPushInstanceColorsToMesh;

	/** Will store the state of selection locks on start of paint mode so that it can be restored on close */
	bool bWasSelectionLockedOnStart;

	/** True when the actor selection has changed since we last updated the TexturePaint list, used to avoid reloading the same textures */
	bool bShouldUpdateTextureList;

	/** True if we need to generate a texture seam mask used for texture dilation */
	bool bGenerateSeamMask;

	/** Real time that we started painting */
	double PaintingStartTime;

	/** Array of static meshes that we've modified */
	TArray< UStaticMesh* > ModifiedStaticMeshes;

	/** A mapping of static meshes to body setups that were built for static meshes */
	TMap<TWeakObjectPtr<UStaticMesh>, TWeakObjectPtr<UBodySetup>> StaticMeshToTempBodySetup;

	/** Which mesh LOD index we're painting */
	// @todo MeshPaint: Allow painting on other LODs?
	static const uint32 PaintingMeshLODIndex = 0;

	/** Texture paint: The static mesh components that we're currently painting */
	UStaticMeshComponent* TexturePaintingStaticMeshComponent;

	/** Texture paint: An octree for the current static mesh & LOD to speed up triangle selection */
	FMeshTriOctree* TexturePaintingStaticMeshOctree;

	/** Texture paint: The LOD being used for texture painting. */
	int32 TexturePaintingStaticMeshLOD;

	/** The original texture that we're painting */
	UTexture2D* PaintingTexture2D;

	/** Stores data associated with our paint target textures */
	TMap< UTexture2D*, PaintTexture2DData > PaintTargetData;

	/** Temporary buffers used when copying/pasting colors */
	TArray< FPerLODVertexColorData > CopiedColorsByLOD;

	/**
	 * Does the work of removing instance vertex colors from a single static mesh component.
	 *
	 * @param	StaticMeshComponent		The SMC to remove vertex colors from.
	 * @param	InstanceMeshLODInfo		The instance's LODInfo which stores the painted information to be cleared.
	 */
	void RemoveInstanceVertexColorsWorker(UStaticMeshComponent *StaticMeshComponent, FStaticMeshComponentLODInfo *InstanceMeshLODInfo) const;

	/** Texture paint: Will hold a list of texture items that we can paint on */
	TArray<FTextureTargetListInfo> TexturePaintTargetList;

	/** Map of settings for each StaticMeshComponent */
	TMap< UStaticMeshComponent*, StaticMeshSettings > StaticMeshSettingsMap;

	/** Used to store a flag that will tell the tick function to restore data to our rendertargets after they have been invalidated by a viewport resize. */
	bool bDoRestoreRenTargets;

	/** Temporary rendertarget used to draw incremental paint to */
	UTextureRenderTarget2D* BrushRenderTargetTexture;
	
	/** Temporary rendertarget used to store a mask of the affected paint region, updated every time we add incremental texture paint */ 
	UTextureRenderTarget2D* BrushMaskRenderTargetTexture;
	
	/** Temporary rendertarget used to store generated mask for texture seams, we create this by projecting object triangles into texture space using the selected UV channel */
	UTextureRenderTarget2D* SeamMaskRenderTargetTexture;

	/** Used to store the transaction to some vertex paint operations that can not be easily scoped. */
	FScopedTransaction* ScopedTransaction;
};



/**
* Imports vertex colors from tga file onto selected meshes
*/
class FImportVertexTextureHelper  
{
public:
	enum ChannelsMask{
		ERed		= 0x1,
		EGreen		= 0x2,
		EBlue		= 0x4,
		EAlpha		= 0x8,
	};

	/**
	* PickVertexColorFromTex() - Color picker function. Retrieves pixel color from coordinates and mask.
	* @param NewVertexColor - returned color
	* @param MipData - Highest mip-map with pixels data
	* @param UV - texture coordinate to read
	* @param Tex - texture info
	* @param ColorMask - mask for filtering which colors to use
	*/
	void PickVertexColorFromTex(FColor & NewVertexColor, uint8* MipData, FVector2D & UV, UTexture2D* Tex, uint8 & ColorMask);
	
	/**
	* Imports Vertex Color data from texture scanning thought uv vertex coordinates for selected actors.  
	* @param Filename - path for loading TGA file
	* @param UVIndex - Coordinate index
	* @param ImportLOD - LOD level to work with
	* @param Tex - texture info
	* @param ColorMask - mask for filtering which colors to use
	*/
	void ImportVertexColors(const FString & Filename, int32 UVIndex, int32 ImportLOD, uint8 ColorMask);

};
#endif	// __MeshPaintEdMode_h__

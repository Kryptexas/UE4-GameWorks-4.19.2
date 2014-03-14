// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LevelEditorViewport.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLandscapeEdMode, Log, All);

// FLandscapeToolMousePosition - Struct to store mouse positions since the last time we applied the brush
struct FLandscapeToolMousePosition
{
	// Stored in heightmap space.
	float PositionX, PositionY;
	bool bShiftDown;

	FLandscapeToolMousePosition( float InPositionX, float InPositionY, bool InbShiftDown )
		:	PositionX(InPositionX)
		,	PositionY(InPositionY)
		,	bShiftDown(InbShiftDown)
	{}
};

class FLandscapeBrush : public FGCObject
{
public:
	enum EBrushType
	{
		BT_Normal = 0,
		BT_Alpha,
		BT_Component,
		BT_Gizmo,
		BT_Splines
	};
	
	virtual void MouseMove( float LandscapeX, float LandscapeY ) = 0;
	virtual bool ApplyBrush( const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& OutX1, int32& OutY1, int32& OutX2, int32& OutY2 ) =0;
	virtual bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) { return false; }
	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) {};
	virtual void BeginStroke(float LandscapeX, float LandscapeY, class FLandscapeTool* CurrentTool);
	virtual void EndStroke();
	virtual void EnterBrush() {}
	virtual void LeaveBrush() {}
	virtual ~FLandscapeBrush() {}
	virtual UMaterialInterface* GetBrushMaterial() { return NULL; }
	virtual const TCHAR* GetBrushName() = 0;
	virtual FText GetDisplayName() = 0;
	virtual EBrushType GetBrushType() { return BT_Normal; }
	
	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE {}
};

struct FLandscapeBrushSet
{
	FLandscapeBrushSet(const TCHAR* InBrushSetName)
		: BrushSetName(InBrushSetName)
		, PreviousBrushIndex(0)
	{
	}

	const FName BrushSetName;
	TArray<FLandscapeBrush*> Brushes;
	int32 PreviousBrushIndex;

	virtual ~FLandscapeBrushSet()
	{
		for( int32 BrushIdx=0;BrushIdx<Brushes.Num();BrushIdx++ )
		{
			delete Brushes[BrushIdx];
		}
	}
};

namespace ELandscapeToolTargetType
{
	enum Type
	{
		Heightmap	= 0,
		Weightmap	= 1,
		Visibility	= 2,

		Invalid		= -1, // only valid for LandscapeEdMode->CurrentToolTarget.TargetType
	};
}

namespace ELandscapeToolTargetTypeMask
{
	enum Type
	{
		Heightmap	= 1<<ELandscapeToolTargetType::Heightmap,
		Weightmap	= 1<<ELandscapeToolTargetType::Weightmap,
		Visibility	= 1<<ELandscapeToolTargetType::Visibility,

		NA			= 0,
		All			= 0xFF,
	};
}

namespace ELandscapeToolNoiseMode
{
	enum Type
	{
		Invalid = -1,
		Both = 0,
		Add = 1,
		Sub = 2,
	};

	inline float Conversion(Type Mode, float NoiseAmount, float OriginalValue)
	{
		switch( Mode )
		{
		case Add: // always +
			OriginalValue += NoiseAmount;
			break;
		case Sub: // always -
			OriginalValue -= NoiseAmount;
			break;
		case Both:
			break;
		}
		return OriginalValue;
	}
}

struct FLandscapeToolTarget
{
	TWeakObjectPtr<ULandscapeInfo> LandscapeInfo;
	ELandscapeToolTargetType::Type TargetType;
	TWeakObjectPtr<ULandscapeLayerInfoObject> LayerInfo;
	FName LayerName;
	
	FLandscapeToolTarget()
		: LandscapeInfo(NULL)
		, TargetType(ELandscapeToolTargetType::Heightmap)
		, LayerInfo(NULL)
		, LayerName(NAME_None)
	{
	}
};

/**
 * FLandscapeTool
 */
class FLandscapeTool
{
public:
	enum EToolType
	{
		TT_Normal = 0,
		TT_Mask,
	};
	virtual void EnterTool() {}
	virtual void ExitTool() {}
	virtual bool IsValidForTarget(const FLandscapeToolTarget& Target) = 0;
	virtual bool BeginTool( FLevelEditorViewportClient* ViewportClient, const FLandscapeToolTarget& Target, const FVector& InHitLocation ) = 0;
	virtual void EndTool(FLevelEditorViewportClient* ViewportClient) = 0;
	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) {};
	virtual bool MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y ) = 0;
	virtual bool HandleClick( HHitProxy* HitProxy, const FViewportClick& Click ) { return false; }
	virtual bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) { return false; }
	virtual bool InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale ) { return false; }
	virtual ~FLandscapeTool() {}
	virtual const TCHAR* GetToolName() = 0;
	virtual FText GetDisplayName() = 0;
	virtual void SetEditRenderType();
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) {}
	virtual bool SupportsMask() { return true; }
	virtual bool SupportsComponentSelection() { return false; }
	virtual bool OverrideSelection() const { return false; }
	virtual bool IsSelectionAllowed( AActor* InActor, bool bInSelection ) const { return false; }
	virtual bool UsesTransformWidget() const { return false; }
	virtual EAxisList::Type GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const { return EAxisList::All; }
	virtual FVector GetWidgetLocation() const { return FVector::ZeroVector; }
	virtual FMatrix GetWidgetRotation() const { return FMatrix::Identity; }
	virtual bool DisallowMouseDeltaTracking() const { return false; }

	virtual EEditAction::Type GetActionEditDuplicate() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditDelete() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditCut() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditCopy() { return EEditAction::Skip; }
	virtual EEditAction::Type GetActionEditPaste() { return EEditAction::Skip; }
	virtual bool ProcessEditDuplicate() { return false; }
	virtual bool ProcessEditDelete() { return false; }
	virtual bool ProcessEditCut() { return false; }
	virtual bool ProcessEditCopy() { return false; }
	virtual bool ProcessEditPaste() { return false; }

	// Functions which doesn't need Viewport data...
	virtual void Process(int32 Index, int32 Arg) {}
	virtual EToolType GetToolType() { return TT_Normal; }
};

class FLandscapeToolSet
{
	TArray<FLandscapeTool*> Tools;
	FLandscapeTool*			CurrentTool;
	FName					ToolSetName;
public:
	int32					PreviousBrushIndex;
	TArray<FName>			ValidBrushes;

	FLandscapeToolSet(FName InToolSetName)
	:	ToolSetName(InToolSetName)
	,	CurrentTool(NULL)
	,	PreviousBrushIndex(INDEX_NONE)
	{		
	}

	virtual ~FLandscapeToolSet()
	{
		for( int32 ToolIdx=0;ToolIdx<Tools.Num();ToolIdx++ )
		{
			delete Tools[ToolIdx];
		}
	}

	virtual FText GetDisplayName() { return Tools[0]->GetDisplayName(); }

	void AddTool(FLandscapeTool* InTool)
	{
		Tools.Add(InTool);
	}

	bool SetToolForTarget( const FLandscapeToolTarget& Target )
	{
		for( int32 ToolIdx=0;ToolIdx<Tools.Num();ToolIdx++ )
		{
			if( Tools[ToolIdx]->IsValidForTarget(Target) )
			{
				CurrentTool = Tools[ToolIdx];
				return true;
			}
		}

		return false;
	}

	FLandscapeTool* GetTool()
	{
		return CurrentTool;
	}

	FName GetToolSetName()
	{
		return ToolSetName;
	}

	bool operator==(const FLandscapeToolSet& OtherToolSet) const
	{
		return OtherToolSet.ToolSetName==ToolSetName;
	}

	bool operator==(const FName& OtherName) const
	{
		return OtherName==ToolSetName;
	}
};

struct FLandscapeToolMode
{
	const FName				ToolModeName;
	int32					SupportedTargetTypes; // ELandscapeToolTargetTypeMask::Type

	TArray<FName>			ValidToolSets;
	FName					CurrentToolSetName;

	FLandscapeToolMode(FName InToolModeName, int32 InSupportedTargetTypes)
		: ToolModeName(InToolModeName)
		, SupportedTargetTypes(InSupportedTargetTypes)
	{
	}
};

struct FLandscapeTargetListInfo
{
	FText TargetName;
	ELandscapeToolTargetType::Type TargetType;
	TWeakObjectPtr<ULandscapeInfo> LandscapeInfo;

	//Values cloned from FLandscapeLayerStruct LayerStruct;			// ignored for heightmap
	TWeakObjectPtr<ULandscapeLayerInfoObject> LayerInfoObj;			// ignored for heightmap
	FName LayerName;												// ignored for heightmap
	TWeakObjectPtr<class ALandscapeProxy> Owner;					// ignored for heightmap
	TWeakObjectPtr<class UMaterialInstanceConstant> ThumbnailMIC;	// ignored for heightmap
	int32 DebugColorChannel;										// ignored for heightmap
	uint32 bValid:1;												// ignored for heightmap

	FLandscapeTargetListInfo(FText InTargetName, ELandscapeToolTargetType::Type InTargetType, const FLandscapeInfoLayerSettings& InLayerSettings)
		: TargetName(InTargetName)
		, TargetType(InTargetType)
		, LandscapeInfo(InLayerSettings.Owner->GetLandscapeInfo())
		, LayerInfoObj(InLayerSettings.LayerInfoObj)
		, LayerName(InLayerSettings.LayerName)
		, Owner(InLayerSettings.Owner)
		, ThumbnailMIC(InLayerSettings.ThumbnailMIC)
		, DebugColorChannel(InLayerSettings.DebugColorChannel)
		, bValid(InLayerSettings.bValid)
	{
	}

	FLandscapeTargetListInfo(FText InTargetName, ELandscapeToolTargetType::Type InTargetType, ULandscapeInfo* InLandscapeInfo)
		: TargetName(InTargetName)
		, TargetType(InTargetType)
		, LandscapeInfo(InLandscapeInfo)
		, LayerInfoObj(NULL)
		, LayerName(NAME_None)
		, ThumbnailMIC(NULL)
		, Owner(NULL)
		, bValid(true)
	{
	}

	FLandscapeInfoLayerSettings* GetLandscapeInfoLayerSettings() const
	{
		if (TargetType == ELandscapeToolTargetType::Weightmap)
		{
			int32 Index = INDEX_NONE;
			if (LayerInfoObj.IsValid())
			{
				Index = LandscapeInfo->GetLayerInfoIndex(LayerInfoObj.Get(), Owner.Get());
			}
			else
			{
				Index = LandscapeInfo->GetLayerInfoIndex(LayerName, Owner.Get());
			}
			if (ensure(Index != INDEX_NONE))
			{
				return &LandscapeInfo->Layers[Index];
			}
		}
		return NULL;
	}

	FLandscapeEditorLayerSettings* GetEditorLayerSettings() const
	{
		if (TargetType == ELandscapeToolTargetType::Weightmap)
		{
			check(LayerInfoObj.IsValid());
			ALandscapeProxy* Proxy = LandscapeInfo->GetLandscapeProxy();
			FLandscapeEditorLayerSettings* EditorLayerSettings = Proxy->EditorLayerSettings.FindByKey(LayerInfoObj.Get());
			if (EditorLayerSettings)
			{
				return EditorLayerSettings;
			}
			else
			{
				int32 Index = Proxy->EditorLayerSettings.Add(LayerInfoObj.Get());
				return &Proxy->EditorLayerSettings[Index];
			}
		}
		return NULL;
	}

	FName GetLayerName() const
	{
		return LayerInfoObj.IsValid() ? LayerInfoObj->LayerName : LayerName;
	}

	FString& ReimportFilePath() const
	{
		if (TargetType == ELandscapeToolTargetType::Weightmap)
		{
			FLandscapeEditorLayerSettings* EditorLayerSettings = GetEditorLayerSettings();
			check(EditorLayerSettings);
			return EditorLayerSettings->ReimportLayerFilePath;
		}
		else //if (TargetType == ELandscapeToolTargetType::Heightmap)
		{
			return LandscapeInfo->GetLandscapeProxy()->ReimportHeightmapFilePath;
		}
	}
};

struct FLandscapeListInfo
{
	FString LandscapeName;
	ULandscapeInfo* Info;
	int32 ComponentQuads;
	int32 NumSubsections;
	int32 Width;
	int32 Height;

	FLandscapeListInfo(const TCHAR* InName, ULandscapeInfo* InInfo, int32 InComponentQuads, int32 InNumSubsections, int32 InWidth, int32 InHeight)
		: LandscapeName(InName)
		, Info(InInfo)
		, ComponentQuads(InComponentQuads)
		, NumSubsections(InNumSubsections)
		, Width(InWidth)
		, Height(InHeight)
	{
	}
};

struct FGizmoHistory
{
	ALandscapeGizmoActor* Gizmo;
	FString GizmoName;

	FGizmoHistory(ALandscapeGizmoActor* InGizmo)
		: Gizmo(InGizmo)
	{
		GizmoName = Gizmo->GetPathName();
	}

	FGizmoHistory(ALandscapeGizmoActiveActor* InGizmo)
	{
		// handle for ALandscapeGizmoActiveActor -> ALandscapeGizmoActor
		// ALandscapeGizmoActor is only for history, so it has limited data
		Gizmo = InGizmo->SpawnGizmoActor();
		GizmoName = Gizmo->GetPathName();
	}
};


namespace ELandscapeEdge
{
	enum Type
	{
		None,

		// Edges
		X_Negative,
		X_Positive,
		Y_Negative,
		Y_Positive,

		// Corners
		X_Negative_Y_Negative,
		X_Positive_Y_Negative,
		X_Negative_Y_Positive,
		X_Positive_Y_Positive,
	};
}

namespace ENewLandscapePreviewMode
{
	enum Type
	{
		None,
		NewLandscape,
		ImportLandscape,
	};
}


/**
 * Landscape editor mode
 */
class FEdModeLandscape : public FEdMode
{
public:

	class ULandscapeEditorObject* UISettings;

	FLandscapeToolMode* CurrentToolMode;
	FLandscapeToolSet* CurrentToolSet;
	FLandscapeBrush* CurrentBrush;
	FLandscapeToolTarget CurrentToolTarget;

	// GizmoBrush for Tick
	FLandscapeBrush* GizmoBrush;
	// UI setting for additional UI Tools
	int32 CurrentToolIndex;
	// UI setting for additional UI Tools
	int32 CurrentBrushSetIndex;

	ENewLandscapePreviewMode::Type NewLandscapePreviewMode;
	ELandscapeEdge::Type DraggingEdge;
	float DraggingEdge_Remainder;

	TWeakObjectPtr<ALandscapeGizmoActiveActor> CurrentGizmoActor;
	//
	FLandscapeToolSet* CopyPasteToolSet;
	void CopyDataToGizmo();
	void PasteDataFromGizmo();

	FLandscapeToolSet* SplinesToolSet;
	void ShowSplineProperties();
	virtual void SelectAllConnectedSplineControlPoints();
	virtual void SelectAllConnectedSplineSegments();

	void ApplyRampTool();
	bool CanApplyRampTool();
	void ResetRampTool();

	/** Constructor */
	FEdModeLandscape();

	/** Initialization */
	void InitializeBrushes();
	void IntializeToolSet_Paint();
	void IntializeToolSet_Smooth();
	void IntializeToolSet_Flatten();
	void IntializeToolSet_Erosion();
	void IntializeToolSet_HydraErosion();
	void IntializeToolSet_Noise();
	void IntializeToolSet_Retopologize();
	void InitializeToolSet_NewLandscape();
	void InitializeToolSet_ResizeLandscape();
	void IntializeToolSet_Select();
	void IntializeToolSet_AddComponent();
	void IntializeToolSet_DeleteComponent();
	void IntializeToolSet_MoveToLevel();
	void IntializeToolSet_Mask();
	void IntializeToolSet_CopyPaste();
	void IntializeToolSet_Visibility();
	void IntializeToolSet_Splines();
	void InitializeToolSet_Ramp();
	void InitializeToolModes();

	/** Destructor */
	virtual ~FEdModeLandscape();

	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	virtual bool UsesToolkits() const OVERRIDE;

	/** FEdMode: Called when the mode is entered */
	virtual void Enter() OVERRIDE;

	/** FEdMode: Called when the mode is exited */
	virtual void Exit() OVERRIDE;

	/** FEdMode: Called when the mouse is moved over the viewport */
	virtual bool MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y ) OVERRIDE;

	/**
	 * FEdMode: Called when the mouse is moved while a window input capture is in effect
	 *
	 * @param	InViewportClient	Level editor viewport client that captured the mouse input
	 * @param	InViewport			Viewport that captured the mouse input
	 * @param	InMouseX			New mouse cursor X coordinate
	 * @param	InMouseY			New mouse cursor Y coordinate
	 *
	 * @return	true if input was handled
	 */
	virtual bool CapturedMouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY ) OVERRIDE;

	/** FEdMode: Called when a mouse button is pressed */
	virtual bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) OVERRIDE;

	/** FEdMode: Called when a mouse button is released */
	virtual bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) OVERRIDE;

	/** FEdMode: Allow us to disable mouse delta tracking during painting */
	virtual bool DisallowMouseDeltaTracking() const OVERRIDE;

	/** FEdMode: Called once per frame */
	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) OVERRIDE;

	/** FEdMode: Called when clicking on a hit proxy */
	virtual bool HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) OVERRIDE;

	/** FEdMode: Called when a key is pressed */
	virtual bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) OVERRIDE;

	/** FEdMode: Called when mouse drag input is applied */
	virtual bool InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale ) OVERRIDE;

	/** FEdMode: Render elements for the landscape tool */
	virtual void Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI ) OVERRIDE;

	/** FEdMode: Render HUD elements for this tool */
	virtual void DrawHUD( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas ) OVERRIDE;

	/** FEdMode: Handling SelectActor */
	virtual bool Select( AActor* InActor, bool bInSelected ) OVERRIDE;

	/** FEdMode: Check to see if an actor can be selected in this mode - no side effects */
	virtual bool IsSelectionAllowed( AActor* InActor, bool bInSelection ) const OVERRIDE;

	/** FEdMode: Called when the currently selected actor has changed */
	virtual void ActorSelectionChangeNotify() OVERRIDE;

	virtual void ActorMoveNotify() OVERRIDE;

	virtual EEditAction::Type GetActionEditDuplicate() OVERRIDE;
	virtual EEditAction::Type GetActionEditDelete() OVERRIDE;
	virtual EEditAction::Type GetActionEditCut() OVERRIDE;
	virtual EEditAction::Type GetActionEditCopy() OVERRIDE;
	virtual EEditAction::Type GetActionEditPaste() OVERRIDE;
	virtual bool ProcessEditDuplicate() OVERRIDE;
	virtual bool ProcessEditDelete() OVERRIDE;
	virtual bool ProcessEditCut() OVERRIDE;
	virtual bool ProcessEditCopy() OVERRIDE;
	virtual bool ProcessEditPaste() OVERRIDE;

	/** FEdMode: If the EdMode is handling InputDelta (ie returning true from it), this allows a mode to indicated whether or not the Widget should also move. */
	virtual bool AllowWidgetMove() OVERRIDE { return true; }

	/** FEdMode: Draw the transform widget while in this mode? */
	virtual bool ShouldDrawWidget() const OVERRIDE;

	/** FEdMode: Returns true if this mode uses the transform widget */
	virtual bool UsesTransformWidget() const OVERRIDE;

	virtual EAxisList::Type GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const OVERRIDE;

	virtual FVector GetWidgetLocation() const OVERRIDE;
	virtual bool GetCustomDrawingCoordinateSystem( FMatrix& InMatrix, void* InData ) OVERRIDE;
	virtual bool GetCustomInputCoordinateSystem( FMatrix& InMatrix, void* InData ) OVERRIDE;

	/** Forces real-time perspective viewports */
	void ForceRealTimeViewports( const bool bEnable, const bool bStoreCurrentState );

	/** Trace under the mouse cursor and return the landscape hit and the hit location (in landscape quad space) */
	bool LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, float& OutHitX, float& OutHitY );
	bool LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, FVector& OutHitLocation );

	/** Trace under the specified coordinates and return the landscape hit and the hit location (in landscape quad space) */
	bool LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, float& OutHitX, float& OutHitY );
	bool LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, FVector& OutHitLocation );

	/** Trace under the mouse cursor / specified screen coordinates against a world-space plane and return the hit location (in world space) */
	bool LandscapePlaneTrace(FLevelEditorViewportClient* ViewportClient, const FPlane& Plane, FVector& OutHitLocation);
	bool LandscapePlaneTrace(FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, const FPlane& Plane, FVector& OutHitLocation);

	void SetCurrentToolMode( FName ToolModeName, bool bRestoreCurrentTool = true);

	/** Change current tool */
	void SetCurrentTool( FName ToolSetName );
	void SetCurrentTool( int32 ToolIdx );

	void SetCurrentBrushSet(FName BrushSetName);
	void SetCurrentBrushSet(int32 BrushSetIndex);

	void SetCurrentBrush(FName BrushName);
	void SetCurrentBrush(int32 BrushIndex);

	const TArray<TSharedRef<FLandscapeTargetListInfo>>& GetTargetList();
	const TArray<FLandscapeListInfo>& GetLandscapeList();

	void AddLayerInfo(ULandscapeLayerInfoObject* LayerInfo);

	int32 UpdateLandscapeList();
	void UpdateTargetList();

	DECLARE_EVENT(FEdModeLandscape, FTargetsListUpdated);
	static FTargetsListUpdated TargetsListUpdated;

	void OnWorldChange();

	void OnMaterialCompilationFinished(UMaterialInterface* MaterialInterface);

	void ReimportData(const FLandscapeTargetListInfo& TargetInfo);
	void ImportData(const FLandscapeTargetListInfo& TargetInfo, const FString& Filename);

	ALandscape* ChangeComponentSetting(int32 NumComponentsX, int32 NumComponentsY, int32 InNumSubsections, int32 InSubsectionSizeQuads);

	TArray<FLandscapeToolMode> LandscapeToolModes;
	TArray<FLandscapeToolSet> LandscapeToolSets;
	TArray<FLandscapeBrushSet> LandscapeBrushSets;

	// For collision add visualization
	FLandscapeAddCollision* LandscapeRenderAddCollision;

private:
	TArray<TSharedRef<FLandscapeTargetListInfo>> LandscapeTargetList;
	TArray<FLandscapeListInfo> LandscapeList;

	UMaterialInterface* CachedLandscapeMaterial;

	bool bToolActive;
	UMaterial* GizmoMaterial;
};

namespace LandscapeEditorUtils
{
	template<typename T>
	TArray<T> ExpandData(const TArray<T>& Data, int32 OldMinX, int32 OldMinY, int32 OldMaxX, int32 OldMaxY,
	                                     int32 NewMinX, int32 NewMinY, int32 NewMaxX, int32 NewMaxY)
	{
		const int32 OldWidth = OldMaxX - OldMinX + 1;
		const int32 OldHeight = OldMaxY - OldMinY + 1;
		const int32 NewWidth = NewMaxX - NewMinX + 1;
		const int32 NewHeight = NewMaxY - NewMinY + 1;
		const int32 OffsetX = NewMinX - OldMinX;
		const int32 OffsetY = NewMinY - OldMinY;

		TArray<T> Result;
		Result.Empty(NewWidth * NewHeight);
		Result.AddUninitialized(NewWidth * NewHeight);

		for (int32 Y = 0; Y < NewHeight; ++Y)
		{
			const int32 OldY = FMath::Clamp<int32>(Y + OffsetY, 0, OldHeight - 1);

			// Pad anything to the left
			const T PadLeft = Data[OldY * OldWidth + 0];
			for (int32 X = 0; X < -OffsetX; ++X)
			{
				Result[Y * NewWidth + X] = PadLeft;
			}

			// Copy one row of the old data
			{
				const int32 X = FMath::Max(0, -OffsetX);
				const int32 OldX = FMath::Clamp<int32>(X + OffsetX, 0, OldWidth - 1);
				FMemory::Memcpy(&Result[Y * NewWidth + X], &Data[OldY * OldWidth + OldX], FMath::Min<int32>(OldWidth, NewWidth) * sizeof(T));
			}

			const T PadRight = Data[OldY * OldWidth + OldWidth - 1];
			for (int32 X = -OffsetX + OldWidth; X < NewWidth; ++X)
			{
				Result[Y * NewWidth + X] = PadRight;
			}
		}

		return Result;
	}
}

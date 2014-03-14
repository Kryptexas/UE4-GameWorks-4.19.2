// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#define LO_CAPTION_HEIGHT		(22)
#define LO_CONNECTOR_WIDTH		(8)
#define LO_CONNECTOR_LENGTH		(10)
#define LO_DESC_X_PADDING		(8)
#define LO_DESC_Y_PADDING		(8)
#define LO_TEXT_BORDER			(3)
#define LO_MIN_SHAPE_SIZE		(64)

#define LO_SLIDER_HANDLE_WIDTH	(7)
#define LO_SLIDER_HANDLE_HEIGHT	(15)

//
//	HLinkedObjProxy
//
struct HLinkedObjProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	UObject*	Obj;

	HLinkedObjProxy(UObject* InObj):
		HHitProxy(HPP_UI),
		Obj(InObj)
	{}
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE
	{
		Collector.AddReferencedObject( Obj );
	}
};

//
//	HLinkedObjProxySpecial
//
struct HLinkedObjProxySpecial : public HHitProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	UObject*	Obj;
	int32			SpecialIndex;

	HLinkedObjProxySpecial(UObject* InObj, int32 InSpecialIndex):
		HHitProxy(HPP_UI),
		Obj(InObj),
		SpecialIndex(InSpecialIndex)
	{}
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE
	{
		Collector.AddReferencedObject( Obj );
	}
};

/** Determines the type of connector a HLinkedObjConnectorProxy represents */
enum EConnectorHitProxyType
{
	/** an input connector */
	LOC_INPUT,

	/** output connector */
	LOC_OUTPUT,

	/** variable connector */
	LOC_VARIABLE,

	/** event connector */
	LOC_EVENT,
};

/**
 * In a linked object drawing, represents a connector for an object's link
 */
struct FLinkedObjectConnector
{
	/** the object that this proxy's connector belongs to */
	UObject* 				ConnObj;

	/** the type of connector this proxy is attached to */
	EConnectorHitProxyType	ConnType;

	/** the link index for this proxy's connector */
	int32						ConnIndex;

	/** Constructor */
	FLinkedObjectConnector(UObject* InObj, EConnectorHitProxyType InConnType, int32 InConnIndex):
		ConnObj(InObj), ConnType(InConnType), ConnIndex(InConnIndex)
	{}

	void AddReferencedObjects( FReferenceCollector& Collector )
	{
		Collector.AddReferencedObject( ConnObj );
	}
};

/**
 * Abstract interface to allow editor to provide callbacks
 */
class FLinkedObjectDrawHelper
{
public:
	/** virtual destructor to allow proper deallocation*/
	virtual ~FLinkedObjectDrawHelper(void) {};
	/** Callback for rendering preview meshes */
	virtual void DrawThumbnail (UObject* PreviewObject,  TArray<UMaterialInterface*>& InMaterialOverrides, FViewport* Viewport, FCanvas* Canvas, const FIntRect& InRect) = 0;
};

/**
 * Hit proxy for link connectors in a linked object drawing.
 */
struct HLinkedObjConnectorProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	FLinkedObjectConnector Connector;

	HLinkedObjConnectorProxy(UObject* InObj, EConnectorHitProxyType InConnType, int32 InConnIndex):
		HHitProxy(HPP_UI),
		Connector(InObj, InConnType, InConnIndex)
	{}

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE
	{
		Connector.AddReferencedObjects( Collector );
	}
};

/**
 * Hit proxy for a line connection between two objects.
 */
struct HLinkedObjLineProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	FLinkedObjectConnector Src, Dest;

	HLinkedObjLineProxy(UObject *SrcObj, int32 SrcIdx, UObject *DestObj, int32 DestIdx) :
		HHitProxy(HPP_UI),
		Src(SrcObj,LOC_OUTPUT,SrcIdx),
		Dest(DestObj,LOC_INPUT,DestIdx)
	{}

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE
	{
		Src.AddReferencedObjects( Collector );
		Dest.AddReferencedObjects( Collector );
	}
};

struct FLinkedObjConnInfo
{
	// The name of the connector
	FString			Name;
	// Tooltip strings for this connector
	TArray<FString>	ToolTips;
	// The color of the connector
	FColor			Color;
	// Whether or not this connector is an output
	bool			bOutput;
	// Whether or not this connector is moving
	bool			bMoving;
	// Whether or not this connector can move left
	bool			bClampedMax;
	// Whether or not this connector can move right
	bool			bClampedMin;
	// Whether or not this connector is new since the last draw
	bool			bNewConnection;
	// The delta that should be applied to this connectors initial position
	int32				OverrideDelta;
	// Enabled will render as the default text color with the connector in the chosen color
	// Disabled will gray the name and connector - overriding the chosen color
	bool			bEnabled;

	FLinkedObjConnInfo( const TCHAR* InName, const FColor& InColor, bool InMoving = false, bool InNewConnection = true, int32 InOverrideDelta = 0, const bool InbOutput = 0, bool InEnabled = true )
	{
		Name = InName;
		Color = InColor;
		bOutput = InbOutput;
		bMoving = InMoving;
		OverrideDelta = InOverrideDelta;
		bNewConnection = InNewConnection;
		bClampedMax = false;
		bClampedMin = false;
		bEnabled = InEnabled;
	}
};

struct FLinkedObjDrawInfo
{
	// Lists of different types of connectors
	TArray<FLinkedObjConnInfo>	Inputs;
	TArray<FLinkedObjConnInfo>	Outputs;
	TArray<FLinkedObjConnInfo>	Variables;
	TArray<FLinkedObjConnInfo>	Events;

	// Pointer to the sequence object that used this struct
	UObject*		ObjObject;
	
	// If we have a pending variable connector recalc
	bool			bPendingVarConnectorRecalc;
	// If we have a pending input connector recalc
	bool			bPendingInputConnectorRecalc;
	// if we have a pending output connector recalc
	bool			bPendingOutputConnectorRecalc;

	/** Defaults to not having any visualization unless otherwise set*/
	FIntPoint		VisualizationSize;

	// Outputs - so you can remember where connectors are for drawing links
	TArray<int32>			InputY;
	TArray<int32>			OutputY;
	TArray<int32>			VariableX;
	TArray<int32>			EventX;
	int32				DrawWidth;
	int32				DrawHeight;

	/** Where the resulting visualization should be drawn*/
	FIntPoint		VisualizationPosition;

	/** Tool tip strings for this node. */
	TArray<FString> ToolTips;

	FLinkedObjDrawInfo()
	{
		ObjObject = NULL;
			
		bPendingVarConnectorRecalc = false;
		bPendingInputConnectorRecalc = false;
		bPendingOutputConnectorRecalc = false;

		VisualizationSize = FIntPoint::ZeroValue;
	}
};

/**
 * A collection of static drawing functions commonly used by linked object editors.
 */
class FLinkedObjDrawUtils
{
public:
	/** Set the font used by Canvas-based editors */
	ENGINE_API static void InitFonts( UFont* InNormalFont );
	/** Get the font used by Canvas-based editor */
	ENGINE_API static UFont* GetFont();
	ENGINE_API static void DrawNGon(FCanvas* Canvas, const FVector2D& Center, const FColor& Color, int32 NumSides, float Radius);
	ENGINE_API static void DrawSpline(FCanvas* Canvas, const FIntPoint& Start, const FVector2D& StartDir, const FIntPoint& End, const FVector2D& EndDir, const FColor& LineColor, bool bArrowhead, bool bInterpolateArrowPositon=false);
	static void DrawArrowhead(FCanvas* InCanvas, const FIntPoint& InPos, const FVector2D& InDir, const FColor& InColor);

	ENGINE_API static FIntPoint GetTitleBarSize(FCanvas* Canvas, const TArray<FString>& Names);
	static FIntPoint GetCommentBarSize(FCanvas* Canvas, const TCHAR* Comment);
	ENGINE_API static int32 DrawTitleBar(FCanvas* Canvas, const FIntPoint& Pos, const FIntPoint& Size, const FColor& BorderColor, const FColor& BkgColor, const TArray<FString>& Names, const TArray<FString>& Comments, int32 BorderWidth = 0);
	static int32 DrawComments(FCanvas* Canvas, const FIntPoint& Pos, const FIntPoint& Size, const TArray<FString>& Comments, UFont* Font);

	ENGINE_API static FIntPoint GetLogicConnectorsSize(const FLinkedObjDrawInfo& ObjInfo, int32* InputY=NULL, int32* OutputY=NULL);
	
	/** 
	 * Draws logic connectors on the sequence object with adjustments for moving connectors
	 * 
	 * @param Canvas	The canvas to draw on
	 * @param ObjInfo	Information about the sequence object so we know how to draw the connectors
	 * @param Pos		The positon of the sequence object
	 * @param Size		The size of the sequence object
	 * @param ConnectorTileBackgroundColor		(Optional)Color to apply to a connector tiles bacground
 	 * @param bHaveMovingInput			(Optional)Whether or not we have a moving input connector
	 * @param bHaveMovingOutput			(Optional)Whether or not we have a moving output connector
	 * @param bGhostNonMoving			(Optional)Whether or not we should ghost all non moving connectors while one is moving
	 */
	static void DrawLogicConnectorsWithMoving(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const FLinearColor* ConnectorTileBackgroundColor=NULL, const bool bHaveMovingInput = false, const bool bHaveMovingOutput = false, const bool bGhostNonMoving = false );

	static FIntPoint GetVariableConnectorsSize(FCanvas* Canvas, const FLinkedObjDrawInfo& ObjInfo);
	
	/** 
	 * Draws variable connectors on the sequence object with adjustments for moving connectors
	 * 
	 * @param Canvas	The canvas to draw on
	 * @param ObjInfo	Information about the sequence object so we know how to draw the connectors
	 * @param Pos		The positon of the sequence object
	 * @param Size		The size of the sequence object
	 * @param VarWidth	The width of space we have to draw connectors
 	 * @param bHaveMovingVariable			(Optional)Whether or not we have a moving variable connector
	 * @param bGhostNonMoving			(Optional)Whether or not we should ghost all non moving connectors while one is moving
	 */
	static void DrawVariableConnectorsWithMoving(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const int32 VarWidth, bool bHaveMovingVariable = false, const bool bGhostNonMoving = false );

	ENGINE_API static void DrawLogicConnectors(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const FLinearColor* ConnectorTileBackgroundColor=NULL);
	static void DrawVariableConnectors(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const int32 VarWidth);
	
	/** Draws connection and node tooltips. */
	ENGINE_API static void DrawToolTips(FCanvas* Canvas, const FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size);

	ENGINE_API static void DrawLinkedObj(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const TCHAR* Name, const TCHAR* Comment, const FColor& BorderColor, const FColor& TitleBkgColor, const FIntPoint& Pos);

	static int32 ComputeSliderHeight(int32 SliderWidth);
	static int32 Compute2DSliderHeight(int32 SliderWidth);
	// returns height of drawn slider
	static int32 DrawSlider( FCanvas* Canvas, const FIntPoint& SliderPos, int32 SliderWidth, const FColor& BorderColor, const FColor& BackGroundColor, float SliderPosition, const FString& ValText, UObject* Obj, int SliderIndex=0, bool bDrawTextOnSide=false);
	// returns height of drawn slider
	static int32 Draw2DSlider(FCanvas* Canvas, const FIntPoint &SliderPos, int32 SliderWidth, const FColor& BorderColor, const FColor& BackGroundColor, float SliderPositionX, float SliderPositionY, const FString &ValText, UObject *Obj, int SliderIndex, bool bDrawTextOnSide);

	/**
	 * @return		true if the current viewport contains some portion of the specified AABB.
	 */
	ENGINE_API static bool AABBLiesWithinViewport(FCanvas* Canvas, float X, float Y, float SizeX, float SizeY);

	/**
	 * Convenience function for filtering calls to FRenderInterface::DrawTile via AABBLiesWithinViewport.
	 */
	ENGINE_API static void DrawTile(FCanvas* Canvas,float X,float Y,float SizeX,float SizeY,const FVector2D& InUV0, const FVector2D& InUV1, const FLinearColor& Color,FTexture* Texture = NULL,bool AlphaBlend = 1);

	/**
	 * Convenience function for filtering calls to FRenderInterface::DrawTile via AABBLiesWithinViewport.
	 */
	ENGINE_API static void DrawTile(FCanvas* Canvas, float X,float Y,float SizeX,float SizeY,const FVector2D& InUV0, const FVector2D& InUV1, FMaterialRenderProxy* MaterialRenderProxy);

	/**
	 * Convenience function for filtering calls to FRenderInterface::DrawString through culling heuristics.
	 */
	ENGINE_API static int32 DrawString(FCanvas* Canvas,float StartX,float StartY, const FString& Text,class UFont* Font,const FLinearColor& Color);

	/**
	 * Convenience function for filtering calls to FRenderInterface::DrawShadowedString through culling heuristics.
	 */
	ENGINE_API static int32 DrawShadowedString(FCanvas* Canvas,float StartX,float StartY, const FString& Text,class UFont* Font,const FLinearColor& Color);

	/**
	 * Convenience function for filtering calls to FRenderInterface::DrawStringWrapped through culling heuristics.
	 */
	static int32 DisplayComment( FCanvas* Canvas, bool Draw, float CurX, float CurY, float Z, float& XL, float& YL, UFont* Font, const FString& Text, FLinearColor DrawColor );

	/**
	 * Takes a transformation matrix and returns a uniform scaling factor based on the 
	 * length of the rotation vectors.
	 *
	 * @param Matrix	A matrix to use to calculate a uniform scaling parameter.
	 *
	 * @return A uniform scaling factor based on the matrix passed in.
	 */
	ENGINE_API static float GetUniformScaleFromMatrix(const FMatrix &Matrix);

private:
	/** Font to use in Canvas-based editors */
	static UFont* NormalFont;
};

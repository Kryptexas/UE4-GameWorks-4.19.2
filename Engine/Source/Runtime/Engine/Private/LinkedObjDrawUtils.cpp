// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinkedObjDrawUtils.cpp: Utils for drawing linked objects.
=============================================================================*/

#include "EnginePrivate.h"
#include "LinkedObjDrawUtils.h"

IMPLEMENT_HIT_PROXY(HLinkedObjProxy,HHitProxy);
IMPLEMENT_HIT_PROXY(HLinkedObjProxySpecial,HHitProxy);
IMPLEMENT_HIT_PROXY(HLinkedObjConnectorProxy,HHitProxy);
IMPLEMENT_HIT_PROXY(HLinkedObjLineProxy,HHitProxy);

/** Minimum viewport zoom at which text will be rendered. */
static const float	TextMinZoom(0.3f);

/** Minimum viewport zoom at which arrowheads will be rendered. */
static const float	ArrowheadMinZoom(0.3f);

/** Minimum viewport zoom at which connectors will be rendered. */
static const float	ConnectorMinZoom(0.2f);

/** Minimum viewport zoom at which connectors will be rendered. */
static const float	SliderMinZoom(0.2f);

static const float	MaxPixelsPerStep(15.f);

static const int32	ArrowheadLength(14);
static const int32	ArrowheadWidth(4);

static const FColor SliderHandleColor(0, 0, 0);

static const FLinearColor DisabledColor(.127f,.127f,.127f);
static const FLinearColor DisabledTextColor(FLinearColor::Gray);

/** Font used by Canvas-based editors */
UFont* FLinkedObjDrawUtils::NormalFont = NULL;

/** Set the font used by Canvas-based editors */
void FLinkedObjDrawUtils::InitFonts( UFont* InNormalFont )
{
	NormalFont = InNormalFont;
}

/** Get the font used by Canvas-based editor */
UFont* FLinkedObjDrawUtils::GetFont()
{
	return NormalFont;
}

void FLinkedObjDrawUtils::DrawNGon(FCanvas* Canvas, const FVector2D& Center, const FColor& Color, int32 NumSides, float Radius)
{
	if ( AABBLiesWithinViewport( Canvas, Center.X-Radius, Center.Y-Radius, Radius*2, Radius*2) )
	{
		FCanvasNGonItem NGonItem( Center, FVector2D( Radius, Radius ), FMath::Clamp(NumSides, 3, 255), Color );
		Canvas->DrawItem( NGonItem );
	}
}

/**
 *	@param EndDir End tangent. Note that this points in the same direction as StartDir ie 'along' the curve. So if you are dealing with 'handles' you will need to invert when you pass it in.
 *
 */
void FLinkedObjDrawUtils::DrawSpline(FCanvas* Canvas, const FIntPoint& Start, const FVector2D& StartDir, const FIntPoint& End, const FVector2D& EndDir, const FColor& LineColor, bool bArrowhead, bool bInterpolateArrowDirection/*=false*/)
{
	const int32 MinX = FMath::Min( Start.X, End.X );
	const int32 MaxX = FMath::Max( Start.X, End.X );
	const int32 MinY = FMath::Min( Start.Y, End.Y );
	const int32 MaxY = FMath::Max( Start.Y, End.Y );

	if ( AABBLiesWithinViewport( Canvas, MinX, MinY, MaxX - MinX, MaxY - MinY ) )
	{
		// Don't draw the arrowhead if the editor is zoomed out most of the way.
		const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
		if ( Zoom2D < ArrowheadMinZoom )
		{
			bArrowhead = false;
		}

		const FVector2D StartVec( Start );
		const FVector2D EndVec( End );

		// Rough estimate of length of curve. Use direct length and distance between 'handles'. Sure we can do better.
		const float DirectLength = (EndVec - StartVec).Size();
		const float HandleLength = ((EndVec - EndDir) - (StartVec + StartDir)).Size();
		
		const int32 NumSteps = FMath::Ceil(FMath::Max(DirectLength,HandleLength)/MaxPixelsPerStep);

		FVector2D OldPos = StartVec;

		FCanvasLineItem LineItem( FVector2D::ZeroVector, FVector2D::ZeroVector );
		LineItem.SetColor( LineColor );
		for(int32 i=0; i<NumSteps; i++)
		{
			const float Alpha = ((float)i+1.f)/(float)NumSteps;
			const FVector2D NewPos = FMath::CubicInterp(StartVec, StartDir, EndVec, EndDir, Alpha);

			const FIntPoint OldIntPos = OldPos.IntPoint();
			const FIntPoint NewIntPos = NewPos.IntPoint();

			LineItem.Draw( Canvas, OldPos, NewPos );

			// If this is the last section, use its direction to draw the arrowhead.
			if( (i == NumSteps-1) && (i >= 2) && bArrowhead )
			{
				// Go back 3 steps to give us decent direction for arrowhead
				FVector2D ArrowStartPos;

				if(bInterpolateArrowDirection)
				{
					const float ArrowStartAlpha = ((float)i-2.f)/(float)NumSteps;
					ArrowStartPos = FMath::CubicInterp(StartVec, StartDir, EndVec, EndDir, ArrowStartAlpha);
				}
				else
				{
					ArrowStartPos = OldPos;
				}

				const FVector2D StepDir = (NewPos - ArrowStartPos).SafeNormal();
				DrawArrowhead( Canvas, NewIntPos, StepDir, LineColor );
			}

			OldPos = NewPos;
		}
	}
}

void FLinkedObjDrawUtils::DrawArrowhead(FCanvas* InCanvas, const FIntPoint& InPos, const FVector2D& InDir, const FColor& InColor)
{
	// Don't draw the arrowhead if the editor is zoomed out most of the way.
	const float Zoom2D = GetUniformScaleFromMatrix(InCanvas->GetTransform());
	if ( Zoom2D > ArrowheadMinZoom )
	{
		const FVector2D Orth(InDir.Y, -InDir.X);
		const FVector2D PosVec(InPos);
		const FVector2D pt2 = PosVec - (InDir * ArrowheadLength) - (Orth * ArrowheadWidth);
		const FVector2D pt1 = PosVec;
		const FVector2D pt3 = PosVec - (InDir * ArrowheadLength) + (Orth * ArrowheadWidth);

		FCanvasTriangleItem TriItem( pt1, pt2, pt3, GWhiteTexture );
		TriItem.SetColor( InColor );
		InCanvas->DrawItem( TriItem );
	}
}


FIntPoint FLinkedObjDrawUtils::GetTitleBarSize(FCanvas* Canvas, const TArray<FString>& Names)
{
	int32 MaxXL = 0;
	int32 TotalY = 0;

	for (int32 StringIndex = 0; StringIndex < Names.Num(); StringIndex++)
	{
		int32 XL, YL;
		StringSize( NormalFont, XL, YL, *Names[StringIndex] );
		MaxXL = FMath::Max(MaxXL, XL);
		TotalY += YL + 2;
	}

	const int32 LabelWidth = MaxXL + (LO_TEXT_BORDER*2) + 4;

	return FIntPoint( FMath::Max(LabelWidth, LO_MIN_SHAPE_SIZE), FMath::Max(LO_CAPTION_HEIGHT, TotalY) );
}

FIntPoint FLinkedObjDrawUtils::GetCommentBarSize(FCanvas* Canvas, const TCHAR* Comment)
{
	int32 XL, YL;
	StringSize( GEngine->GetTinyFont(), XL, YL, Comment );

	const int32 LabelWidth = XL + (LO_TEXT_BORDER*2) + 4;

	return FIntPoint( FMath::Max(LabelWidth, LO_MIN_SHAPE_SIZE), YL + 4 );
}

int32 FLinkedObjDrawUtils::DrawTitleBar(FCanvas* Canvas, const FIntPoint& Pos, const FIntPoint& Size, const FColor& BorderColor, const FColor& BkgColor, const TArray<FString>& Names, const TArray<FString>& Comments, int32 BorderWidth /*= 0*/)
{
	// Draw label at top
	if ( AABBLiesWithinViewport( Canvas, Pos.X, Pos.Y, Size.X, Size.Y ) )
	{
		int32 DBorderWidth = BorderWidth * 2;
		FCanvasTileItem TileItem( FVector2D( Pos.X-BorderWidth, Pos.Y-BorderWidth ), FVector2D( Size.X+DBorderWidth, Size.Y+DBorderWidth ), BorderColor );
		Canvas->DrawItem( TileItem );
		TileItem.Position = FVector2D( Pos.X+1, Pos.Y+1 );
		TileItem.Size = FVector2D( Size.X-2, Size.Y-2 );
		TileItem.SetColor( BkgColor );
		Canvas->DrawItem( TileItem );
	}

	if ( Names.Num() > 0 )
	{
		int32 BaseY = 0;
		for (int32 StringIndex = 0; StringIndex < Names.Num(); StringIndex++)
		{
			int32 XL, YL;
			StringSize( NormalFont, XL, YL, *Names[StringIndex] );

			const FIntPoint StringPos( Pos.X + ((Size.X - XL) / 2), Pos.Y + BaseY + ((Size.Y / Names.Num() - YL) / 2) );
			if ( AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) )
			{
				DrawShadowedString( Canvas, StringPos.X, StringPos.Y, *Names[StringIndex], NormalFont, FColor(255,255,128) );
			}

			BaseY += YL + 2;
		}
	}

	return DrawComments(Canvas, Pos, Size, Comments, GEngine->GetSmallFont());
}

int32 FLinkedObjDrawUtils::DrawComments(FCanvas* Canvas, const FIntPoint& Pos, const FIntPoint& Size, const TArray<FString>& Comments, UFont* Font)
{
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	int32 CommentY = Pos.Y - 2;

	// Handle multiline comments
	if( !Canvas->IsHitTesting() && Comments.Num() > 0 )
	{
		for (int32 CommentIdx = Comments.Num() - 1; CommentIdx >= 0; --CommentIdx)
		{
			int32 XL, YL;
			StringSize( Font, XL, YL, *Comments[CommentIdx] );
			CommentY -= YL;

			const FIntPoint StringPos( Pos.X + 2, CommentY );
			if ( AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) )
			{
				DrawString( Canvas, StringPos.X, StringPos.Y, *Comments[CommentIdx], Font, FColor(0,0,0) );
				if( Zoom2D > 1.f - DELTA )
				{
					DrawString( Canvas, StringPos.X+1, StringPos.Y, *Comments[CommentIdx], Font, FColor(120,120,255) );
				}
			}
			CommentY -= 2;
		}
	}
	return CommentY;
}


// The InputY and OuputY are optional extra outputs, giving the size of the input and output connectors

FIntPoint FLinkedObjDrawUtils::GetLogicConnectorsSize(const FLinkedObjDrawInfo& ObjInfo, int32* InputY, int32* OutputY)
{
	int32 MaxInputDescX = 0;
	int32 MaxInputDescY = 0;
	for(int32 i=0; i<ObjInfo.Inputs.Num(); i++)
	{
		int32 XL, YL;
		StringSize( NormalFont, XL, YL, *ObjInfo.Inputs[i].Name );

		MaxInputDescX = FMath::Max(XL, MaxInputDescX);

		if(i>0) MaxInputDescY += LO_DESC_Y_PADDING;
		MaxInputDescY += FMath::Max(YL, LO_CONNECTOR_WIDTH);
	}

	int32 MaxOutputDescX = 0;
	int32 MaxOutputDescY = 0;
	for(int32 i=0; i<ObjInfo.Outputs.Num(); i++)
	{
		int32 XL, YL;
		StringSize( NormalFont, XL, YL, *ObjInfo.Outputs[i].Name );

		MaxOutputDescX = FMath::Max(XL, MaxOutputDescX);

		if(i>0) MaxOutputDescY += LO_DESC_Y_PADDING;
		MaxOutputDescY += FMath::Max(YL, LO_CONNECTOR_WIDTH);
	}

	const int32 NeededX = MaxInputDescX + MaxOutputDescX + LO_DESC_X_PADDING + (2*LO_TEXT_BORDER);
	const int32 NeededY = FMath::Max( MaxInputDescY, MaxOutputDescY ) + (2*LO_TEXT_BORDER);

	if(InputY)
	{
		*InputY = MaxInputDescY + (2*LO_TEXT_BORDER);
	}

	if(OutputY)
	{
		*OutputY = MaxOutputDescY + (2*LO_TEXT_BORDER);
	}

	return FIntPoint(NeededX, NeededY);
}

// Helper strut for drawing moving connectors
struct FConnectorPlacementData
{
	/** The index into the original array where the connector resides. We need this because data can be sorted out of its original order */
	int32 Index;

	/** Only used by variable event connectors since their connectors reside in a different array but still 
		Need to be sorted with the rest of the variable connectors */
	int32 EventOffset;

	/** The position of the connector (X or Y axis depending on connector type) */
	int32 Pos;
	
	/** The delta that is added on to the above position based on user movement */
	int32 OverrideDelta;

	/** Spacing in both the X and Y direction for the connector string */
	FIntPoint Spacing;

	/** Whether or not the connector can be moved to the left */
	bool bClampedMin;

	/** Whether or not the connector can be moved to the right */
	bool bClampedMax;

	/** The type of connector we have */
	EConnectorHitProxyType ConnType;

	
	/** Constructor */
	FConnectorPlacementData()
		: Index(-1),
		  EventOffset(0),
		  Pos(0),
		  OverrideDelta(0),
		  Spacing(0,0),
		  bClampedMin(false),
		  bClampedMax(false),
		  ConnType(LOC_VARIABLE)
	{
	}
};

/**
 * Compare callback predicate for TArray::Sort. Compares Connector X positions
 */
struct FCompareConnectorDataX
{
	FORCEINLINE bool operator()( const FConnectorPlacementData& A, const FConnectorPlacementData& B ) const
	{
		// The midpoint positions of the connectors is what is compared.
		// Calculated as follows:
		//	 Pos.X		         Pos.X+Spacing.X/2	      Pos.X+Spacing.X
		//	   |-----------------------|-----------------------|
				
		const int32 ConnPosA =  A.Pos + A.Spacing.X/2;
		const int32 ConnPosB = B.Pos + B.Spacing.X/2;
		return ( ConnPosA < ConnPosB );
	}
};

/**
 * Compare callback predicate for TArray::Sort. Compares Connector Y positions
 */
struct FCompareConnectorDataY
{
	FORCEINLINE bool operator()( const FConnectorPlacementData& A, const FConnectorPlacementData& B ) const
	{
		// The midpoint positions of the connectors is what is compared.
		// Calculated as follows:
		//	 Pos.Y		         Pos.Y+Spacing.Y/2	      Pos.Y+Spacing.Y
		//	   |-----------------------|-----------------------|
				
		const int32 ConnPosA =  A.Pos + A.Spacing.Y/2;
		const int32 ConnPosB = B.Pos + B.Spacing.Y/2;
		return ( ConnPosA < ConnPosB );
	}
};

/** 
 * Adjusts connectors after they have been moved so that they do not overlap.
 * This is done by sorting the connectors based on their (possibly) overlapping positions.
 * Once they are sorted, we recompute their positions based on their sorted order so that they are spaced evenly and do not overlap but still maintain relative position based on their sorted order.
 *
 * @param PlacementData		An array of connector placement data to be adjusted
 * @param Pos			The position of the sequence object where these connectors reside
 * @param Size			The size of the sequence object where these connectors reside.
 * @param VarSpace		The amount of space we have to place connectors
 * @param AdjustY		Whether or not we are adjusting in the Y direction (since spacing is computed differently).
 */
static void AdjustForOverlap( TArray<FConnectorPlacementData>& PlacementData, const FIntPoint& Pos, const FIntPoint& Size, int32 VarSpace, bool bAdjustY )
{	
	// Copy the placement data into a new array so we maintain the original data's order
	TArray<FConnectorPlacementData> SortedData = PlacementData;
	// The position of the last connector (initialized to the center of the sequence object
	int32 LastPos;
	// The spacing of the last connector
	int32 LastSpacing = 0;
	// The size of the sequence object to check when centering connectors.
	int32 CheckSize;	

	if( bAdjustY )
	{	
		// We are adjusting in the Y direction, compute the amount of space we have to place connectors
		const int32 ConnectorRangeY	= Size.Y - 2*LO_TEXT_BORDER;
		// Calculate the center spot
		const int32 CenterY		= Pos.Y + LO_TEXT_BORDER + ConnectorRangeY / 2;
		
		CheckSize = Size.Y;

		// Initialize the last connector spacing and position.
		LastSpacing = ConnectorRangeY/PlacementData.Num();
		LastPos = CenterY - (PlacementData.Num()-1) * LastSpacing / 2;

		// Sort the connectors based on their positions
		SortedData.Sort(FCompareConnectorDataY());
	}
	else
	{
		// Initialize last position
		LastPos = Pos.X;
		CheckSize = Size.X;

		// if the title is wider than needed for the variables
		if (VarSpace < CheckSize)
		{
			// then center the variables
			LastPos += (CheckSize - VarSpace)/2;
		}

		// Sort the connectors based on their positions
		SortedData.Sort(FCompareConnectorDataX());
	}

	// Now iterate through each connector in sorted position and place them in that order
	for( int32 SortedIdx = 0; SortedIdx < SortedData.Num(); ++SortedIdx )
	{
		FConnectorPlacementData CurData = SortedData[ SortedIdx ];
		if( bAdjustY )
		{
			// Calculation of position done by using the position of the starting connector, 
			// offset by the current index of the connector and adding padding.
			// We only worry about the Y position of variable connectors since the X position is fixed
			const int32 ConnectorPos = LastPos + SortedIdx * LastSpacing;
			CurData.OverrideDelta = ConnectorPos - Pos.Y;
			CurData.Pos = Pos.Y + CurData.OverrideDelta;

			// Lookup into the original data and change its positon and delta override
			// This is why we store the index on the PlacmentData struct
			// Delta override is relative to the sequence objects position. 
			FConnectorPlacementData& OriginalData = PlacementData[ CurData.Index ];
			OriginalData.Pos = CurData.Pos;
			OriginalData.OverrideDelta = SortedData[SortedIdx].OverrideDelta = ConnectorPos - Pos.Y;
		}
		else
		{
			// Calculation of position done by using the position of the previous connector and adding spacing for
			// the current connector's name plus some padding
			// We only worry about the X position of variable connectors since the Y position is fixed
			const int32 ConnectorPos = LastPos + LastSpacing + (LO_DESC_X_PADDING * 2);
			
			CurData.OverrideDelta = ConnectorPos - Pos.X;
			CurData.Pos = Pos.X + CurData.OverrideDelta;

			// Lookup into the original data and change its positon and delta override
			// This is why we store the index on the PlacmentData struct
			// Delta override is relative to the sequence objects position. 
			FConnectorPlacementData& OriginalData = PlacementData[ CurData.Index + CurData.EventOffset ];
			OriginalData.Pos = CurData.Pos;
			OriginalData.OverrideDelta = CurData.OverrideDelta = ConnectorPos - Pos.X;
	
			// The new last pos and spacing is this connectors position and spacing
			LastPos = ConnectorPos;
			LastSpacing = SortedData[SortedIdx].Spacing.X;
		}
	}
}

/** 
 * Clamps a connectors position so it never crosses over the boundary of a sequence object.
 * @param PlacementData		The connector data to clamp.  
 * @param Min				The min edge of the sequence object
 * @param Max				The max edge of the sequence object
 */
static void ClampConnector( FConnectorPlacementData& PlacementData, int32 MinEdge, int32 MaxEdge )
{
	if( PlacementData.Pos <= MinEdge )
	{
		// The connector is too far past the min edge of the sequence object
		PlacementData.Pos = MinEdge+1;
		// The delta override is always 0 at the min edge of the sequence object, the 1 is so we arent actually on the edge of the object
		PlacementData.OverrideDelta = PlacementData.Pos - MinEdge;
		PlacementData.bClampedMin = true;
		PlacementData.bClampedMax = false;
	}
	else if( PlacementData.Pos >= MaxEdge )
	{
		// The connector is too far past the max edge of the sequence object
		PlacementData.Pos = MaxEdge-1;
		// the delta override is always the length of the sequence object
		PlacementData.OverrideDelta = PlacementData.Pos - MinEdge;
		PlacementData.bClampedMin = false;
		PlacementData.bClampedMax = true;
	}
	else
	{
		// Nothing needs to be clamped
		PlacementData.bClampedMin = false;
		PlacementData.bClampedMax = false;
	}
}

/**
 * Pre-Calculates input or output connector positions based on their current delta offsets.
 * We need all the positions of connectors upfront so we can sort them based on position later.
 *
 * @param OutPlacementData	An array of connector placement data that is poptulated
 * @param ObjInfo		Information about the sequence object were the output connectors reside
 * @param Pos			The position of the sequence object where these connectors reside
 * @param Size			The size of the sequence object where these connectors reside.
 */
static void PreCalculateLogicConnectorPositions( TArray< FConnectorPlacementData >& OutPlacementData, FLinkedObjDrawInfo& ObjInfo, bool bCalculateInputs, const FIntPoint& Pos, const FIntPoint& Size )
{
	// Set up a reference to the connector data we are actually calculating
	TArray<FLinkedObjConnInfo>& Connectors = bCalculateInputs ? ObjInfo.Inputs : ObjInfo.Outputs;

	// The amount of space there is to draw output connectors
	const int32 ConnectorRangeY	= Size.Y - 2*LO_TEXT_BORDER;
	// The center position of the draw area
	const int32 CenterY			= Pos.Y + LO_TEXT_BORDER + ConnectorRangeY / 2;
	// The spacing that should exist between each connector
	const int32 SpacingY	= ConnectorRangeY/Connectors.Num();
	// The location of the first connector
	const int32 StartY	= CenterY - (Connectors.Num()-1) * SpacingY / 2;

	// Pre-calculate all connections.  We need to know the positions of each connector 
	// before they are drawn so we can position them correctly if one moved
	// We will adjust for overlapping connectors if need be.
	for( int32 ConnectorIdx=0; ConnectorIdx< Connectors.Num(); ++ConnectorIdx )
	{
		FLinkedObjConnInfo& ConnectorInfo = Connectors[ConnectorIdx];

		if( ConnectorInfo.bNewConnection == true && ConnectorInfo.bMoving == false )
		{	
			// This is a new connection.  It should be positioned at the end of all the connectors
			const int32 NewLoc = StartY + ConnectorIdx * SpacingY;

			// Find out the change in position from the sequence objects top edge. The top edge is used as zero for the Delta override )
			ConnectorInfo.OverrideDelta = NewLoc - Pos.Y;

			// Recalc all connector positions if one was added.
			// Store it on the ObjInfo struct so it can be passed back to the sequence object.
			// This way we can defer this recalculation.
			bCalculateInputs ? ObjInfo.bPendingInputConnectorRecalc = true : ObjInfo.bPendingOutputConnectorRecalc = true;
		}
		else if( ConnectorInfo.bMoving == true )
		{
			// Recalc all connector positions if one was moved.
			// Store it on the ObjInfo struct so it can be passed back to the sequence object.
			// This way we can defer this recalculation until when we don't have an interp actor
			bCalculateInputs ? ObjInfo.bPendingInputConnectorRecalc = true : ObjInfo.bPendingOutputConnectorRecalc = true;
		}

		const int32 DrawY = Pos.Y + ConnectorInfo.OverrideDelta;

		// Set up some placement data for the connector 
		// Not all the placement data is needed
		FConnectorPlacementData ConnData;
		// Index of the connector. This is used to lookup the correct index later because the call to AdjustForOverlap sorts the connectors by position
		ConnData.Index = ConnectorIdx;
		ConnData.ConnType = bCalculateInputs ? LOC_INPUT : LOC_OUTPUT;
		ConnData.Pos = DrawY;
		ConnData.OverrideDelta = ConnectorInfo.OverrideDelta;
		ConnData.Spacing = FIntPoint(0, SpacingY);

		if( ConnectorInfo.bMoving )
		{
			// Make sure moving connectors can not go past the bounds of the sequence object
 			ClampConnector( ConnData, Pos.Y, Pos.Y+Size.Y );
		}

		OutPlacementData.Add( ConnData );
	}
}

void FLinkedObjDrawUtils::DrawLogicConnectors(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const FLinearColor* ConnectorTileBackgroundColor)
{
	const bool bHitTesting				= Canvas->IsHitTesting();
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	const bool bSufficientlyZoomedIn	= Zoom2D > ConnectorMinZoom;

	// Get the maximum height of a string in this font
	int32 XL, YL;
	StringSize(NormalFont, XL, YL, TEXT("GgIhy"));

	const int32 ConnectorWidth	= FMath::Max(YL, LO_CONNECTOR_WIDTH);
	const int32 ConnectorRangeY	= Size.Y - 2*LO_TEXT_BORDER;
	const int32 CenterY			= Pos.Y + LO_TEXT_BORDER + ConnectorRangeY / 2;

	// Do nothing if no Input connectors
	if( ObjInfo.Inputs.Num() > 0 )
	{
		const int32 SpacingY	= ConnectorRangeY/ObjInfo.Inputs.Num();
		const int32 StartY	= CenterY - (ObjInfo.Inputs.Num()-1) * SpacingY / 2;
		ObjInfo.InputY.AddUninitialized( ObjInfo.Inputs.Num() );

		for(int32 i=0; i<ObjInfo.Inputs.Num(); i++)
		{
			const int32 LinkY		= StartY + i * SpacingY;
			ObjInfo.InputY[i]	= LinkY;

			if ( bSufficientlyZoomedIn )
			{
				if(bHitTesting)
				{
					Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, LOC_INPUT, i) );
				}
				FColor ConnectorColor = ObjInfo.Inputs[i].bEnabled ? ObjInfo.Inputs[i].Color : FColor(DisabledColor);
				DrawTile( Canvas, Pos.X - LO_CONNECTOR_LENGTH, LinkY - LO_CONNECTOR_WIDTH / 2, LO_CONNECTOR_LENGTH, LO_CONNECTOR_WIDTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ConnectorColor );
				if(bHitTesting)
				{
					Canvas->SetHitProxy( NULL );
				}

				StringSize( NormalFont, XL, YL, *ObjInfo.Inputs[i].Name );
				const FIntPoint StringPos( Pos.X + LO_TEXT_BORDER, LinkY - YL / 2 );
				if ( AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) ) 
				{
					if ( ConnectorTileBackgroundColor )
					{
						DrawTile( Canvas, StringPos.X, StringPos.Y, XL, YL, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), *ConnectorTileBackgroundColor, NULL, true );
					}
					const FLinearColor& TextColor = ObjInfo.Inputs[i].bEnabled ? FLinearColor::White : DisabledTextColor;
					DrawShadowedString( Canvas, StringPos.X, StringPos.Y, *ObjInfo.Inputs[i].Name, NormalFont, TextColor );
				}
			}
		}
	}

	// Do nothing if no Output connectors
	if( ObjInfo.Outputs.Num() > 0 )
	{
		const int32 SpacingY	= ConnectorRangeY/ObjInfo.Outputs.Num();
		const int32 StartY	= CenterY - (ObjInfo.Outputs.Num()-1) * SpacingY / 2;
		ObjInfo.OutputY.AddUninitialized( ObjInfo.Outputs.Num() );

		for(int32 i=0; i<ObjInfo.Outputs.Num(); i++)
		{
			const int32 LinkY		= StartY + i * SpacingY;
			ObjInfo.OutputY[i]	= LinkY;

			if ( bSufficientlyZoomedIn )
			{
				if(bHitTesting)
				{
					Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, LOC_OUTPUT, i) );
				}
				FColor ConnectorColor = ObjInfo.Outputs[i].bEnabled ? ObjInfo.Outputs[i].Color : FColor(DisabledColor);
				DrawTile( Canvas, Pos.X + Size.X, LinkY - LO_CONNECTOR_WIDTH / 2, LO_CONNECTOR_LENGTH, LO_CONNECTOR_WIDTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ConnectorColor );
				if(bHitTesting)
				{
					Canvas->SetHitProxy( NULL );
				}

				StringSize( NormalFont, XL, YL, *ObjInfo.Outputs[i].Name );
				const FIntPoint StringPos( Pos.X + Size.X - XL - LO_TEXT_BORDER, LinkY - YL / 2 );
				if ( AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) )
				{
					if ( ConnectorTileBackgroundColor )
					{
						DrawTile( Canvas, StringPos.X, StringPos.Y, XL, YL, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), *ConnectorTileBackgroundColor, NULL, true );
					}
					const FLinearColor& TextColor = ObjInfo.Outputs[i].bEnabled ? FLinearColor::White : DisabledTextColor;
					DrawShadowedString( Canvas, StringPos.X, StringPos.Y, *ObjInfo.Outputs[i].Name, NormalFont, TextColor );
				}
			}
		}
	}
}

/** 
 * Draws logic connectors on the sequence object with adjustments for moving connectors.
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
void FLinkedObjDrawUtils::DrawLogicConnectorsWithMoving(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const FLinearColor* ConnectorTileBackgroundColor, const bool bHaveMovingInput, const bool bHaveMovingOutput, const bool bGhostNonMoving )
{
	const bool bHitTesting				= Canvas->IsHitTesting();
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	const bool bSufficientlyZoomedIn	= Zoom2D > ConnectorMinZoom;

	int32 XL, YL;
	StringSize(NormalFont, XL, YL, TEXT("GgIhy"));

	const int32 ConnectorWidth	= FMath::Max(YL, LO_CONNECTOR_WIDTH);
	const int32 ConnectorRangeY	= Size.Y - 2*LO_TEXT_BORDER;
	const int32 CenterY			= Pos.Y + LO_TEXT_BORDER + ConnectorRangeY / 2;

	const FLinearColor GhostedConnectorColor(.2f, .2f, .2f);
	const FLinearColor GhostedTextColor(.6f, .6f, .6f);

	// Do nothing if no Input connectors
	if( ObjInfo.Inputs.Num() > 0 )
	{
		// Return as many Y positions as we have connectors
		ObjInfo.InputY.AddUninitialized( ObjInfo.Inputs.Num() );

		// Pre-calculate all positions.
		// Every connector needs to have a known position so we can sort their locations and properly place interp actors.
		TArray< FConnectorPlacementData > PlacementData;
		PreCalculateLogicConnectorPositions( PlacementData, ObjInfo, true, Pos, Size );

		if( !bHaveMovingInput && ObjInfo.bPendingInputConnectorRecalc == true )
		{
			// If we don't have a moving connector and we need to recalc connector positions do it now
			AdjustForOverlap( PlacementData, Pos, Size, 0, true);
			ObjInfo.bPendingInputConnectorRecalc = false;
		}

		// Now actually do the drawing
		for(int32 InputIdx=0; InputIdx < ObjInfo.Inputs.Num(); ++InputIdx )
		{
			FLinkedObjConnInfo& InputInfo = ObjInfo.Inputs[InputIdx];
			// Get the placement data from the same index as the variable
			const FConnectorPlacementData& CurData = PlacementData[InputIdx];

			const int32 DrawY = CurData.Pos;
			ObjInfo.InputY[InputIdx] = DrawY;
			// pass back the important data to the connector so it can be saved per draw call
			InputInfo.OverrideDelta = CurData.OverrideDelta;
			InputInfo.bClampedMax = CurData.bClampedMax;
			InputInfo.bClampedMin = CurData.bClampedMin;

			if ( bSufficientlyZoomedIn )
			{
				FLinearColor ConnColor = InputInfo.Color;
				FLinearColor TextColor = FLinearColor::White;

				if( bGhostNonMoving && InputInfo.bMoving == false )
				{
					// The color of non-moving connectors and their names should be ghosted if we are moving connectors
					ConnColor = GhostedConnectorColor;
					TextColor = GhostedTextColor;
				}

				if(bHitTesting)
				{
					Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, LOC_INPUT, InputIdx) );
				}
		
				DrawTile( Canvas, Pos.X - LO_CONNECTOR_LENGTH, DrawY - LO_CONNECTOR_WIDTH / 2, LO_CONNECTOR_LENGTH, LO_CONNECTOR_WIDTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ConnColor);

				if(bHitTesting) 
				{
					Canvas->SetHitProxy( NULL );
				}

				StringSize( NormalFont, XL, YL, *InputInfo.Name );
				const FIntPoint StringPos( Pos.X + LO_TEXT_BORDER, DrawY - YL / 2 );
				if ( AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) ) 
				{
					if ( ConnectorTileBackgroundColor )
					{
						FLinearColor BgColor = *ConnectorTileBackgroundColor;
						if( bGhostNonMoving )
						{
							// All non moving connectors should be ghosted
							BgColor = GhostedConnectorColor;
						}

						DrawTile( Canvas, StringPos.X, StringPos.Y, XL, YL, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), BgColor, NULL, true );
					}

					if( bGhostNonMoving && !InputInfo.bMoving )
					{
						DrawString( Canvas, StringPos.X, StringPos.Y, InputInfo.Name, NormalFont, TextColor );
					}
					else
					{
						DrawShadowedString( Canvas, StringPos.X, StringPos.Y, InputInfo.Name, NormalFont, TextColor );
					}
				}
			}
		}
	}

	// Do nothing if no Output connectors
	if( ObjInfo.Outputs.Num() > 0 )
	{
		ObjInfo.OutputY.AddUninitialized( ObjInfo.Outputs.Num() );
		// Pre-calculate all positions.
		// Every connector needs to have a known position so we can sort their locations and properly place interp actors.
		TArray< FConnectorPlacementData > PlacementData;
		PreCalculateLogicConnectorPositions( PlacementData, ObjInfo, false, Pos, Size );

		if( !bHaveMovingOutput && ObjInfo.bPendingOutputConnectorRecalc == true )
		{
			// If we don't have a moving connector and we need to recalc connector positions do it now
			AdjustForOverlap( PlacementData, Pos, Size, 0, true);
			ObjInfo.bPendingOutputConnectorRecalc = false;
		}

		// Now actually do the drawing
		for( int32 OutputIdx=0; OutputIdx<ObjInfo.Outputs.Num(); ++OutputIdx )
		{
			FLinkedObjConnInfo& OutputInfo = ObjInfo.Outputs[OutputIdx];
			// Get the placement data from the same index as the variable
			const FConnectorPlacementData& CurData = PlacementData[OutputIdx];

			const int32 DrawY = CurData.Pos;
			ObjInfo.OutputY[OutputIdx] = DrawY;
			// pass back the important data to the connector so it can be saved per draw call
			OutputInfo.OverrideDelta = CurData.OverrideDelta;
			OutputInfo.bClampedMax = CurData.bClampedMax;
			OutputInfo.bClampedMin = CurData.bClampedMin;
		
			if ( bSufficientlyZoomedIn )
			{
				FLinearColor ConnColor = OutputInfo.Color;
				FLinearColor TextColor = FLinearColor::White;
				
				if( bGhostNonMoving && OutputInfo.bMoving == false )
				{
					// The color of non-moving connectors and their names should be ghosted if we are moving connectors
					ConnColor = GhostedConnectorColor;
					TextColor = GhostedTextColor;
				}

				if( bHitTesting )
				{
					Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, LOC_OUTPUT, OutputIdx) );
				}

				// Draw the connector tile
				DrawTile( Canvas, Pos.X + Size.X, DrawY - LO_CONNECTOR_WIDTH / 2, LO_CONNECTOR_LENGTH, LO_CONNECTOR_WIDTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ConnColor);
				if( bHitTesting )
				{
					Canvas->SetHitProxy( NULL );
				}

				StringSize( NormalFont, XL, YL, *OutputInfo.Name );
				const FIntPoint StringPos( Pos.X + Size.X - XL - LO_TEXT_BORDER, DrawY - YL / 2 );
				if ( AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) )
				{
					if ( ConnectorTileBackgroundColor )
					{
						FLinearColor BgColor = *ConnectorTileBackgroundColor;
						if( bGhostNonMoving && OutputInfo.bMoving == false )
						{
							BgColor = GhostedConnectorColor;
						}
						DrawTile( Canvas, StringPos.X, StringPos.Y, XL, YL, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), BgColor, NULL, true );
					}

					// Draw the connector name
					if( bGhostNonMoving && !OutputInfo.bMoving )
					{
						DrawString( Canvas, StringPos.X, StringPos.Y, OutputInfo.Name, NormalFont, TextColor );
					}
					else
					{
						DrawShadowedString( Canvas, StringPos.X, StringPos.Y, OutputInfo.Name, NormalFont, TextColor );
					}
				}
			}
		}
	}
}

/**
 * Special version of string size that handles unique wrapping of variable names.
 */
static bool VarStringSize(UFont *Font, int32 &XL, int32 &YL, FString Text, FString *LeftSplit = NULL, int32 *LeftXL = NULL, FString *RightSplit = NULL, int32 *RightXL = NULL)
{
	if (Text.Len() >= 4)
	{
		// walk through the string and find the wrap point (skip the first few chars since wrapping early would be pointless)
		for (int32 Idx = 4; Idx < Text.Len(); Idx++)
		{
			TCHAR TextChar = Text[Idx];
			if (TextChar == ' ' || FChar::IsUpper(TextChar))
			{
				// found wrap point, find the size of the first string
				FString FirstPart = Text.Left(Idx);
				FString SecondPart = Text.Right(Text.Len() - Idx);
				int32 FirstXL, FirstYL, SecondXL, SecondYL;
				StringSize(Font, FirstXL, FirstYL, *FirstPart);
				StringSize(Font, SecondXL, SecondYL, *SecondPart);
				XL = FMath::Max(FirstXL, SecondXL);
				YL = FirstYL + SecondYL;
				if (LeftSplit != NULL)
				{
					*LeftSplit = FirstPart;
				}
				if (LeftXL != NULL)
				{
					*LeftXL = FirstXL;
				}
				if (RightSplit != NULL)
				{
					*RightSplit = SecondPart;
				}
				if (RightXL != NULL)
				{
					*RightXL = SecondXL;
				}
				return true;
			}
		}
	}
	// no wrap, normal size
	StringSize(Font, XL, YL,*Text);
	return false;
}

FIntPoint FLinkedObjDrawUtils::GetVariableConnectorsSize(FCanvas* Canvas, const FLinkedObjDrawInfo& ObjInfo)
{
	// sum the length of all the var/event names and add extra padding
	int32 TotalXL = 0, MaxYL = 0;
	for (int32 Idx = 0; Idx < ObjInfo.Variables.Num(); Idx++)
	{
		int32 XL, YL;
		VarStringSize( NormalFont, XL, YL, ObjInfo.Variables[Idx].Name );
		TotalXL += XL;
		MaxYL = FMath::Max(MaxYL,YL);
	}
	for (int32 Idx = 0; Idx < ObjInfo.Events.Num(); Idx++)
	{
		int32 XL, YL;
		VarStringSize( NormalFont, XL, YL, ObjInfo.Events[Idx].Name );
		TotalXL += XL;
		MaxYL = FMath::Max(MaxYL,YL);
	}
	// add the final padding based on number of connectors
	TotalXL += (2 * LO_DESC_X_PADDING) * (ObjInfo.Variables.Num() + ObjInfo.Events.Num()) + (2 * LO_DESC_X_PADDING);
	return FIntPoint(TotalXL,MaxYL);
}

/**
 * Pre-Calculates variable connector positions based on their current delta offsets.
 * We need all the positions of connectors upfront so we can sort them based on position later.
 *
 * @param OutPlacementData	An array of connector placement data that is poptulated
 * @param ObjInfo		Information about the sequence object were the output connectors reside
 * @param Pos			The position of the sequence object where these connectors reside
 * @param Size			The size of the sequence object where these connectors reside.
 * @param InFont        Font to use
 */
static void PreCalculateVariableConnectorPositions( TArray< FConnectorPlacementData >& OutPlacementData, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const int32 VarWidth, UFont* InFont )
{
	// The position of the last connector
	int32 LastPos = Pos.X;
	// The spacing of the last connector
	int32 LastSpacing = 0;

	// if the title is wider than needed for the variables
	if (VarWidth < Size.X)
	{
		// then center the variables
		LastPos += (Size.X - VarWidth)/2;
	}

	// String split and spacing info
	FString LeftSplit, RightSplit;
	int32 LeftXSpacing, RightXSpacing;

	// Pre-calculate all connections.  We need to know the positions of each connector 
	// before they are drawn so we can position them correctly if one moved
	// We will adjust for overlapping connectors if need be.
	// 
	// Calculation is done by using the position of the previous connector and adding spacing for
	// the current connector's name plus some padding
	// We only worry about the X position of variable connectors since the Y position is fixed
	for (int32 Idx = 0; Idx < ObjInfo.Variables.Num(); Idx++)
	{
		// The current variable connector info
		FLinkedObjConnInfo& VarInfo = ObjInfo.Variables[Idx];

		// Figure out how much space the name of the connector takes up takes up.
		int32 XL, YL;
		bool bSplit = VarStringSize( InFont, XL, YL, VarInfo.Name, &LeftSplit, &LeftXSpacing, &RightSplit, &RightXSpacing );

		// Set up some placement data for the connector 
		FConnectorPlacementData ConnData; 

		if( VarInfo.bNewConnection == true && VarInfo.bMoving == false )
		{
			// This is a new connection.  It should be positioned at the end of all the connectors
			const int32 NewLoc = LastPos + LastSpacing + (LO_DESC_X_PADDING * 2);
			
			// Find out the change in position from the sequence objects left edge. The left edge is used as zero for the Delta override )
			VarInfo.OverrideDelta = NewLoc - Pos.X;

			// Recalc all connector positions if one was added.
			// Store it on the ObjInfo struct so it can be passed back to the sequence object.
			// This way we can defer this recalculation.
			ObjInfo.bPendingVarConnectorRecalc = true;
		}
		else if( VarInfo.bMoving == true )
		{
			// Recalc all connector positions if one was moved.
			// Store it on the ObjInfo struct so it can be passed back to the sequence object.
			// This way we can defer this recalculation until when we don't have an interp actor
			ObjInfo.bPendingVarConnectorRecalc = true;
		}

		// The new location to draw
		int32 DrawX = Pos.X + VarInfo.OverrideDelta;
		
		// Setup placement data for this connector.
		// Index of the connector. This is used to lookup the correct index later because the call to AdjustForOverlap sorts the connectors by position
		ConnData.Index = Idx; 
		ConnData.ConnType = LOC_VARIABLE;
		ConnData.Pos = DrawX;
		ConnData.OverrideDelta = VarInfo.OverrideDelta;
		ConnData.Spacing = FIntPoint(XL,YL);

		if( VarInfo.bMoving )
		{
			// Make sure the connector doesn't go over the edge of the sequence object
			ClampConnector( ConnData, Pos.X, Pos.X + Size.X - XL );
		}

		OutPlacementData.Add(ConnData);
		// Update the position and spacing for the next connector
		LastPos = DrawX;
		LastSpacing = XL;
	}

	// calculate event connectors.
	for (int32 Idx = 0; Idx < ObjInfo.Events.Num(); Idx++)
	{
		// The current variable connector info
		FLinkedObjConnInfo& EventInfo = ObjInfo.Events[Idx];

		// Figure out how much space the name of the connector takes up takes up.
		int32 XL, YL;
		VarStringSize( InFont, XL, YL, EventInfo.Name );

		// Set up some placement data for the connector 
		FConnectorPlacementData ConnData; 

		if( EventInfo.bNewConnection == true && EventInfo.bMoving == false )
		{
			// This is a new connection.  It should be positioned at the end of all the connectors
			const int32 NewLoc = LastPos + LastSpacing + (LO_DESC_X_PADDING * 2);
			
			// Find out the change in position from the sequence objects left edge. The left edge is used as zero for the Delta override )
			EventInfo.OverrideDelta = NewLoc - Pos.X;

			// Recalc all connector positions if one was added.
			// Store it on the ObjInfo struct so it can be passed back to the sequence object.
			// This way we can defer this recalculation.
			ObjInfo.bPendingVarConnectorRecalc = true;
		}
		else if( EventInfo.bMoving == true )
		{
			// Recalc all connector positions if one was moved.
			// Store it on the ObjInfo struct so it can be passed back to the sequence object.
			// This way we can defer this recalculation until when we don't have an interp actor
			ObjInfo.bPendingVarConnectorRecalc = true;
		}

		// The new location to draw
		int32 DrawX = Pos.X + EventInfo.OverrideDelta;
		
	
		// Index of the connector. This is used to lookup the correct index later because the call to AdjustForOverlap sorts the connectors by position
		ConnData.Index = Idx; 
		ConnData.ConnType = LOC_EVENT;
		// This is index offset we need to add when looking up the original index of this connector
		// This is needed since event variable connectors reside in a different array but must be sorted together with variable connectors.
		// A variable and event connector can have the same index since they are in different arrays, which is why we need the offset.
		ConnData.EventOffset = ObjInfo.Variables.Num();
		ConnData.Pos = DrawX;
		ConnData.OverrideDelta = EventInfo.OverrideDelta;
		ConnData.Spacing = FIntPoint(XL,YL);

		if( EventInfo.bMoving )
		{
			// Make sure the connector doesn't go over the edge of the sequence object
			ClampConnector( ConnData, Pos.X, Pos.X + Size.X - XL );
		}

		OutPlacementData.Add(ConnData);
		// Update the position and spacing for the next connector
		LastPos = DrawX;
		LastSpacing = XL;
	}
}

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
void FLinkedObjDrawUtils::DrawVariableConnectorsWithMoving(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const int32 VarWidth, const bool bHaveMovingVariable, const bool bGhostNonMoving )
{
	// Do nothing here if no variables or event connectors.
	if( ObjInfo.Variables.Num() == 0 && ObjInfo.Events.Num() == 0 )
	{
		return;
	}

	const FLinearColor GhostedConnectorColor(.2f, .2f, .2f);
	const FLinearColor GhostedTextColor(.6f, .6f, .6f);

	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	const bool bHitTesting = Canvas->IsHitTesting();
	const bool bSufficientlyZoomedIn = Zoom2D > ConnectorMinZoom;
	const int32 LabelY = Pos.Y - LO_TEXT_BORDER;
	
	// Return as many x locations as there are variables and events
	ObjInfo.VariableX.AddUninitialized( ObjInfo.Variables.Num() );
	ObjInfo.EventX.AddUninitialized( ObjInfo.Events.Num() );

	// Initialize placement data
	TArray<FConnectorPlacementData> PlacementData;

	PreCalculateVariableConnectorPositions( PlacementData, ObjInfo, Pos, Size, VarWidth, NormalFont );

	if( !bHaveMovingVariable && ObjInfo.bPendingVarConnectorRecalc == true )
	{
		// If we don't have a moving connector and we need to recalc connector positions do it now
		AdjustForOverlap( PlacementData, Pos, Size, VarWidth, false);
		ObjInfo.bPendingVarConnectorRecalc = false;
	}

	FString LeftSplit, RightSplit;
	int32 LeftXSpacing=0,RightXSpacing=0;
	// Now actually do the drawing
	for (int32 DataIdx = 0; DataIdx < PlacementData.Num(); ++DataIdx)
	{
		const FConnectorPlacementData& CurData = PlacementData[ DataIdx ];
		EConnectorHitProxyType ConnType = CurData.ConnType;
		// Get the variable data from the same index as the placement data
		FLinkedObjConnInfo& VarInfo = ( ConnType == LOC_VARIABLE ? ObjInfo.Variables[ CurData.Index ] : ObjInfo.Events[ CurData.Index ] );

		bool bSplit = false;
		if( ConnType == LOC_VARIABLE)
		{
			// Calculate the string size to get split string and spacing info
			int32 XL, YL;
			bSplit = VarStringSize( NormalFont, XL, YL, VarInfo.Name, &LeftSplit, &LeftXSpacing, &RightSplit, &RightXSpacing );
		}

		// The x location of where to draw
		const int32 DrawX = CurData.Pos;

		// Calculate this only once
		const int32 HalfXL = CurData.Spacing.X/2;
		
		// Pass back this info to the sequence object so it can be stored and saved
		// We use the midpoint of the connector as the X location to pass back so the arrow of anything linked to the connector will show up in the middle
		if( ConnType == LOC_VARIABLE )
		{
			ObjInfo.VariableX[ CurData.Index ] = DrawX + HalfXL; 
		}
		else 
		{
			ObjInfo.EventX[ CurData.Index ] = DrawX + HalfXL;
		}

		VarInfo.OverrideDelta = CurData.OverrideDelta;
		VarInfo.bClampedMax = CurData.bClampedMax;
		VarInfo.bClampedMin = CurData.bClampedMin;

		// Only draw if we are zoomed in close enough
		if ( bSufficientlyZoomedIn )
		{
			// The connector color
			FLinearColor ConnColor = VarInfo.Color;
			// The color of the connector text
			FLinearColor TextColor = FLinearColor::White;
			if( bGhostNonMoving && VarInfo.bMoving == false )
			{
				// If we are moving, gray everything out (unless we are the interp actor!)
				ConnColor = GhostedConnectorColor;
				TextColor = GhostedTextColor;
			}

			// Set up hit proxy info
			if( bHitTesting )
			{
				Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, ConnType, CurData.Index ) );
			}

			if (VarInfo.bOutput)
			{
				// Draw a triangle if this is an output variable
				FIntPoint Vert0, Vert1, Vert2;
				Vert0.X = -2 + DrawX  + HalfXL - LO_CONNECTOR_WIDTH/2;
				Vert0.Y = Pos.Y + Size.Y;
				Vert1.X = Vert0.X + LO_CONNECTOR_WIDTH + 2;
				Vert1.Y = Vert0.Y;
				Vert2.X = (Vert1.X + Vert0.X) / 2;
				Vert2.Y = Vert0.Y + LO_CONNECTOR_LENGTH + 2;
				
				FCanvasTriangleItem TriItem( Vert0, Vert1, Vert2, GWhiteTexture );
				TriItem.SetColor( ConnColor );
				Canvas->DrawItem( TriItem );
			}
			else
			{
				// Draw the connector square
				DrawTile( Canvas, DrawX + HalfXL - LO_CONNECTOR_WIDTH / 2, Pos.Y+Size.Y, LO_CONNECTOR_WIDTH, LO_CONNECTOR_LENGTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ConnColor );
			}

			if(bHitTesting)
			{
				Canvas->SetHitProxy( NULL );
			}

			if ( AABBLiesWithinViewport( Canvas, DrawX, LabelY, CurData.Spacing.X, CurData.Spacing.Y ) )
			{
				//Draw strings for the connector name
				if ( bSplit )
				{
					if( bGhostNonMoving && !VarInfo.bMoving )
					{
						// Don't draw a shadowed string if this one is grayed out
						FCanvasTextItem TextItem( FVector2D( DrawX + HalfXL - RightXSpacing/2, LabelY + CurData.Spacing.Y/2 ), FText::FromString( RightSplit ), NormalFont, TextColor );
						TextItem.EnableShadow( FLinearColor::Black );

						Canvas->DrawItem( TextItem );
						TextItem.Position = FVector2D( DrawX + HalfXL - LeftXSpacing/2, LabelY );
						TextItem.Text = FText::FromString( LeftSplit );
						Canvas->DrawItem( TextItem );
					}
					else
					{
						DrawShadowedString( Canvas, DrawX + HalfXL - RightXSpacing/2, LabelY + CurData.Spacing.Y/2, *RightSplit, NormalFont, TextColor );
						DrawShadowedString( Canvas, DrawX + HalfXL - LeftXSpacing/2, LabelY, *LeftSplit, NormalFont, TextColor );
					}
				}
				else
				{
					if( bGhostNonMoving && !VarInfo.bMoving )
					{
						// Don't draw a shadowed string if this one is grayed out
						FCanvasTextItem TextItem( FVector2D( DrawX, LabelY ), FText::FromString( VarInfo.Name ), NormalFont, TextColor );
						Canvas->DrawItem( TextItem );
					}
					else
					{
						DrawShadowedString( Canvas, DrawX, LabelY, *VarInfo.Name, NormalFont, TextColor);
					}
				}
			}
		}
	}

}

void FLinkedObjDrawUtils::DrawVariableConnectors(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size, const int32 VarWidth)
{
	// Do nothing here if no variables or event connectors.
	if( ObjInfo.Variables.Num() == 0 && ObjInfo.Events.Num() == 0 )
	{
		return;
	}
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	const bool bHitTesting = Canvas->IsHitTesting();
	const bool bSufficientlyZoomedIn = Zoom2D > ConnectorMinZoom;
	const int32 LabelY = Pos.Y - LO_TEXT_BORDER;

	int32 LastX = Pos.X, LastXL = 0;
	// if the title is wider than needed for the variables
	if (VarWidth < Size.X)
	{
		// then center the variables
		LastX += (Size.X - VarWidth)/2;
	}
	ObjInfo.VariableX.AddUninitialized(ObjInfo.Variables.Num());
	FString LeftSplit, RightSplit;
	int32 LeftXL, RightXL;
	for (int32 Idx = 0; Idx < ObjInfo.Variables.Num(); Idx++)
	{
		int32 VarX = LastX + LastXL + (LO_DESC_X_PADDING * 2);
		int32 XL, YL;
		bool bSplit = VarStringSize( NormalFont, XL, YL, ObjInfo.Variables[Idx].Name, &LeftSplit, &LeftXL, &RightSplit, &RightXL );
		ObjInfo.VariableX[Idx] = VarX + XL/2;
		if ( bSufficientlyZoomedIn )
		{
			if(bHitTesting) Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, LOC_VARIABLE, Idx) );
			if (ObjInfo.Variables[Idx].bOutput)
			{
				FIntPoint Vert0, Vert1, Vert2;
				Vert0.X = -2 + VarX + XL/2 - LO_CONNECTOR_WIDTH/2;
				Vert0.Y = Pos.Y + Size.Y;
				Vert1.X = Vert0.X + LO_CONNECTOR_WIDTH + 2;
				Vert1.Y = Vert0.Y;
				Vert2.X = (Vert1.X + Vert0.X) / 2;
				Vert2.Y = Vert0.Y + LO_CONNECTOR_LENGTH + 2;

				FCanvasTriangleItem TriItem( Vert0, Vert1, Vert2, GWhiteTexture );
				TriItem.SetColor( ObjInfo.Variables[Idx].Color );
				Canvas->DrawItem( TriItem );
			}
			else
			{
				DrawTile( Canvas, VarX + XL/2 - LO_CONNECTOR_WIDTH / 2, Pos.Y + Size.Y, LO_CONNECTOR_WIDTH, LO_CONNECTOR_LENGTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ObjInfo.Variables[Idx].Color );
			}
			if(bHitTesting)
			{
				Canvas->SetHitProxy( NULL );
			}


			if ( AABBLiesWithinViewport( Canvas, VarX, LabelY, XL, YL ) )
			{
				if (bSplit)
				{
					DrawShadowedString( Canvas, VarX + XL/2 - RightXL/2, LabelY + YL/2, *RightSplit, NormalFont, FLinearColor::White );
					DrawShadowedString( Canvas, VarX + XL/2 - LeftXL/2, LabelY, *LeftSplit, NormalFont, FLinearColor::White );
				}
				else
				{
					DrawShadowedString( Canvas, VarX, LabelY, *ObjInfo.Variables[Idx].Name, NormalFont, FLinearColor::White );
				}
			}
		}
		LastX = VarX;
		LastXL = XL;
	}
	// Draw event connectors.
	ObjInfo.EventX.AddUninitialized( ObjInfo.Events.Num() );
	for (int32 Idx = 0; Idx < ObjInfo.Events.Num(); Idx++)
	{
		int32 VarX = LastX + LastXL + (LO_DESC_X_PADDING * 2);
		int32 XL, YL;
		VarStringSize( NormalFont, XL, YL, ObjInfo.Events[Idx].Name );
		ObjInfo.EventX[Idx]	= VarX + XL/2;

		if ( bSufficientlyZoomedIn )
		{
			if(bHitTesting)
			{
				Canvas->SetHitProxy( new HLinkedObjConnectorProxy(ObjInfo.ObjObject, LOC_EVENT, Idx) );
			}
			DrawTile( Canvas, VarX + XL/2 - LO_CONNECTOR_WIDTH / 2, Pos.Y + Size.Y, LO_CONNECTOR_WIDTH, LO_CONNECTOR_LENGTH, FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), ObjInfo.Events[Idx].Color );
			if(bHitTesting)
			{
				Canvas->SetHitProxy( NULL );
			}

			if ( AABBLiesWithinViewport( Canvas, VarX, LabelY, XL, YL ) )
			{
				DrawShadowedString( Canvas, VarX, LabelY, *ObjInfo.Events[Idx].Name, NormalFont, FLinearColor::White );
			}
		}
		LastX = VarX;
		LastXL = XL;
	}
}

/** Draws connection and node tooltips. */
void FLinkedObjDrawUtils::DrawToolTips(FCanvas* Canvas, const FLinkedObjDrawInfo& ObjInfo, const FIntPoint& Pos, const FIntPoint& Size)
{
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	const bool bSufficientlyZoomedIn	= Zoom2D > ConnectorMinZoom;

	const int32 ConnectorRangeY	= Size.Y - 2*LO_TEXT_BORDER;
	const int32 CenterY			= Pos.Y + LO_TEXT_BORDER + ConnectorRangeY / 2;

	// These can be exposed via function parameters if needed
	const FColor ToolTipTextColor(255, 255, 255);
	const FColor ToolTipBackgroundColor(140,140,140);
	const int32 ToolTipBackgroundBorder = 3;

	// Draw tooltips for the node
	if (bSufficientlyZoomedIn && ObjInfo.ToolTips.Num() > 0)
	{
		TArray<FIntPoint> ToolTipPositions;
		ToolTipPositions.Empty(ObjInfo.ToolTips.Num());
		FIntRect ToolTipBounds(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

		// Calculate the tooltip string's bounds
		for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.ToolTips.Num(); ToolTipIndex++)
		{
			FIntPoint StringDim;
			const FString& ToolTip = ObjInfo.ToolTips[ToolTipIndex];
			StringSize(NormalFont, StringDim.X, StringDim.Y, *ToolTip);

			FIntPoint StringPos(Pos.X - 30, Pos.Y + Size.Y - 5 + ToolTipIndex * 17);
			ToolTipPositions.Add(StringPos);
			ToolTipBounds.Min.X = FMath::Min(ToolTipBounds.Min.X, StringPos.X);
			ToolTipBounds.Min.Y = FMath::Min(ToolTipBounds.Min.Y, StringPos.Y);
			ToolTipBounds.Max.X = FMath::Max(ToolTipBounds.Max.X, StringPos.X + StringDim.X);
			ToolTipBounds.Max.Y = FMath::Max(ToolTipBounds.Max.Y, StringPos.Y + StringDim.Y);
		}

		const int32 BackgroundX = ToolTipBounds.Min.X - ToolTipBackgroundBorder;
		const int32 BackgroundY = ToolTipBounds.Min.Y - ToolTipBackgroundBorder;
		const int32 BackgroundSizeX = ToolTipBounds.Max.X - ToolTipBounds.Min.X + ToolTipBackgroundBorder * 2;
		const int32 BackgroundSizeY = ToolTipBounds.Max.Y - ToolTipBounds.Min.Y + ToolTipBackgroundBorder * 2;

		FCanvasTileItem TileItem( FVector2D( BackgroundX, BackgroundY ), GWhiteTexture, FVector2D( BackgroundSizeX, BackgroundSizeY ), FLinearColor::Black );
		Canvas->DrawItem( TileItem );

		TileItem.Size = FVector2D( BackgroundSizeX - 2, BackgroundSizeY - 2 );
		TileItem.SetColor( ToolTipBackgroundColor );
		Canvas->DrawItem( TileItem, BackgroundX + 1, BackgroundY + 1 );

		// Draw the tooltip strings
		for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.ToolTips.Num(); ToolTipIndex++)
		{
			const FString& ToolTip = ObjInfo.ToolTips[ToolTipIndex];
			DrawShadowedString(Canvas, ToolTipPositions[ToolTipIndex].X, ToolTipPositions[ToolTipIndex].Y, *ToolTip, NormalFont, ToolTipTextColor);
		}
	}
	
	// Draw tooltips for connections on the left side of the node
	if( ObjInfo.Inputs.Num() > 0 )
	{
		const int32 SpacingY = ConnectorRangeY/ObjInfo.Inputs.Num();
		const int32 StartY = CenterY - (ObjInfo.Inputs.Num()-1) * SpacingY / 2;

		for(int32 i=0; i<ObjInfo.Inputs.Num(); i++)
		{
			const int32 LinkY	= StartY + i * SpacingY;

			if ( bSufficientlyZoomedIn && ObjInfo.Inputs[i].ToolTips.Num() > 0 )
			{
				TArray<FIntPoint> ToolTipPositions;
				ToolTipPositions.Empty(ObjInfo.Inputs[i].ToolTips.Num());
				FIntRect ToolTipBounds(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

				// Calculate the tooltip string's bounds
				for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.Inputs[i].ToolTips.Num(); ToolTipIndex++)
				{
					FIntPoint StringDim;
					const FString& ToolTip = ObjInfo.Inputs[i].ToolTips[ToolTipIndex];
					StringSize(NormalFont, StringDim.X, StringDim.Y, *ToolTip);

					FIntPoint StringPos(Pos.X - LO_CONNECTOR_LENGTH * 2 - 5, LinkY - LO_CONNECTOR_WIDTH / 2 + ToolTipIndex * 17);
					ToolTipPositions.Add(StringPos);
					ToolTipBounds.Min.X = FMath::Min(ToolTipBounds.Min.X, StringPos.X);
					ToolTipBounds.Min.Y = FMath::Min(ToolTipBounds.Min.Y, StringPos.Y);
					ToolTipBounds.Max.X = FMath::Max(ToolTipBounds.Max.X, StringPos.X + StringDim.X);
					ToolTipBounds.Max.Y = FMath::Max(ToolTipBounds.Max.Y, StringPos.Y + StringDim.Y);
				}

				// Move the bounds and string positions left, by how long the longest tooltip string is
				const int32 XOffset = ToolTipBounds.Max.X - ToolTipBounds.Min.X;
				for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.Inputs[i].ToolTips.Num(); ToolTipIndex++)
				{
					ToolTipPositions[ToolTipIndex].X -= XOffset;
				}
				ToolTipBounds.Min.X -= XOffset;
				ToolTipBounds.Max.X -= XOffset;

				const int32 BackgroundX = ToolTipBounds.Min.X - ToolTipBackgroundBorder;
				const int32 BackgroundY = ToolTipBounds.Min.Y - ToolTipBackgroundBorder;
				const int32 BackgroundSizeX = ToolTipBounds.Max.X - ToolTipBounds.Min.X + ToolTipBackgroundBorder * 2;
				const int32 BackgroundSizeY = ToolTipBounds.Max.Y - ToolTipBounds.Min.Y + ToolTipBackgroundBorder * 2;

				// Draw the black outline
				DrawTile(
					Canvas, 
					BackgroundX, BackgroundY, 
					BackgroundSizeX, BackgroundSizeY,	
					FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ),
					FColor(0, 0, 0));

				// Draw the background
				DrawTile(
					Canvas, 
					BackgroundX + 1, BackgroundY + 1, 
					BackgroundSizeX - 2, BackgroundSizeY - 2,	
					FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ),
					ToolTipBackgroundColor);

				for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.Inputs[i].ToolTips.Num(); ToolTipIndex++)
				{
					// Draw the tooltip strings
					const FString& ToolTip = ObjInfo.Inputs[i].ToolTips[ToolTipIndex];
					DrawShadowedString(Canvas, ToolTipPositions[ToolTipIndex].X, ToolTipPositions[ToolTipIndex].Y, *ToolTip, NormalFont, ToolTipTextColor);
				}
			}
		}
	}

	// Draw tooltips for connections on the right side of the node
	if( ObjInfo.Outputs.Num() > 0 )
	{
		const int32 SpacingY	= ConnectorRangeY/ObjInfo.Outputs.Num();
		const int32 StartY = CenterY - (ObjInfo.Outputs.Num()-1) * SpacingY / 2;

		for(int32 i=0; i<ObjInfo.Outputs.Num(); i++)
		{
			const int32 LinkY	= StartY + i * SpacingY;

			if ( bSufficientlyZoomedIn && ObjInfo.Outputs[i].ToolTips.Num() > 0 )
			{
				TArray<FIntPoint> ToolTipPositions;
				ToolTipPositions.Empty(ObjInfo.Outputs[i].ToolTips.Num());
				FIntRect ToolTipBounds(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

				// Calculate the tooltip string's bounds
				for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.Outputs[i].ToolTips.Num(); ToolTipIndex++)
				{
					FIntPoint StringDim;
					const FString& ToolTip = ObjInfo.Outputs[i].ToolTips[ToolTipIndex];
					StringSize(NormalFont, StringDim.X, StringDim.Y, *ToolTip);

					FIntPoint StringPos(Pos.X + Size.X + LO_CONNECTOR_LENGTH + 16, LinkY - LO_CONNECTOR_WIDTH / 2 + ToolTipIndex * 17);
					ToolTipPositions.Add(StringPos);
					ToolTipBounds.Min.X = FMath::Min(ToolTipBounds.Min.X, StringPos.X);
					ToolTipBounds.Min.Y = FMath::Min(ToolTipBounds.Min.Y, StringPos.Y);
					ToolTipBounds.Max.X = FMath::Max(ToolTipBounds.Max.X, StringPos.X + StringDim.X);
					ToolTipBounds.Max.Y = FMath::Max(ToolTipBounds.Max.Y, StringPos.Y + StringDim.Y);
				}

				const int32 BackgroundX = ToolTipBounds.Min.X - ToolTipBackgroundBorder;
				const int32 BackgroundY = ToolTipBounds.Min.Y - ToolTipBackgroundBorder;
				const int32 BackgroundSizeX = ToolTipBounds.Max.X - ToolTipBounds.Min.X + ToolTipBackgroundBorder * 2;
				const int32 BackgroundSizeY = ToolTipBounds.Max.Y - ToolTipBounds.Min.Y + ToolTipBackgroundBorder * 2;

				// Draw the black outline
				DrawTile(
					Canvas, 
					BackgroundX, BackgroundY, 
					BackgroundSizeX, BackgroundSizeY,	
					FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ),
					FColor(0, 0, 0));

				// Draw the background
				DrawTile(
					Canvas, 
					BackgroundX + 1, BackgroundY + 1, 
					BackgroundSizeX - 2, BackgroundSizeY - 2,	
					FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ),
					ToolTipBackgroundColor);

				for (int32 ToolTipIndex = 0; ToolTipIndex < ObjInfo.Outputs[i].ToolTips.Num(); ToolTipIndex++)
				{
					// Draw the tooltip strings
					const FString& ToolTip = ObjInfo.Outputs[i].ToolTips[ToolTipIndex];
					DrawShadowedString(Canvas, ToolTipPositions[ToolTipIndex].X, ToolTipPositions[ToolTipIndex].Y, *ToolTip, NormalFont, ToolTipTextColor);
				}
			}
		}
	}
}

void FLinkedObjDrawUtils::DrawLinkedObj(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const TCHAR* Name, const TCHAR* Comment, const FColor& BorderColor, const FColor& TitleBkgColor, const FIntPoint& Pos)
{
	const bool bHitTesting = Canvas->IsHitTesting();

	TArray<FString> Names;
	Names.Add(Name);

	const FIntPoint TitleSize			= GetTitleBarSize(Canvas, Names);
	const FIntPoint LogicSize			= GetLogicConnectorsSize(ObjInfo);
	const FIntPoint VarSize				= GetVariableConnectorsSize(Canvas, ObjInfo);
	const FIntPoint VisualizationSize	= ObjInfo.VisualizationSize;

	ObjInfo.DrawWidth	= FMath::Max(FMath::Max3(TitleSize.X, LogicSize.X, VarSize.X), VisualizationSize.X);
	ObjInfo.DrawHeight	= TitleSize.Y + LogicSize.Y + VarSize.Y + VisualizationSize.Y + 3;

	ObjInfo.VisualizationPosition = Pos + FIntPoint(0, TitleSize.Y + LogicSize.Y + 1);

	if(Canvas->IsHitTesting())
	{
		Canvas->SetHitProxy( new HLinkedObjProxy(ObjInfo.ObjObject) );
	}

	// Comment list
	TArray<FString> Comments;
	Comments.Add(FString(Comment));
	DrawTitleBar(Canvas, Pos, FIntPoint(ObjInfo.DrawWidth, TitleSize.Y), BorderColor, TitleBkgColor, Names, Comments);

	// Border
	FCanvasTileItem TileItem( FVector2D( Pos.X,	Pos.Y + TitleSize.Y + 1 ), GWhiteTexture, FVector2D( ObjInfo.DrawWidth, LogicSize.Y + VarSize.Y + VisualizationSize.Y), BorderColor );
	Canvas->DrawItem( TileItem );

	// Background
	TileItem.Position = FVector2D( Pos.X + 1,	Pos.Y + TitleSize.Y + 2 );
	TileItem.Size = FVector2D( ObjInfo.DrawWidth - 2, LogicSize.Y + VarSize.Y + VisualizationSize.Y - 2 );
	TileItem.SetColor( FColor(140,140,140) );
	Canvas->DrawItem( TileItem );

	// Drop shadow
	TileItem.Position = FVector2D( Pos.X, Pos.Y + TitleSize.Y + LogicSize.Y + VisualizationSize.Y );
	TileItem.Size = FVector2D( ObjInfo.DrawWidth - 2, 2 );
	TileItem.SetColor( BorderColor );
	Canvas->DrawItem( TileItem );
	
	if(Canvas->IsHitTesting())
	{
		Canvas->SetHitProxy( NULL );
	}

	DrawLogicConnectors(Canvas, ObjInfo, Pos + FIntPoint(0, TitleSize.Y + 1), FIntPoint(ObjInfo.DrawWidth, LogicSize.Y));
	DrawVariableConnectors(Canvas, ObjInfo, Pos + FIntPoint(0, TitleSize.Y + 1 + LogicSize.Y + 1 + VisualizationSize.Y), FIntPoint(ObjInfo.DrawWidth, VarSize.Y), VarSize.X);
}

// if the rendering changes, these need to change
int32 FLinkedObjDrawUtils::ComputeSliderHeight(int32 SliderWidth)
{
	return LO_SLIDER_HANDLE_HEIGHT+4;
}

int32 FLinkedObjDrawUtils::Compute2DSliderHeight(int32 SliderWidth)
{
	return SliderWidth;
}

int32 FLinkedObjDrawUtils::DrawSlider( FCanvas* Canvas, const FIntPoint& SliderPos, int32 SliderWidth, const FColor& BorderColor, const FColor& BackGroundColor, float SliderPosition, const FString& ValText, UObject* Obj, int32 SliderIndex , bool bDrawTextOnSide)
{
	const bool bHitTesting = Canvas->IsHitTesting();
	const int32 SliderBoxHeight = LO_SLIDER_HANDLE_HEIGHT + 4;

	if ( AABBLiesWithinViewport( Canvas, SliderPos.X, SliderPos.Y, SliderWidth, SliderBoxHeight ) )
	{
		const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
		const int32 SliderRange = (SliderWidth - 4 - LO_SLIDER_HANDLE_WIDTH);
		const int32 SliderHandlePosX = FMath::Trunc(SliderPos.X + 2 + (SliderPosition * SliderRange));

		if(bHitTesting)
		{
			Canvas->SetHitProxy( new HLinkedObjProxySpecial(Obj, SliderIndex) );
		}
		DrawTile( Canvas, SliderPos.X,		SliderPos.Y - 1,	SliderWidth,		SliderBoxHeight,		FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), BorderColor );
		DrawTile( Canvas, SliderPos.X + 1,	SliderPos.Y,		SliderWidth - 2,	SliderBoxHeight - 2,	FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), BackGroundColor );

		if ( Zoom2D > SliderMinZoom )
		{
			DrawTile( Canvas, SliderHandlePosX, SliderPos.Y + 1, LO_SLIDER_HANDLE_WIDTH, LO_SLIDER_HANDLE_HEIGHT, FVector2D( 0.f, 0.f), FVector2D( 1.f, 1.f ), SliderHandleColor );
		}
		if(bHitTesting) Canvas->SetHitProxy( NULL );
	}

	if(bDrawTextOnSide)
	{
		int32 SizeX, SizeY;
		StringSize(NormalFont, SizeX, SizeY, *ValText);

		const int32 PosX = SliderPos.X - 2 - SizeX; 
		const int32 PosY = SliderPos.Y + (SliderBoxHeight + 1 - SizeY)/2;
		if ( AABBLiesWithinViewport( Canvas, PosX, PosY, SizeX, SizeY ) )
		{
			DrawString( Canvas, PosX, PosY, ValText, NormalFont, FColor(0,0,0) );
		}
	}
	else
	{
		DrawString( Canvas, SliderPos.X + 2, SliderPos.Y + SliderBoxHeight + 1, ValText, NormalFont, FColor(0,0,0) );
	}
	return SliderBoxHeight;
}

int32 FLinkedObjDrawUtils::Draw2DSlider(FCanvas* Canvas, const FIntPoint &SliderPos, int32 SliderWidth, const FColor& BorderColor, const FColor& BackGroundColor, float SliderPositionX, float SliderPositionY, const FString &ValText, UObject *Obj, int32 SliderIndex, bool bDrawTextOnSide)
{
	const bool bHitTesting = Canvas->IsHitTesting();

	const int32 SliderBoxHeight = SliderWidth;

	if ( AABBLiesWithinViewport( Canvas, SliderPos.X, SliderPos.Y, SliderWidth, SliderBoxHeight ) )
	{
		const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
		const int32 SliderRangeX = (SliderWidth - 4 - LO_SLIDER_HANDLE_HEIGHT);
		const int32 SliderRangeY = (SliderBoxHeight - 4 - LO_SLIDER_HANDLE_HEIGHT);
		const int32 SliderHandlePosX = SliderPos.X + 2 + FMath::Trunc(SliderPositionX * SliderRangeX);
		const int32 SliderHandlePosY = SliderPos.Y + 2 + FMath::Trunc(SliderPositionY * SliderRangeY);

		if(bHitTesting) 
		{
			Canvas->SetHitProxy( new HLinkedObjProxySpecial(Obj, SliderIndex) );
		}
		DrawTile( Canvas, SliderPos.X,		SliderPos.Y - 1,	SliderWidth,		SliderBoxHeight,		FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), BorderColor );
		DrawTile( Canvas, SliderPos.X + 1,	SliderPos.Y,		SliderWidth - 2,	SliderBoxHeight - 2,	FVector2D( 0.f, 0.f), FVector2D( 0.f, 0.f ), BackGroundColor );

		if ( Zoom2D > SliderMinZoom )
		{
			DrawTile( Canvas, SliderHandlePosX, SliderHandlePosY, LO_SLIDER_HANDLE_HEIGHT, LO_SLIDER_HANDLE_HEIGHT, FVector2D( 0.f, 0.f), FVector2D( 1.f, 1.f ), SliderHandleColor );
		}
		if(bHitTesting)
		{
			Canvas->SetHitProxy( NULL );
		}
	}

	if(bDrawTextOnSide)
	{
		int32 SizeX, SizeY;
		StringSize(NormalFont, SizeX, SizeY, *ValText);
		const int32 PosX = SliderPos.X - 2 - SizeX;
		const int32 PosY = SliderPos.Y + (SliderBoxHeight + 1 - SizeY)/2;
		if ( AABBLiesWithinViewport( Canvas, PosX, PosY, SizeX, SizeY ) )
		{
			DrawString( Canvas, PosX, PosY, ValText, NormalFont, FColor(0,0,0));
		}
	}
	else
	{
		DrawString( Canvas, SliderPos.X + 2, SliderPos.Y + SliderBoxHeight + 1, ValText, NormalFont, FColor(0,0,0) );
	}

	return SliderBoxHeight;
}


/**
 * @return		true if the current viewport contains some portion of the specified AABB.
 */
bool FLinkedObjDrawUtils::AABBLiesWithinViewport(FCanvas* Canvas, float X, float Y, float SizeX, float SizeY)
{
	const FMatrix TransformMatrix = Canvas->GetTransform();
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());
	FRenderTarget* RenderTarget = Canvas->GetRenderTarget();
	if ( !RenderTarget )
	{
		return false;
	}

	// Transform the 2D point by the current transform matrix.
	FVector Point(X,Y, 0.0f);
	Point = TransformMatrix.TransformPosition(Point);
	X = Point.X;
	Y = Point.Y;

	// Check right side.
	if ( X > RenderTarget->GetSizeXY().X )
	{
		return false;
	}

	// Check left side.
	if ( X+SizeX*Zoom2D < 0.f )
	{
		return false;
	}

	// Check bottom side.
	if ( Y > RenderTarget->GetSizeXY().Y )
	{
		return false;
	}

	// Check top side.
	if ( Y+SizeY*Zoom2D < 0.f )
	{
		return false;
	}

	return true;
}

/**
 * Convenience function for filtering calls to FRenderInterface::DrawTile via AABBLiesWithinViewport.
 */
void FLinkedObjDrawUtils::DrawTile(FCanvas* Canvas,float X,float Y,float SizeX,float SizeY,const FVector2D& InUV0, const FVector2D& InUV1,const FLinearColor& Color,FTexture* InTexture,bool AlphaBlend)
{
	if ( AABBLiesWithinViewport( Canvas, X, Y, SizeX, SizeY ) )
	{
		FTexture* Texture = InTexture == NULL ? GWhiteTexture : InTexture;
		FCanvasTileItem TileItem( FVector2D( X, Y ), Texture, FVector2D( SizeX, SizeY ), InUV0, InUV1, Color );
		TileItem.BlendMode = AlphaBlend ? SE_BLEND_Translucent : SE_BLEND_Opaque;
		Canvas->DrawItem( TileItem );
	}
}

/**
 * Convenience function for filtering calls to FRenderInterface::DrawTile via AABBLiesWithinViewport.
 */
void FLinkedObjDrawUtils::DrawTile(FCanvas* Canvas,float X,float Y,float SizeX,float SizeY,const FVector2D& InUV0, const FVector2D& InUV1,FMaterialRenderProxy* MaterialRenderProxy)
{
	if ( AABBLiesWithinViewport( Canvas, X, Y, SizeX, SizeY ) )
	{
		FCanvasTileItem TileItem( FVector2D( X, Y ), MaterialRenderProxy, FVector2D( SizeX, SizeY ), InUV0, InUV1 );
		Canvas->DrawItem( TileItem );
	}
}

/**
 * Convenience function for filtering calls to FRenderInterface::DrawString through culling heuristics.
 */
int32 FLinkedObjDrawUtils::DrawString(FCanvas* Canvas,float StartX,float StartY,const FString& Text,UFont* Font,const FLinearColor& Color)
{
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());

	if ( Zoom2D > TextMinZoom )
	{
		FCanvasTextItem TextItem( FVector2D( StartX, StartY ), FText::FromString( Text ), Font, Color );
		Canvas->DrawItem( TextItem );
		return TextItem.DrawnSize.Y;
	}
	else
	{
		return 0;
	}
}

/**
 * Convenience function for filtering calls to FRenderInterface::DrawShadowedString through culling heuristics.
 */
int32 FLinkedObjDrawUtils::DrawShadowedString(FCanvas* Canvas,float StartX,float StartY, const FString& Text,UFont* Font,const FLinearColor& Color)
{
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());

	if ( Zoom2D > TextMinZoom )
	{
		FCanvasTextItem TextItem( FVector2D( StartX, StartY ), FText::FromString( Text ), Font, Color );
		if ( Zoom2D >= ( 1.f - DELTA ) )
		{
			TextItem.EnableShadow( FLinearColor::Black );
		}
		Canvas->DrawItem( TextItem );
		return TextItem.DrawnSize.Y;
	}
	else
	{
		return 0;
	}
}

/**
 * Convenience function for filtering calls to FRenderInterface::DrawStringZ through culling heuristics.
 */
int32 FLinkedObjDrawUtils::DisplayComment( FCanvas* Canvas, bool Draw, float CurX, float CurY, float Z, float& XL, float& YL, UFont* Font, const FString& Text, FLinearColor DrawColor )
{
	const float Zoom2D = GetUniformScaleFromMatrix(Canvas->GetTransform());

	if ( Zoom2D > TextMinZoom )
	{
		TArray<FWrappedStringElement> Lines;
		FTextSizingParameters RenderParms( 0.0f, 0.0f, XL, 0.0f, Font );
		UCanvas::WrapString( RenderParms, 0, *Text, Lines );

		if( Lines.Num() > 0 )
		{
			float StrHeight = Font->GetMaxCharHeight();
			float HeightTest = Canvas->GetRenderTarget()->GetSizeXY().Y;

			for( int32 Idx = 0; Idx < Lines.Num(); Idx++ )
			{
				const TCHAR* TextLine = *Lines[ Idx ].Value;
				
				DrawShadowedString( Canvas, CurX, CurY, TextLine, Font, DrawColor );
				
				CurY += StrHeight;
			}
		}

		return 1;
	}

	return 0;
}

/**
 * Takes a transformation matrix and returns a uniform scaling factor based on the 
 * length of the rotation vectors.
 */
float FLinkedObjDrawUtils::GetUniformScaleFromMatrix(const FMatrix &Matrix)
{
	const FVector XAxis = Matrix.GetScaledAxis( EAxis::X );
	const FVector YAxis = Matrix.GetScaledAxis( EAxis::Y );
	const FVector ZAxis = Matrix.GetScaledAxis( EAxis::Z );

	float Scale = FMath::Max(XAxis.Size(), YAxis.Size());
	Scale = FMath::Max(Scale, ZAxis.Size());

	return Scale;
}


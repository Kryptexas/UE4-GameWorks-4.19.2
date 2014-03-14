// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Canvas.cpp: Unreal canvas rendering.
=============================================================================*/

#include "EnginePrivate.h"
#include "CanvasItem.h"
#include "EngineUserInterfaceClasses.h"
#include "TileRendering.h"
#include "EngineMaterialClasses.h"
#include "RHIStaticStates.h"
#include "Engine.h"
#include "Internationalization.h"


DECLARE_CYCLE_STAT(TEXT("CanvasTileTextureItem Time"),STAT_Canvas_TileTextureItemTime,STATGROUP_Canvas);
DECLARE_CYCLE_STAT(TEXT("CanvasTileMaterialItem Time"),STAT_Canvas_TileMaterialItemTime,STATGROUP_Canvas);
DECLARE_CYCLE_STAT(TEXT("CanvasTextItem Time"),STAT_Canvas_TextItemTime,STATGROUP_Canvas);
DECLARE_CYCLE_STAT(TEXT("CanvasLineItem Time"),STAT_Canvas_LineItemTime,STATGROUP_Canvas);
DECLARE_CYCLE_STAT(TEXT("CanvasBoxItem Time"),STAT_Canvas_BoxItemTime,STATGROUP_Canvas);
DECLARE_CYCLE_STAT(TEXT("CanvasTriItem Time"),STAT_Canvas_TriItemTime,STATGROUP_Canvas);
DECLARE_CYCLE_STAT(TEXT("CanvasBorderItem Time"),STAT_Canvas_BorderItemTime,STATGROUP_Canvas);




#if WITH_EDITOR
#include "UnrealEd.h"
#include "ObjectTools.h"

#define LOCTEXT_NAMESPACE "CanvasItem"

FCanvasItemTestbed::LineVars FCanvasItemTestbed::TestLine;
bool FCanvasItemTestbed::bTestState = false;
bool FCanvasItemTestbed::bShowTestbed = false;

void FCanvasItemTestbed::Draw( class FViewport* Viewport, class FCanvas* Canvas )
{
	bTestState = !bTestState;

	if( !bShowTestbed )
	{
		return;
	}

	
	// A little ott for a testbed - but I wanted to draw several lines to ensure it worked :)
	if( TestLine.bTestSet == false )
	{
		TestLine.bTestSet = true;
		TestLine.LineStart.X = FMath::FRandRange( 0.0f, Viewport->GetSizeXY().X );
		TestLine.LineStart.Y = FMath::FRandRange( 0.0f, Viewport->GetSizeXY().Y );
		TestLine.LineEnd.X = FMath::FRandRange( 0.0f, Viewport->GetSizeXY().X );
		TestLine.LineEnd.Y  = FMath::FRandRange( 0.0f, Viewport->GetSizeXY().Y );
		TestLine.LineMove.X = FMath::FRandRange( 0.0f, 32.0f );
		TestLine.LineMove.Y = FMath::FRandRange( 0.0f, 32.0f );
		TestLine.LineMove2.X = FMath::FRandRange( 0.0f, 32.0f );
		TestLine.LineMove2.Y = FMath::FRandRange( 0.0f, 32.0f );
	}
	else
	{
		TestLine.LineStart += TestLine.LineMove;
		TestLine.LineEnd += TestLine.LineMove2;
		if( TestLine.LineStart.X < 0 )
		{
			TestLine.LineMove.X = -TestLine.LineMove.X;
		}
		if( TestLine.LineStart.Y < 0 )
		{
			TestLine.LineMove.Y = -TestLine.LineMove.Y;
		}
		if( TestLine.LineEnd.X < 0 )
		{
			TestLine.LineMove2.X = -TestLine.LineMove2.X;
		}
		if( TestLine.LineEnd.Y < 0 )
		{
			TestLine.LineMove2.Y = -TestLine.LineMove2.Y;
		}
		if( TestLine.LineStart.X > Viewport->GetSizeXY().X )
		{
			TestLine.LineMove.X = -TestLine.LineMove.X;
		}
		if( TestLine.LineStart.Y > Viewport->GetSizeXY().Y )
		{
			TestLine.LineMove.Y = -TestLine.LineMove.Y;
		}
		if( TestLine.LineEnd.X > Viewport->GetSizeXY().X )
		{
			TestLine.LineMove2.X = -TestLine.LineMove2.X;
		}
		if( TestLine.LineEnd.Y > Viewport->GetSizeXY().Y )
		{
			TestLine.LineMove2.Y = -TestLine.LineMove2.Y;
		}
	}
	
	// Text
	float CenterX = 0.0f;
	float YTest = 16.0f;
	FCanvasTextItem TextItem( FVector2D( CenterX, YTest ), LOCTEXT( "stringhere", "String Here" ), GEngine->GetSmallFont(), FLinearColor::Red );	
	TextItem.Draw( Canvas );
	
	// Shadowed text
	TextItem.Position.Y += TextItem.DrawnSize.Y;
	TextItem.Scale.X = 2.0f;
	TextItem.EnableShadow( FLinearColor::Green, FVector2D( 2.0f, 2.0f ) );
	TextItem.Text = LOCTEXT( "Scaled String here", "Scaled String here" );
	TextItem.Draw( Canvas );
	TextItem.DisableShadow();

	TextItem.Position.Y += TextItem.DrawnSize.Y;;
	TextItem.Text = LOCTEXT( "CenterdStringhere", "CenterdStringhere" );
	TextItem.Scale.X = 1.0f;
	TextItem.bCentreX = true;
	TextItem.Draw( Canvas );

	// Outlined text
	TextItem.Position.Y += TextItem.DrawnSize.Y;
	TextItem.Text = LOCTEXT( "ScaledCentredStringhere", "Scaled Centred String here" );	
	TextItem.OutlineColor = FLinearColor::Black;
	TextItem.bOutlined = true;
	TextItem.Scale = FVector2D( 2.0f, 2.0f );
	TextItem.SetColor( FLinearColor::Green );
	TextItem.Text = LOCTEXT( "ScaledCentredOutlinedStringhere", "Scaled Centred Outlined String here" );	
	TextItem.Draw( Canvas );
	
	// a line
	FCanvasLineItem LineItem( TestLine.LineStart, TestLine.LineEnd );
	LineItem.Draw( Canvas );

	// some boxes
	FCanvasBoxItem BoxItem( FVector2D( 88.0f, 88.0f ), FVector2D( 188.0f, 188.0f ) );
	BoxItem.SetColor( FLinearColor::Yellow );
	BoxItem.Draw( Canvas );

	BoxItem.SetColor( FLinearColor::Red );
	BoxItem.Position = FVector2D( 256.0f, 256.0f );
	BoxItem.Draw( Canvas );

	BoxItem.SetColor( FLinearColor::Blue );
	BoxItem.Position = FVector2D( 6.0f, 6.0f );
	BoxItem.Size = FVector2D( 48.0f, 96.0f );
	BoxItem.Draw( Canvas );
	
	// Triangle
	FCanvasTriangleItem TriItem(  FVector2D( 48.0f, 48.0f ), FVector2D( 148.0f, 48.0f ), FVector2D( 48.0f, 148.0f ), GWhiteTexture );
	TriItem.Draw( Canvas );

	// Triangle list
	TArray< FCanvasUVTri >	TriangleList;
	FCanvasUVTri SingleTri;
	SingleTri.V0_Pos = FVector2D( 128.0f, 128.0f );
	SingleTri.V1_Pos = FVector2D( 248.0f, 108.0f );
	SingleTri.V2_Pos = FVector2D( 100.0f, 348.0f );
	SingleTri.V0_UV = FVector2D::ZeroVector;
	SingleTri.V1_UV = FVector2D::ZeroVector;
	SingleTri.V2_UV = FVector2D::ZeroVector;
	TriangleList.Add( SingleTri );
	SingleTri.V0_Pos = FVector2D( 348.0f, 128.0f );
	SingleTri.V1_Pos = FVector2D( 448.0f, 148.0f );
	SingleTri.V2_Pos = FVector2D( 438.0f, 308.0f );
	TriangleList.Add( SingleTri );

	FCanvasTriangleItem TriItemList( TriangleList, GWhiteTexture );
	TriItemList.SetColor( FLinearColor::Red );
	TriItemList.Draw( Canvas );
	
// 	FCanvasNGonItem NGon( FVector2D( 256.0f, 256.0f ), FVector2D( 256.0f, 256.0f ), 6, GWhiteTexture, FLinearColor::White );
// 	NGon.Draw( Canvas );
// 
// 	FCanvasNGonItem NGon2( FVector2D( 488, 666.0f ), FVector2D( 256.0f, 256.0f ), 16, GWhiteTexture, FLinearColor::Green );
// 	NGon2.Draw( Canvas );

	// Texture
	UTexture* SelectedTexture = GEditor->GetSelectedObjects()->GetTop<UTexture>();	
	if( SelectedTexture )
	{
		// Plain tex
		FCanvasTileItem TileItem( FVector2D( 128.0f,128.0f ), SelectedTexture->Resource, FLinearColor::White );
		TileItem.Draw( Canvas );
		TileItem.Size = FVector2D( 32.0f,32.0f );
		TileItem.Position = FVector2D( 16.0f,16.0f );
		TileItem.Draw( Canvas );

		// UV 
		TileItem.Size = FVector2D( 64.0f,64.0f );
		TileItem.UV0 = FVector2D( 0.0f, 0.0f );
		TileItem.UV1 = FVector2D( 1.0f, 1.0f );
		TileItem.Position = FVector2D( 256.0f,16.0f );
		TileItem.Draw( Canvas );

		// UV 
		TileItem.Size = FVector2D( 64.0f,64.0f );
		TileItem.UV0 = FVector2D( 0.0f, 0.0f );
		TileItem.UV1 = FVector2D( 1.0f, -1.0f );
		TileItem.Position = FVector2D( 356.0f,16.0f );
		TileItem.Draw( Canvas );

		// UV 
		TileItem.Size = FVector2D( 64.0f,64.0f );
		TileItem.UV0 = FVector2D( 0.0f, 0.0f );
		TileItem.UV1 = FVector2D( -1.0f, 1.0f );
		TileItem.Position = FVector2D( 456.0f,16.0f );
		TileItem.Draw( Canvas );

		// UV 
		TileItem.Size = FVector2D( 64.0f,64.0f );
		TileItem.UV0 = FVector2D( 0.0f, 0.0f );
		TileItem.UV1 = FVector2D( -1.0f, -1.0f );
		TileItem.Position = FVector2D( 556.0f,16.0f );
		TileItem.Draw( Canvas );

		// Rotate top/left pivot
		TileItem.Size = FVector2D( 96.0f,96.0f );
		TileItem.UV0 = FVector2D( 0.0f, 0.0f );
		TileItem.UV1 = FVector2D( 1.0f, 1.0f );
		TileItem.Position = FVector2D( 400.0f,264.0f );
		TileItem.Rotation.Yaw = TestLine.Testangle;
		TileItem.Draw( Canvas );

		// Rotate center pivot
		TileItem.Size = FVector2D( 128.0f, 128.0f );
		TileItem.UV0 = FVector2D( 0.0f, 0.0f );
		TileItem.UV1 = FVector2D( 1.0f, 1.0f );
		TileItem.Position = FVector2D( 600.0f,264.0f );
		TileItem.Rotation.Yaw = 360.0f - TestLine.Testangle;
		TileItem.PivotPoint = FVector2D( 0.5f, 0.5f );
		TileItem.Draw( Canvas );

		TestLine.Testangle = FMath::Fmod( TestLine.Testangle + 2.0f, 360.0f );

		// textured tri
		FCanvasTriangleItem TriItemTex(  FVector2D( 48.0f, 48.0f ), FVector2D( 148.0f, 48.0f ), FVector2D( 48.0f, 148.0f ), FVector2D( 0.0f, 0.0f ), FVector2D( 1.0f, 0.0f ), FVector2D( 0.0f, 1.0f ), SelectedTexture->Resource  );
		TriItem.Texture = GWhiteTexture;
		TriItemTex.Draw( Canvas );

		// moving tri (only 1 point moves !)
		TriItemTex.Position = TestLine.LineStart;
		TriItemTex.Draw( Canvas );
	}
}
#undef LOC_NAMESPACE

#endif // WITH_EDITOR


FCanvasTileItem::FCanvasTileItem( const FVector2D& InPosition, const FVector2D& InSize, const FLinearColor& InColor )
	: FCanvasItem( InPosition )
	, Size( InSize )
	, Z( 1.0f )
	, UV0( 0.0f, 0.0f )
	, UV1( 1.0f, 1.0f )
	, Texture( GWhiteTexture )
	, MaterialRenderProxy( NULL )
	, Rotation( ForceInitToZero )
	, PivotPoint( FVector2D::ZeroVector )
{		
	SetColor( InColor );
}

void FCanvasTileItem::Draw( class FCanvas* InCanvas )
{		
	// Rotate the canvas if the item has rotation
	if( Rotation.IsZero() == false )
	{
		FVector AnchorPos( Size.X * PivotPoint.X, Size.Y * PivotPoint.Y, 0.0f );
		FRotationMatrix RotMatrix( Rotation );
		FMatrix TransformMatrix;
		TransformMatrix = FTranslationMatrix(-AnchorPos) * RotMatrix * FTranslationMatrix(AnchorPos);

		FVector TestPos( Position.X, Position.Y, 0.0f );
		// translate the matrix back to origin, apply the rotation matrix, then transform back to the current position
		FMatrix FinalTransform = FTranslationMatrix(-TestPos) * TransformMatrix * FTranslationMatrix( TestPos );

		InCanvas->PushRelativeTransform(FinalTransform);
	}

	// Draw the item
	if( Texture )
	{
		SCOPE_CYCLE_COUNTER(STAT_Canvas_TileTextureItemTime);

		FLinearColor ActualColor = Color;
		ActualColor.A *= InCanvas->AlphaModulate;
		const FTexture* FinalTexture = Texture ? Texture : GWhiteTexture;	
		FBatchedElements* BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Triangle, BatchedElementParameters, FinalTexture, BlendMode);
		FHitProxyId HitProxyId = InCanvas->GetHitProxyId();

		// Correct for Depth. This only works because we won't be applying a transform later--otherwise we'd have to adjust the transform instead.
		float Left, Top, Right, Bottom;
		Left =		Position.X * Z;
		Top =		Position.Y * Z;
		Right =		( Position.X + Size.X ) * Z;
		Bottom =	( Position.Y + Size.Y ) * Z;		

		int32 V00 = BatchedElements->AddVertex(
			FVector4( Left, Top, 0.0f, Z ),
			FVector2D( UV0.X, UV0.Y ),
			ActualColor,
			HitProxyId );
		int32 V10 = BatchedElements->AddVertex(
			FVector4( Right, Top, 0.0f, Z ),
			FVector2D( UV1.X, UV0.Y ),
			ActualColor,
			HitProxyId );
		int32 V01 = BatchedElements->AddVertex(
			FVector4( Left, Bottom, 0.0f, Z ),
			FVector2D( UV0.X, UV1.Y ),		
			ActualColor,
			HitProxyId );
		int32 V11 = BatchedElements->AddVertex(
			FVector4(Right,	Bottom,	0.0f, Z ),
			FVector2D( UV1.X, UV1.Y ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, FinalTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, FinalTexture, BlendMode );
	}
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_Canvas_TileMaterialItemTime);
		RenderMaterialTile( InCanvas, Position );
	}

	// Restore the canvas transform if we rotated it.
	if( Rotation.IsZero() == false )
	{
		InCanvas->PopTransform();
	}
	

}


void FCanvasTileItem::RenderMaterialTile( class FCanvas* InCanvas, const FVector2D& InPosition )
{
	// get sort element based on the current sort key from top of sort key stack
	FCanvas::FCanvasSortElement& SortElement = InCanvas->GetSortElement(InCanvas->TopDepthSortKey());
	// find a batch to use 
	FCanvasTileRendererItem* RenderBatch = NULL;
	// get the current transform entry from top of transform stack
	const FCanvas::FTransformEntry& TopTransformEntry = InCanvas->GetTransformStack().Top();	

	// try to use the current top entry in the render batch array
	if( SortElement.RenderBatchArray.Num() > 0 )
	{
		checkSlow( SortElement.RenderBatchArray.Last() );
		RenderBatch = SortElement.RenderBatchArray.Last()->GetCanvasTileRendererItem();
	}	
	// if a matching entry for this batch doesn't exist then allocate a new entry
	if( RenderBatch == NULL ||		
		!RenderBatch->IsMatch(MaterialRenderProxy,TopTransformEntry) )
	{
		INC_DWORD_STAT(STAT_Canvas_NumBatchesCreated);

		RenderBatch = new FCanvasTileRendererItem( MaterialRenderProxy,TopTransformEntry,bFreezeTime );
		SortElement.RenderBatchArray.Add(RenderBatch);
	}
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();
	// add the quad to the tile render batch
	RenderBatch->AddTile( InPosition.X, InPosition.Y ,Size.X, Size.Y, UV0.X, UV0.Y, UV1.X-UV0.X, UV1.Y-UV0.Y, HitProxyId );
}


void FCanvasBorderItem::Draw( class FCanvas* InCanvas )
{		
	// Rotate the canvas if the item has rotation
	if( Rotation.IsZero() == false )
	{
		FVector AnchorPos( Size.X * PivotPoint.X, Size.Y * PivotPoint.Y, 0.0f );
		FRotationMatrix RotMatrix( Rotation );
		FMatrix TransformMatrix;
		TransformMatrix = FTranslationMatrix(-AnchorPos) * RotMatrix * FTranslationMatrix(AnchorPos);

		FVector TestPos( Position.X, Position.Y, 0.0f );
		// translate the matrix back to origin, apply the rotation matrix, then transform back to the current position
		FMatrix FinalTransform = FTranslationMatrix(-TestPos) * TransformMatrix * FTranslationMatrix( TestPos );

		InCanvas->PushRelativeTransform(FinalTransform);
	}

	// Draw the item
	if( BorderTexture )
	{
		SCOPE_CYCLE_COUNTER(STAT_Canvas_BorderItemTime);

		FLinearColor ActualColor = Color;
		ActualColor.A *= InCanvas->AlphaModulate;
		const FTexture* CornersTexture = BorderTexture ? BorderTexture : GWhiteTexture;	
		const FTexture* BackTexture = BackgroundTexture ? BackgroundTexture : GWhiteTexture;	
		const FTexture* LeftTexture = BorderLeftTexture ? BorderLeftTexture : GWhiteTexture;	
		const FTexture* RightTexture = BorderRightTexture ? BorderRightTexture : GWhiteTexture;	
		const FTexture* TopTexture = BorderTopTexture ? BorderTopTexture : GWhiteTexture;	
		const FTexture* BottomTexture = BorderBottomTexture ? BorderBottomTexture : GWhiteTexture;	
		FBatchedElements* BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Triangle, BatchedElementParameters, CornersTexture, BlendMode);
		FHitProxyId HitProxyId = InCanvas->GetHitProxyId();

		// Correct for Depth. This only works because we won't be applying a transform later--otherwise we'd have to adjust the transform instead.
		float Left, Top, Right, Bottom;
		Left =		Position.X * Z;
		Top =		Position.Y * Z;
		Right =		( Position.X + Size.X ) * Z;
		Bottom =	( Position.Y + Size.Y ) * Z;		

		const float BorderLeftDrawSizeX = BorderLeftTexture->GetSizeX()*BorderScale.X;
		const float BorderLeftDrawSizeY = BorderLeftTexture->GetSizeY()*BorderScale.Y;
		const float BorderTopDrawSizeX = BorderTopTexture->GetSizeX()*BorderScale.X;
		const float BorderTopDrawSizeY = BorderTopTexture->GetSizeY()*BorderScale.Y;
		const float BorderRightDrawSizeX = BorderRightTexture->GetSizeX()*BorderScale.X;
		const float BorderRightDrawSizeY = BorderRightTexture->GetSizeY()*BorderScale.Y;
		const float BorderBottomDrawSizeX = BorderBottomTexture->GetSizeX()*BorderScale.X;
		const float BorderBottomDrawSizeY = BorderBottomTexture->GetSizeY()*BorderScale.Y;

		const float BackgroundTilingX = (Right-Left)/(BackTexture->GetSizeX()*BackgroundScale.X);
		const float BackgroundTilingY = (Bottom-Top)/(BackTexture->GetSizeY()*BackgroundScale.Y);

		//Draw background
		int32 V00 = BatchedElements->AddVertex(
			FVector4( Left + BorderLeftDrawSizeX, Top + BorderTopDrawSizeY, 0.0f, Z ),
			FVector2D( 0, 0 ),
			ActualColor,
			HitProxyId );
		int32 V10 = BatchedElements->AddVertex(
			FVector4( Right - BorderRightDrawSizeX, Top + BorderTopDrawSizeY, 0.0f, Z ),
			FVector2D( BackgroundTilingX, 0 ),
			ActualColor,
			HitProxyId );
		int32 V01 = BatchedElements->AddVertex(
			FVector4( Left + BorderLeftDrawSizeX, Bottom - BorderBottomDrawSizeY, 0.0f, Z ),
			FVector2D( 0, BackgroundTilingY ),		
			ActualColor,
			HitProxyId );
		int32 V11 = BatchedElements->AddVertex(
			FVector4(Right - BorderRightDrawSizeX, Bottom - BorderBottomDrawSizeY, 0.0f, Z ),
			FVector2D( BackgroundTilingX, BackgroundTilingY ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, BackTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, BackTexture, BlendMode );


		const float BorderTextureWidth = BorderTexture->GetSizeX() * (BorderUV1.X - BorderUV0.X);
		const float BorderTextureHeight = BorderTexture->GetSizeY() * (BorderUV1.Y - BorderUV0.Y);
		const float CornerDrawWidth = BorderTextureWidth * CornerSize.X * BorderScale.X;
		const float CornerDrawHeight = BorderTextureHeight * CornerSize.Y * BorderScale.Y;

		//Top Left Corner
		V00 = BatchedElements->AddVertex(
			FVector4( Left, Top, 0.0f, Z ),
			FVector2D( BorderUV0.X, BorderUV0.Y ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( Left + CornerDrawWidth, Top, 0.0f, Z ),
			FVector2D( BorderUV1.X*CornerSize.X, BorderUV0.Y ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( Left, Top + CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV0.X, BorderUV1.Y*CornerSize.Y ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( Left + CornerDrawWidth, Top + CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV1.X*CornerSize.X, BorderUV1.Y*CornerSize.Y ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, CornersTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, CornersTexture, BlendMode );

		// Top Right Corner
		V00 = BatchedElements->AddVertex(
			FVector4( Right - CornerDrawWidth, Top, 0.0f, Z ),
			FVector2D( BorderUV1.X - (BorderUV1.X - BorderUV0.X)*CornerSize.X, BorderUV0.Y ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( Right, Top, 0.0f, Z ),
			FVector2D( BorderUV1.X, BorderUV0.Y ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( Right - CornerDrawWidth, Top + CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV1.X - (BorderUV1.X - BorderUV0.X)*CornerSize.X, BorderUV1.Y*CornerSize.Y ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( Right, Top + CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV1.X, BorderUV1.Y*CornerSize.Y ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, CornersTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, CornersTexture, BlendMode );

		//Left Bottom Corner
		V00 = BatchedElements->AddVertex(
			FVector4( Left, Bottom - CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV0.X, BorderUV1.Y - (BorderUV1.Y - BorderUV0.Y)*CornerSize.Y),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( Left + CornerDrawWidth, Bottom - CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV1.X*CornerSize.X, BorderUV1.Y - (BorderUV1.Y - BorderUV0.Y)*CornerSize.Y ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( Left, Bottom, 0.0f, Z ),
			FVector2D( BorderUV0.X, BorderUV1.Y ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( Left + CornerDrawWidth, Bottom, 0.0f, Z ),
			FVector2D( BorderUV1.X*CornerSize.X, BorderUV1.Y),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, CornersTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, CornersTexture, BlendMode );

		// Right Bottom Corner
		V00 = BatchedElements->AddVertex(
			FVector4( Right - CornerDrawWidth, Bottom - CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV1.X - (BorderUV1.X - BorderUV0.X)*CornerSize.X, BorderUV1.Y - (BorderUV1.Y - BorderUV0.Y)*CornerSize.Y ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( Right, Bottom - CornerDrawHeight, 0.0f, Z ),
			FVector2D( BorderUV1.X, BorderUV1.Y - (BorderUV1.Y - BorderUV0.Y)*CornerSize.Y ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( Right - CornerDrawWidth, Bottom, 0.0f, Z ),
			FVector2D( BorderUV1.X - (BorderUV1.X - BorderUV0.X)*CornerSize.X, BorderUV1.Y ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( Right, Bottom, 0.0f, Z ),
			FVector2D( BorderUV1.X, BorderUV1.Y ),
			ActualColor,
			HitProxyId);
			
		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, CornersTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, CornersTexture, BlendMode );

		const float BorderLeft = Left + CornerDrawWidth;
		const float BorderRight = Right - CornerDrawWidth;
		const float BorderTop = Top + CornerDrawHeight;
		const float BorderBottom = Bottom - CornerDrawHeight;

		//Top Frame Border
		const float TopFrameTilingX = (BorderRight-BorderLeft)/BorderTopDrawSizeX;

		V00 = BatchedElements->AddVertex(
			FVector4( BorderLeft, Top, 0.0f, Z ),
			FVector2D( 0, 0 ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( BorderRight, Top, 0.0f, Z ),
			FVector2D( TopFrameTilingX , 0 ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( BorderLeft, Top + BorderTopDrawSizeY, 0.0f, Z ),
			FVector2D( 0, 1.0f ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( BorderRight, Top + BorderTopDrawSizeY, 0.0f, Z ),
			FVector2D( TopFrameTilingX, 1.0f ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, BorderTopTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, BorderTopTexture, BlendMode );

		//Bottom Frame Border
		const float BottomFrameTilingX = (BorderRight-BorderLeft)/BorderBottomDrawSizeX;

		V00 = BatchedElements->AddVertex(
			FVector4( BorderLeft, Bottom - BorderBottomDrawSizeY, 0.0f, Z ),
			FVector2D( 0, 0 ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( BorderRight, Bottom - BorderBottomDrawSizeY, 0.0f, Z ),
			FVector2D( BottomFrameTilingX, 0 ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( BorderLeft, Bottom, 0.0f, Z ),
			FVector2D( 0, 1.0f ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( BorderRight, Bottom, 0.0f, Z ),
			FVector2D( BottomFrameTilingX, 1.0f ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, BorderBottomTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, BorderBottomTexture, BlendMode );


		//Left Frame Border
		const float LeftFrameTilingY = (BorderBottom-BorderTop) / BorderLeftDrawSizeY;

		V00 = BatchedElements->AddVertex(
			FVector4( Left, BorderTop, 0.0f, Z ),
			FVector2D( 0, 0 ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( Left +  BorderLeftDrawSizeX , BorderTop, 0.0f, Z ),
			FVector2D( 1.0f, 0 ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( Left, BorderBottom, 0.0f, Z ),
			FVector2D( 0, LeftFrameTilingY ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( Left + BorderLeftDrawSizeX, BorderBottom, 0.0f, Z ),
			FVector2D( 1.0f, LeftFrameTilingY ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, BorderLeftTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, BorderLeftTexture, BlendMode );


		//Right Frame Border
		const float RightFrameTilingY = (BorderBottom-BorderTop)/BorderRightDrawSizeY;

		V00 = BatchedElements->AddVertex(
			FVector4( Right - BorderRightDrawSizeX, BorderTop, 0.0f, Z ),
			FVector2D( 0, 0 ),
			ActualColor,
			HitProxyId );
		V10 = BatchedElements->AddVertex(
			FVector4( Right, BorderTop, 0.0f, Z ),
			FVector2D( 1.0f , 0 ),
			ActualColor,
			HitProxyId );
		V01 = BatchedElements->AddVertex(
			FVector4( Right - BorderRightDrawSizeX, BorderBottom, 0.0f, Z ),
			FVector2D( 0, RightFrameTilingY ),		
			ActualColor,
			HitProxyId );
		V11 = BatchedElements->AddVertex(
			FVector4( Right, BorderBottom, 0.0f, Z ),
			FVector2D( 1.0f, RightFrameTilingY ),
			ActualColor,
			HitProxyId);

		BatchedElements->AddTriangleExtensive( V00, V10, V11, BatchedElementParameters, BorderRightTexture, BlendMode );
		BatchedElements->AddTriangleExtensive( V00, V11, V01, BatchedElementParameters, BorderRightTexture, BlendMode );

	}

	// Restore the canvas transform if we rotated it.
	if( Rotation.IsZero() == false )
	{
		InCanvas->PopTransform();
	}

}


void FCanvasTextItem::Draw( class FCanvas* InCanvas )
{	
	SCOPE_CYCLE_COUNTER(STAT_Canvas_TextItemTime);

	if(Font == NULL || Text.IsEmpty() )
	{
		return;
	}

	XScale = Scale.X;
	YScale = Scale.Y;

	bool bHasShadow = ShadowOffset.Size() != 0.0f;
	if( ( FontRenderInfo.bEnableShadow == true ) && ( bHasShadow == false ) )
	{
		EnableShadow( FLinearColor::Black );
		bHasShadow = true;
	}
	if (Font->ImportOptions.bUseDistanceFieldAlpha)
	{
		// convert blend mode to distance field type
		switch(BlendMode)
		{
		case SE_BLEND_Translucent:
			BlendMode = (FontRenderInfo.bEnableShadow) ? SE_BLEND_TranslucentDistanceFieldShadowed : SE_BLEND_TranslucentDistanceField;
			break;
		case SE_BLEND_Masked:
			BlendMode = (FontRenderInfo.bEnableShadow) ? SE_BLEND_MaskedDistanceFieldShadowed : SE_BLEND_MaskedDistanceField;
			break;
		}
		// Disable the show if the blend mode is not suitable
		if (BlendMode != SE_BLEND_TranslucentDistanceFieldShadowed && BlendMode != SE_BLEND_MaskedDistanceFieldShadowed)
		{
			bHasShadow = false;
		}
	}	

	CharIncrement = ( (float)Font->Kerning + HorizSpacingAdjust ) * Scale.X;
	DrawnSize.Y = Font->GetMaxCharHeight() * Scale.Y;

	FVector2D DrawPos( Position.X , Position.Y );

	// If we are centering the string or we want to fix stereoscopic rending issues we need to measure the string
	if( ( bCentreX || bCentreY ) || ( !bDontCorrectStereoscopic ) )
	{
		FTextSizingParameters Parameters( Font, Scale.X ,Scale.Y );
		UCanvas::CanvasStringSize( Parameters, *Text.ToString() );
				
		// Calculate the offset if we are centering
		if( bCentreX || bCentreY )
		{		
			// Note we drop the fraction after the length divide or we can end up with coords on 1/2 pixel boundaries
			if( bCentreX )
			{
				DrawPos.X = DrawPos.X - (int)( Parameters.DrawXL / 2 );
			}
			if( bCentreY )
			{
				DrawPos.Y = DrawPos.Y - (int)( Parameters.DrawYL / 2 );
			}
		}

		// Check if we want to correct the stereo3d issues - if we do, render the correction now
		bool CorrectStereo = !bDontCorrectStereoscopic  && GEngine->IsStereoscopic3D();
		if( CorrectStereo )
		{
			FVector2D StereoOutlineBoxSize( 2.0f, 2.0f );
			TileItem.MaterialRenderProxy = GEngine->RemoveSurfaceMaterial->GetRenderProxy( false );
			TileItem.Position = DrawPos - StereoOutlineBoxSize;
			FVector2D CorrectionSize = FVector2D( Parameters.DrawXL, Parameters.DrawYL ) + StereoOutlineBoxSize + StereoOutlineBoxSize;
			TileItem.Size=  CorrectionSize;
			TileItem.bFreezeTime = true;
			TileItem.Draw( InCanvas );
		}		
	}
	
	FLinearColor DrawColor;
	BatchedElements = NULL;
	TextLen = Text.ToString().Len();
	if( bOutlined )
	{
		DrawColor = OutlineColor;
		DrawColor.A *= InCanvas->AlphaModulate;
		DrawStringInternal( InCanvas, DrawPos + FVector2D( -1.0f, -1.0f ), DrawColor );
		DrawStringInternal( InCanvas, DrawPos + FVector2D( -1.0f, 1.0f ), DrawColor );
		DrawStringInternal( InCanvas, DrawPos + FVector2D( 1.0f, 1.0f ), DrawColor );
		DrawStringInternal( InCanvas, DrawPos + FVector2D( 1.0f, -1.0f ), DrawColor );
	}
	// If we have a shadow - draw it now
	if( bHasShadow )
	{
		DrawColor = ShadowColor;
		// Copy the Alpha from the shadow otherwise if we fade the text the shadow wont fade - which is almost certainly not what we will want.
		DrawColor.A = Color.A;
		DrawColor.A *= InCanvas->AlphaModulate;
		DrawStringInternal( InCanvas, DrawPos + ShadowOffset, DrawColor );
	}
	DrawColor = Color;
	DrawColor.A *= InCanvas->AlphaModulate;	
	DrawStringInternal( InCanvas, DrawPos, DrawColor );
	
	return;
}

void FCanvasTextItem::DrawStringInternal( class FCanvas* InCanvas, const FVector2D& DrawPos, const FLinearColor& InColour )
{		
	DrawnSize.X = 0.0f;
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();
	FTexture* LastTexture = NULL;
	UTexture2D* Tex = NULL;
	FVector2D InvTextureSize(1.0f,1.0f);
	const TArray< TCHAR >& Chars = Text.ToString().GetCharArray();
	// Draw all characters in string.
	for( int32 i=0; i < TextLen; i++ )
	{
		int32 Ch = (int32)Font->RemapChar(Chars[i]);

		// Process character if it's valid.
		if( Font->Characters.IsValidIndex(Ch) )
		{
			FFontCharacter& Char = Font->Characters[Ch];
			if( Font->Textures.IsValidIndex(Char.TextureIndex) && 
				(Tex=Font->Textures[Char.TextureIndex])!=NULL && 
				Tex->Resource != NULL )
			{
				if( LastTexture != Tex->Resource || BatchedElements == NULL )
				{
					FBatchedElementParameters* BatchedElementParameters = NULL;
					BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Triangle, BatchedElementParameters, Tex->Resource, BlendMode, FontRenderInfo.GlowInfo);
					check(BatchedElements != NULL);
					// trade-off between memory and performance by pre-allocating more reserved space 
					// for the triangles/vertices of the batched elements used to render the text tiles
					//BatchedElements->AddReserveTriangles(TextLen*2,Tex->Resource,BlendMode);
					//BatchedElements->AddReserveVertices(TextLen*4);

					FIntPoint ImportedTextureSize = Tex->GetImportedSize();
					InvTextureSize.X = 1.0f / (float)ImportedTextureSize.X;
					InvTextureSize.Y = 1.0f / (float)ImportedTextureSize.Y;
				}
				LastTexture = Tex->Resource;

				const float X		= DrawnSize.X + DrawPos.X;
				const float Y		= DrawPos.Y + + Char.VerticalOffset * YScale;
				float SizeX			= Char.USize * XScale;
				const float SizeY	= Char.VSize * YScale;
				const float U		= Char.StartU * InvTextureSize.X;
				const float V		= Char.StartV * InvTextureSize.Y;
				const float SizeU	= Char.USize * InvTextureSize.X;
				const float SizeV	= Char.VSize * InvTextureSize.Y;				

				float Left, Top, Right, Bottom;
				Left = X * Depth;
				Top = Y * Depth;
				Right = (X + SizeX) * Depth;
				Bottom = (Y + SizeY) * Depth;

				int32 V00 = BatchedElements->AddVertex(
					FVector4( Left, Top, 0.f, Depth ),
					FVector2D( U, V ),
					InColour,
					HitProxyId );
				int32 V10 = BatchedElements->AddVertex(
					FVector4( Right, Top, 0.0f, Depth ),
					FVector2D( U + SizeU, V ),			
					InColour,
					HitProxyId );
				int32 V01 = BatchedElements->AddVertex(
					FVector4( Left, Bottom, 0.0f, Depth ),
					FVector2D( U, V + SizeV ),	
					InColour,
					HitProxyId);
				int32 V11 = BatchedElements->AddVertex(
					FVector4( Right, Bottom, 0.0f, Depth ),
					FVector2D( U + SizeU, V + SizeV ),
					InColour,
					HitProxyId);

				BatchedElements->AddTriangle(V00, V10, V11, Tex->Resource, BlendMode, FontRenderInfo.GlowInfo);
				BatchedElements->AddTriangle(V00, V11, V01, Tex->Resource, BlendMode, FontRenderInfo.GlowInfo);

				// if we have another non-whitespace character to render, add the font's kerning.
				if ( Chars[i+1] && !FChar::IsWhitespace(Chars[i+1]) )
				{
					SizeX += CharIncrement;
				}

				// Update the current rendering position
				DrawnSize.X += SizeX;
			}
		}
	}
}
/* Draw the item. */
void FCanvasLineItem::Draw( class FCanvas* InCanvas )
{
	SCOPE_CYCLE_COUNTER(STAT_Canvas_LineItemTime);

	FBatchedElements* BatchedElements = InCanvas->GetBatchedElements( FCanvas::ET_Line );
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();
	BatchedElements->AddLine( Origin, EndPos, Color, HitProxyId, LineThickness );
}


void FCanvasBoxItem::Draw( class FCanvas* InCanvas )
{
	SCOPE_CYCLE_COUNTER(STAT_Canvas_BoxItemTime);

	SetupBox();

	FBatchedElements* BatchedElements = InCanvas->GetBatchedElements( FCanvas::ET_Line );
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();
	
	// Draw the 4 edges
	for (int32 iEdge = 0; iEdge < Corners.Num() ; iEdge++)
	{
		int32 NextCorner = ( ( iEdge + 1 ) % Corners.Num() );
		BatchedElements->AddLine( Corners[ iEdge ], Corners[ NextCorner ] , Color, HitProxyId, LineThickness );	
	}
}

void FCanvasBoxItem::SetupBox()
{
	Corners.Empty();
	// Top
	Corners.AddUnique( FVector( Position.X , Position.Y, 0.0f ) );
	// Right
	Corners.AddUnique( FVector( Position.X + Size.X , Position.Y, 0.0f ) );
	// Bottom
	Corners.AddUnique( FVector( Position.X + Size.X , Position.Y + Size.Y, 0.0f ) );
	// Left
	Corners.AddUnique( FVector( Position.X, Position.Y + Size.Y, 0.0f ) );
}


void FCanvasTriangleItem::Draw( class FCanvas* InCanvas )
{
	SCOPE_CYCLE_COUNTER(STAT_Canvas_TriItemTime);

	FBatchedElements* BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Triangle, BatchedElementParameters, Texture, BlendMode);
	
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();

	for(int32 i=0; i<TriangleList.Num(); i++)
	{
		const FCanvasUVTri& Tri = TriangleList[i];
		int32 V0 = BatchedElements->AddVertex(FVector4(Tri.V0_Pos.X,Tri.V0_Pos.Y,0,1), Tri.V0_UV, Tri.V0_Color, HitProxyId);
		int32 V1 = BatchedElements->AddVertex(FVector4(Tri.V1_Pos.X,Tri.V1_Pos.Y,0,1), Tri.V1_UV, Tri.V1_Color, HitProxyId);
		int32 V2 = BatchedElements->AddVertex(FVector4(Tri.V2_Pos.X,Tri.V2_Pos.Y,0,1), Tri.V2_UV, Tri.V2_Color, HitProxyId);

		if( BatchedElementParameters )
		{
			BatchedElements->AddTriangle(V0,V1,V2, BatchedElementParameters, BlendMode);
		}
		else
		{
			check( Texture );
			BatchedElements->AddTriangle(V0,V1,V2,Texture, BlendMode);
		}
	}	
}

void FCanvasTriangleItem::SetColor( const FLinearColor& InColor )
{
	for(int32 i=0; i<TriangleList.Num(); i++)
	{
		TriangleList[i].V0_Color = InColor;
		TriangleList[i].V1_Color = InColor;
		TriangleList[i].V2_Color = InColor;
	}	
}


void FCanvasNGonItem::Draw( class FCanvas* InCanvas )
{
	TriListItem->Draw( InCanvas );
}

void FCanvasNGonItem::SetColor( const FLinearColor& InColor )
{
	TriListItem->SetColor( InColor );
}



FCanvasNGonItem::FCanvasNGonItem( const FVector2D& InPosition, const FVector2D& InRadius, int32 InNumSides, const FLinearColor& InColor )
	: FCanvasItem( InPosition )
	, TriListItem( NULL )
	, Texture( GWhiteTexture )
{
	Color = InColor;
	check( InNumSides >= 3 );
	TriangleList.SetNum( InNumSides );
	SetupPosition( InPosition, InRadius );
}

#undef LOCTEXT_NAMESPACE 

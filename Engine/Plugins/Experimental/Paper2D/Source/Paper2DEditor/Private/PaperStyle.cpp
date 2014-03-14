// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperStyle.h"
#include "SlateStyle.h"
#include "EditorStyle.h"

//////////////////////////////////////////////////////////////////////////
// FPaperStyle

TSharedPtr<FSlateStyleSet> FPaperStyle::PaperStyleInstance = NULL;

void FPaperStyle::Initialize()
{
	if (!PaperStyleInstance.IsValid())
	{
		PaperStyleInstance = Create();
	}

	SetStyle(PaperStyleInstance.ToSharedRef());
}

void FPaperStyle::Shutdown()
{
	ResetToDefault();
	ensure(PaperStyleInstance.IsUnique());
	PaperStyleInstance.Reset();
}

TSharedRef<FSlateStyleSet> FPaperStyle::Create()
{
	IEditorStyleModule& EditorStyle = FModuleManager::LoadModuleChecked<IEditorStyleModule>(TEXT("EditorStyle"));
	TSharedRef< FSlateStyleSet > StyleRef = EditorStyle.CreateEditorStyleInstance();
	FSlateStyleSet& Style = StyleRef.Get();

	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	// Tile map editor
	{
		Style.Set("PaperEditor.EnterTileMapEditMode", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/TileMapEdModeIcon.TileMapEdModeIcon"), Icon40x40));

		Style.Set("PaperEditor.SelectPaintTool", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ToolIcon_PaintBrush.ToolIcon_PaintBrush"), Icon40x40));
		Style.Set("PaperEditor.SelectEraserTool", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ToolIcon_Eraser.ToolIcon_Eraser"), Icon40x40));
		Style.Set("PaperEditor.SelectFillTool", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ToolIcon_PaintBucket.ToolIcon_PaintBucket"), Icon40x40));
	}

	// Sprite editor
	{
		Style.Set( "SpriteEditor.SetShowGrid", new FSlateImageBrush( TEXT("Icons/icon_MatEd_Grid_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowGrid.Small", new FSlateImageBrush( TEXT("Icons/icon_MatEd_Grid_40x"), Icon20x20 ) );
		Style.Set( "SpriteEditor.SetShowSourceTexture", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ShowSpriteSheetButton.ShowSpriteSheetButton"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowSourceTexture.Small", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ShowSpriteSheetButton.ShowSpriteSheetButton"), Icon20x20 ) );
		Style.Set( "SpriteEditor.SetShowBounds", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_Bounds_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowBounds.Small", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_Bounds_40x"), Icon20x20 ) );
		Style.Set( "SpriteEditor.SetShowCollision", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_Collision_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowCollision.Small", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_Collision_40x"), Icon20x20 ) );

		Style.Set( "SpriteEditor.SetShowSockets", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_ShowSockets_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowSockets.Small", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_ShowSockets_40x"), Icon20x20 ) );
		Style.Set( "SpriteEditor.SetShowNormals", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_Normals_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowNormals.Small", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_Normals_40x"), Icon20x20 ) );
		Style.Set( "SpriteEditor.SetShowPivot", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_ShowPivot_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.SetShowPivot.Small", new FSlateImageBrush( TEXT("Icons/icon_StaticMeshEd_ShowPivot_40x"), Icon20x20 ) );

		Style.Set( "SpriteEditor.EnterViewMode", new FSlateImageBrush( TEXT("Icons/icon_TextureEd_AlphaChannel_40x"), Icon40x40 ) );
		Style.Set( "SpriteEditor.EnterViewMode.Small", new FSlateImageBrush( TEXT("Icons/icon_TextureEd_AlphaChannel_40x"), Icon20x20 ) );

		Style.Set( "SpriteEditor.EnterCollisionEditMode", new FSlateImageBrush( TEXT("texture://Paper2D/SlateBrushes/EditCollisionGeom.EditCollisionGeom"), Icon40x40 ) );
		Style.Set( "SpriteEditor.EnterCollisionEditMode.Small", new FSlateImageBrush( TEXT("texture://Paper2D/SlateBrushes/EditCollisionGeom.EditCollisionGeom"), Icon20x20 ) );

		Style.Set( "SpriteEditor.EnterRenderingEditMode", new FSlateImageBrush( TEXT("texture://Paper2D/SlateBrushes/EditRenderGeomButton.EditRenderGeomButton"), Icon40x40 ) );
		Style.Set( "SpriteEditor.EnterRenderingEditMode.Small", new FSlateImageBrush( TEXT("texture://Paper2D/SlateBrushes/EditRenderGeomButton.EditRenderGeomButton"), Icon20x20 ) );

		Style.Set( "SpriteEditor.EnterAddSpriteMode", new FSlateImageBrush( TEXT("texture://Paper2D/SlateBrushes/AddSpriteButton.AddSpriteButton"), Icon40x40 ) );
		Style.Set( "SpriteEditor.EnterAddSpriteMode.Small", new FSlateImageBrush( TEXT("texture://Paper2D/SlateBrushes/AddSpriteButton.AddSpriteButton"), Icon20x20 ) );
	}

	return StyleRef;
}

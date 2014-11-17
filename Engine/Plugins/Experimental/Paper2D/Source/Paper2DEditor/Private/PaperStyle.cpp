// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperStyle.h"
#include "SlateStyle.h"
#include "EditorStyle.h"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(Style.RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(Style.RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

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
		Style.Set("SpriteEditor.SetShowGrid", new IMAGE_BRUSH(TEXT("Icons/icon_MatEd_Grid_40x"), Icon40x40));
		Style.Set("SpriteEditor.SetShowGrid.Small", new IMAGE_BRUSH(TEXT("Icons/icon_MatEd_Grid_40x"), Icon20x20));
		Style.Set("SpriteEditor.SetShowSourceTexture", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ShowSpriteSheetButton.ShowSpriteSheetButton"), Icon40x40));
		Style.Set("SpriteEditor.SetShowSourceTexture.Small", new FSlateImageBrush(TEXT("texture://Paper2D/SlateBrushes/ShowSpriteSheetButton.ShowSpriteSheetButton"), Icon20x20));
		Style.Set("SpriteEditor.SetShowBounds", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Bounds_40x"), Icon40x40));
		Style.Set("SpriteEditor.SetShowBounds.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Bounds_40x"), Icon20x20));
		Style.Set("SpriteEditor.SetShowCollision", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Collision_40x"), Icon40x40));
		Style.Set("SpriteEditor.SetShowCollision.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Collision_40x"), Icon20x20));

		Style.Set("SpriteEditor.SetShowSockets", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_ShowSockets_40x"), Icon40x40));
		Style.Set("SpriteEditor.SetShowSockets.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_ShowSockets_40x"), Icon20x20));
		Style.Set("SpriteEditor.SetShowNormals", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Normals_40x"), Icon40x40));
		Style.Set("SpriteEditor.SetShowNormals.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Normals_40x"), Icon20x20));
		Style.Set("SpriteEditor.SetShowPivot", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_ShowPivot_40x"), Icon40x40));
		Style.Set("SpriteEditor.SetShowPivot.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_ShowPivot_40x"), Icon20x20));

		Style.Set("SpriteEditor.AddPolygon", new IMAGE_BRUSH(TEXT("Icons/icon_Paper2D_AddPolygon_40x"), Icon40x40));
		Style.Set("SpriteEditor.AddPolygon.Small", new IMAGE_BRUSH(TEXT("Icons/icon_Paper2D_AddPolygon_40x"), Icon20x20));

		Style.Set("SpriteEditor.SnapAllVertices", new IMAGE_BRUSH(TEXT("Icons/icon_Paper2D_SnapToPixelGrid_40x"), Icon40x40));
		Style.Set("SpriteEditor.SnapAllVertices.Small", new IMAGE_BRUSH(TEXT("Icons/icon_Paper2D_SnapToPixelGrid_40x"), Icon20x20));

		Style.Set("SpriteEditor.EnterViewMode", new IMAGE_BRUSH(TEXT("Icons/icon_Paper2D_ViewSprite_40x"), Icon40x40));
		Style.Set("SpriteEditor.EnterViewMode.Small", new IMAGE_BRUSH(TEXT("Icons/icon_Paper2D_ViewSprite_40x"), Icon20x20));

		Style.Set("SpriteEditor.EnterCollisionEditMode", new IMAGE_BRUSH("Icons/icon_Paper2D_EditCollision_40x", Icon40x40));
		Style.Set("SpriteEditor.EnterCollisionEditMode.Small", new IMAGE_BRUSH("Icons/icon_Paper2D_EditCollision_40x", Icon20x20));

		Style.Set("SpriteEditor.EnterSourceRegionEditMode", new IMAGE_BRUSH("Icons/icon_Paper2D_EditSourceRegion_40x", Icon40x40));
		Style.Set("SpriteEditor.EnterSourceRegionEditMode.Small", new IMAGE_BRUSH("Icons/icon_Paper2D_EditSourceRegion_40x", Icon20x20));

		Style.Set("SpriteEditor.EnterRenderingEditMode", new IMAGE_BRUSH("Icons/icon_Paper2D_RenderGeom_40x", Icon40x40));
		Style.Set("SpriteEditor.EnterRenderingEditMode.Small", new IMAGE_BRUSH("Icons/icon_Paper2D_RenderGeom_40x", Icon20x20));

		Style.Set("SpriteEditor.EnterAddSpriteMode", new IMAGE_BRUSH("Icons/icon_Paper2D_AddSprite_40x", Icon40x40));
		Style.Set("SpriteEditor.EnterAddSpriteMode.Small", new IMAGE_BRUSH("Icons/icon_Paper2D_AddSprite_40x", Icon20x20));
	}

	// Flipbook editor
	{
		Style.Set("FlipbookEditor.SetShowGrid", new IMAGE_BRUSH(TEXT("Icons/icon_MatEd_Grid_40x"), Icon40x40));
		Style.Set("FlipbookEditor.SetShowGrid.Small", new IMAGE_BRUSH(TEXT("Icons/icon_MatEd_Grid_40x"), Icon20x20));
		Style.Set("FlipbookEditor.SetShowBounds", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Bounds_40x"), Icon40x40));
		Style.Set("FlipbookEditor.SetShowBounds.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Bounds_40x"), Icon20x20));
		Style.Set("FlipbookEditor.SetShowCollision", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Collision_40x"), Icon40x40));
		Style.Set("FlipbookEditor.SetShowCollision.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_Collision_40x"), Icon20x20));
		Style.Set("FlipbookEditor.SetShowPivot", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_ShowPivot_40x"), Icon40x40));
		Style.Set("FlipbookEditor.SetShowPivot.Small", new IMAGE_BRUSH(TEXT("Icons/icon_StaticMeshEd_ShowPivot_40x"), Icon20x20));

		Style.Set("FlipbookEditor.RegionGrabHandle", new BOX_BRUSH("Sequencer/ScrubHandleWhole", FMargin(6.f / 13.f, 10 / 24.f, 6 / 13.f, 10 / 24.f)));
		Style.Set("FlipbookEditor.RegionBody", new BOX_BRUSH("Common/Scrollbar_Thumb", FMargin(4.f / 16.f)));
		Style.Set("FlipbookEditor.RegionBorder", new BOX_BRUSH("Common/CurrentCellBorder", FMargin(4.f / 16.f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	}

	return StyleRef;
}

//////////////////////////////////////////////////////////////////////////

#undef IMAGE_BRUSH

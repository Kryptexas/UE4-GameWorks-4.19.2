// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SNodePanel.h"
#include "GraphEditor.h"

class FLevelModel;
class FLevelCollectionModel;
class FSlateTextureRenderTarget2DResource;

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
class SWorldLevelsGridView 
	: public SNodePanel
{
public:
	SLATE_BEGIN_ARGS(SWorldLevelsGridView) {}
		SLATE_ARGUMENT(TSharedPtr<FLevelCollectionModel>, InWorldModel)
	SLATE_END_ARGS()

	SWorldLevelsGridView();
	~SWorldLevelsGridView();

	void Construct(const FArguments& InArgs);
	
	/**  Add specified item to the grid view */
	void AddItem(const TSharedPtr<FLevelModel>& InLevelModel);
	
	/**  Remove specified item from the grid view */
	void RemoveItem(const TSharedPtr<FLevelModel>& InLevelModel);
	
	/**  Updates all the items in the grid view */
	void RefreshView();
	
	/**  SWidget interface */
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	
	/**  SWidget interface */
	void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	
	/**  SWidget interface */
	int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, 
					FSlateWindowElementList& OutDrawElements, int32 LayerId, 
					const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	/**  SWidget interface */
	FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	/**  @return Size of a marquee rectangle in world space */
	FVector2D GetMarqueeWorldSize() const;

	/**  @return Mouse cursor position in world space */
	FVector2D GetMouseWorldLocation() const;

protected:
	/**  Draws background for grid view */
	uint32 PaintBackground(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, uint32 LayerId) const;
	
	/**  SWidget interface */
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent);
	
	/**  SWidget interface */
	bool SupportsKeyboardFocus() const OVERRIDE;
	
	/**  SNodePanel interface */
	TSharedPtr<SWidget> OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	
	/**  SNodePanel interface */
	void PopulateVisibleChildren(const FGeometry& AllottedGeometry) OVERRIDE;

	/**  SNodePanel interface */
	void OnBeginNodeInteraction(const TSharedRef<SNode>& InNodeToDrag, const FVector2D& GrabOffset) OVERRIDE;
	
	/**  SNodePanel interface */
	void OnEndNodeInteraction(const TSharedRef<SNode>& InNodeToDrag) OVERRIDE;

	/**  SNodePanel interface */
	bool OnHandleLeftMouseRelease(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Handles selection changes in the grid view */
	void OnSelectionChanged(const FGraphPanelSelectionSet& SelectedNodes);

	/** Handles selection changes in data source */
	void OnUpdateSelection();

	/** Handles new item added to data source */
	void OnNewItemAdded(TSharedPtr<FLevelModel> NewItem);

	/**  FitToSelection command handler */
	void FitToSelection_Executed();
	
	/**  @returns Whether any of the levels are selected */
	bool AreAnyItemsSelected() const;

	/**  Requests view scroll to specified position and fit to specified area 
	 *   @param	InLocation		The location to scroll to
	 *   @param	InArea			The area to fit in view
	 *   @param	vAllowZoomIn	Is zoom in allowed during fit to area calculations
	 */
	void RequestScrollTo(FVector2D InLocation, FVector2D InArea, bool vAllowZoomIn = false);

	/** Handlers for moving items using arrow keys */
	void MoveLevelLeft_Executed();
	void MoveLevelRight_Executed();
	void MoveLevelUp_Executed();
	void MoveLevelDown_Executed();
	
	/** Moves selected nodes by specified offset */
	void MoveSelectedNodes(const TSharedPtr<SNode>& InNodeToDrag, FVector2D InNewPosition);

	/**  Converts cursor absolute position to the world position */
	FVector2D CursorToWorldPosition(const FGeometry& InGeometry, FVector2D AbsoluteCursorPosition);
	
private:
	/** Levels data list to display*/
	TSharedPtr<FLevelCollectionModel>		WorldModel;

	/** Geometry cache */
	mutable FVector2D						CachedAllottedGeometryScaledSize;
	/** Bouncing curve */
	FCurveSequence							BounceCurve;
	bool									bUpdatingSelection;
	TArray<FIntRect>						OccupiedCells;
	const TSharedRef<FUICommandList>		CommandList;

	bool									bHasScrollRequest;
	FVector2D								RequestedScrollLocation;
	FVector2D								RequestedZoomArea;
	bool									RequestedAllowZoomIn;

	bool									bIsFirstTickCall;
	// Is user interacting with a node now
	bool									bHasNodeInteraction;

	// Snapping distance in screen units
	float									SnappingDistance;

	//
	// Mouse location in the world
	FVector2D								WorldMouseLocation;
	// Current marquee rectangle size in world units
	FVector2D								WorldMarqueeSize;

	FSlateTextureRenderTarget2DResource*	SharedThumbnailRT;
};

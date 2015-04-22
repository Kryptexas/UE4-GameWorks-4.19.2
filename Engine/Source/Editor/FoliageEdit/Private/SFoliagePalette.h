// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SFoliagePaletteTile;
class FEdModeFoliage;
struct FFoliageMeshUIInfo;
typedef TSharedPtr<FFoliageMeshUIInfo> FFoliageMeshUIInfoPtr; //should match typedef in FoliageEdMode.h

typedef STreeView<FFoliageMeshUIInfoPtr> SFoliageTypeTreeView;
typedef STileView<FFoliageMeshUIInfoPtr> SFoliageTypeTileView;

/** View modes supported by the foliage palette */
namespace EPaletteViewMode
{
	enum Type
	{
		Thumbnail,
		Tree
	};
}

/** The palette of foliage types available for use by the foliage edit mode */
class SFoliagePalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFoliagePalette) {}
		SLATE_ARGUMENT(FEdModeFoliage*, FoliageEdMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Refresh();
	
	bool AnySelectedTileHovered() const;
	void ActivateAllSelectedTypes(bool bActivate) const;

	/** @return True if the given view mode is the active view mode */
	bool IsActiveViewMode(EPaletteViewMode::Type ViewMode) const;

	/** @return True if tooltips should be shown when hovering over foliage type items in the palette */
	bool ShouldShowTooltips() const;

private:	// GENERAL

	/** Binds commands used by the palette */
	void BindCommands();

	/** Adds the foliage type asset to the foliage actor's list of meshes. */
	void AddFoliageType(const FAssetData& AssetData);
	
	/** Gets the asset picker for adding a foliage type. */
	TSharedRef<SWidget> GetAddFoliageTypePicker();

	/** Gets the visibility of the Add Foliage Type text in the header row button */
	EVisibility GetAddFoliageTypeButtonTextVisibility() const;

	/** Toggle whether all foliage types are active */
	ECheckBoxState GetState_AllMeshes() const;
	void OnCheckStateChanged_AllMeshes(ECheckBoxState InState);

	/** Handler to trigger a refresh of the details view when the active tool changes */
	void HandleOnToolChanged();

	/** Sets the view mode of the palette */
	void SetViewMode(EPaletteViewMode::Type NewViewMode);

	/** Sets whether to show tooltips when hovering over foliage type items in the palette */
	void ToggleShowTooltips();

	/** Switches the palette display between the tile and tree view */
	FReply OnToggleViewModeClicked();

	/** @return The index of the view widget to display */
	int32 GetActiveViewIndex() const;

	/** Handler for selection changes in either view */
	void OnSelectionChanged(FFoliageMeshUIInfoPtr Item, ESelectInfo::Type SelectInfo);

	/** Toggle the activation state of a type on a double-click */
	void OnItemDoubleClicked(FFoliageMeshUIInfoPtr Item) const;

	/** Creates the view options menu */
	TSharedRef<SWidget> GetViewOptionsMenuContent();

	TSharedPtr<SListView<FFoliageMeshUIInfoPtr>> GetActiveViewWidget() const;

	/** Gets the visibility of the "Drop Foliage Here" prompt for when the palette is empty */
	EVisibility GetDropFoliageHintVisibility() const;

private:	// CONTEXT MENU

	/** @return the SWidget containing the context menu */
	TSharedPtr<SWidget> ConstructFoliageTypeContextMenu();

	/** Saves a temporary non-asset foliage type (created from a static mesh) as a foliage type asset */
	void OnSaveSelectedAsFoliageType();

	/** @return True if all selected types are non-assets */
	bool OnCanSaveSelectedAsFoliageType() const;

	/** @return True if any of the selected types are non-assets */
	bool AreAnyNonAssetTypesSelected() const;

	/** Fills 'Replace' menu command  */
	void FillReplaceFoliageTypeSubmenu(FMenuBuilder& MenuBuilder);

	/** Handler for 'Replace' command  */
	void OnReplaceFoliageTypeSelected(const class FAssetData& AssetData);

	/** Handler for 'Remove' command  */
	void OnRemoveFoliageType();

	/** Handler for 'Show in CB' command  */
	void OnShowFoliageTypeInCB();

	/** Handler for 'Select All' command  */
	void OnSelectAllInstances();

	/** Handler for 'Deselect All' command  */
	void OnDeselectAllInstances();

	/** Handler for 'Select Invalid Instances' command  */
	void OnSelectInvalidInstances();

	/** @return Whether selecting instances is currently possible */
	bool CanSelectInstances() const;

private:	// THUMBNAIL VIEW

	/** Creates a thumbnail tile for the given foliage type */
	TSharedRef<ITableRow> GenerateTile(FFoliageMeshUIInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

private:	// TREE VIEW

	/** Generates a row widget for foliage mesh item */
	TSharedRef<ITableRow> TreeViewGenerateRow(FFoliageMeshUIInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Generates a list of children items for foliage item */
	void TreeViewGetChildren(FFoliageMeshUIInfoPtr Item, TArray<FFoliageMeshUIInfoPtr>& OutChildren);

	/** Text for foliage meshes list header */
	FText GetMeshesHeaderText() const;

	/** Mesh list sorting support */
	EColumnSortMode::Type GetMeshColumnSortMode() const;
	void OnMeshesColumnSortModeChanged(EColumnSortPriority::Type InPriority, const FName& InColumnName, EColumnSortMode::Type InSortMode);

	/** Tooltip text for 'Instance Count" column */
	FText GetTotalInstanceCountTooltipText() const;

private:	// DETAILS

	/** Refreshes the mesh details widget to match the current selection */
	void RefreshMeshDetailsWidget();

	/** Gets the text for the details area header */
	FText GetDetailsNameAreaText() const;

	/** Gets the text for the show/hide details button tooltip */
	FText GetShowHideDetailsTooltipText() const;

	/** Gets the image for the show/hide details button */
	const FSlateBrush* GetShowHideDetailsImage() const;

	/** Handles the show/hide details button click */
	FReply OnShowHideDetailsClicked() const;

private:

	/** Switches between the thumbnail and tree views */
	TSharedPtr<class SWidgetSwitcher> WidgetSwitcher;

	/** The Add Foliage Type combo button */
	TSharedPtr<class SComboButton> AddFoliageTypeCombo;

	/** The header row of the foliage mesh tree */
	TSharedPtr<class SHeaderRow> TreeViewHeaderRow;

	/** Foliage type thumbnails widget  */
	TSharedPtr<SFoliageTypeTileView> TileViewWidget;

	/** Foliage type tree widget  */
	TSharedPtr<SFoliageTypeTreeView> TreeViewWidget;

	/** Foliage mesh details widget  */
	TSharedPtr<class IDetailsView> DetailsWidget;

	/** Command list for binding functions for the context menu. */
	TSharedPtr<FUICommandList> UICommandList;

	FEdModeFoliage* FoliageEditMode;

	//@todo: Should be saved to config
	EPaletteViewMode::Type ActiveViewMode;
	bool bShowTooltips;
};
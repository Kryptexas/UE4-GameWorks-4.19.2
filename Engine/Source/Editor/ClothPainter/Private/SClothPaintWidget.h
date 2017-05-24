// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "SListView.h"
#include "SComboBox.h"

class IDetailsView;
class FClothPainter;
class UClothingAsset;
class UClothPainterSettings;
struct FClothParameterMask_PhysMesh;

struct FClothingAssetListItem
{
	TWeakObjectPtr<UClothingAsset> ClothingAsset;
};

struct FClothingMaskListItem
{
	FClothingMaskListItem()
		: ClothingAsset(nullptr)
		, LodIndex(INDEX_NONE)
		, MaskIndex(INDEX_NONE)
	{}

	FClothParameterMask_PhysMesh* GetMask();

	TWeakObjectPtr<UClothingAsset> ClothingAsset;
	int32 LodIndex;
	int32 MaskIndex;
};

struct FClothingAssetLodItem
{
	int32 Lod;
};

class SClothPaintWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClothPaintWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FClothPainter* InPainter);
	void CreateDetailsView(FClothPainter* InPainter);

	// Refresh the widget such as when entering the paint mode
	void OnRefresh();

	// Resets the selections and puts the widget back to starting state
	void Reset();

	// Currently selected clothing asset, Lod Index and Mask index
	TWeakObjectPtr<UClothingAsset> SelectedAsset;
	int32 SelectedLod;
	int32 SelectedMask;

protected:

	// Details view placed below asset selection
	TSharedPtr<IDetailsView> DetailsView;

	// Objects observed in the details view
	TArray<UObject*> Objects;

	// The painter instance this widget is using
	FClothPainter* Painter;

	// Settings for the painter instance
	UClothPainterSettings* ClothPainterSettings;

	// Setters for the list selections so we can handle list selections changing properly
	void SetSelectedAsset(TWeakObjectPtr<UClothingAsset> InSelectedAsset);
	void SetSelectedLod(int32 InLodIndex, bool bRefreshMasks = true);
	void SetSelectedMask(int32 InMaskIndex);

	// List types for this panel
	typedef SListView<TSharedPtr<FClothingAssetListItem>> SAssetList;
	typedef SListView<TSharedPtr<FClothingMaskListItem>> SMaskList;
	typedef SComboBox<TSharedPtr<FClothingAssetLodItem>> SLodList;

	// Asset List handling
	void RefreshClothingAssetListItems();
	TSharedRef<ITableRow> OnGenerateWidgetForClothingAssetItem(TSharedPtr<FClothingAssetListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnAssetListSelectionChanged(TSharedPtr<FClothingAssetListItem> InSelectedItem, ESelectInfo::Type InSelectInfo);
	TArray<TSharedPtr<FClothingAssetListItem>> AssetListItems;

	// Lod dropdown handling
	void RefreshClothingLodItems();
	TArray<TSharedPtr<FClothingAssetLodItem>> LodListItems;
	TSharedRef<SWidget> OnGenerateWidgetForLodItem(TSharedPtr<FClothingAssetLodItem> InItem);
	FText GetLodDropdownText() const;
	void OnClothingLodChanged(TSharedPtr<FClothingAssetLodItem> InSelectedItem, ESelectInfo::Type InSelectInfo);

	// Mask list handling
	void RefreshMaskListItems();
	TSharedRef<ITableRow> OnGenerateWidgetForMaskItem(TSharedPtr<FClothingMaskListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnMaskSelectionChanged(TSharedPtr<FClothingMaskListItem> InSelectedItem, ESelectInfo::Type InSelectInfo);
	TArray<TSharedPtr<FClothingMaskListItem>> MaskListItems;

	// Mask manipulation
	FReply AddNewMask();
	bool CanAddNewMask() const;

	// List widgets for asset, lod and mask tracked for refreshing
	TSharedPtr<SAssetList> AssetList;
	TSharedPtr<SMaskList> MaskList;
	TSharedPtr<SLodList> LodList;
};
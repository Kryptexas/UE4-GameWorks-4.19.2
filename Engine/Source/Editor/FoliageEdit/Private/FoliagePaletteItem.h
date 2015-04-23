// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SToolTip;
class SFoliagePalette;

class FEdModeFoliage;
struct FFoliageMeshUIInfo;

typedef TSharedPtr<FFoliageMeshUIInfo> FFoliageMeshUIInfoPtr; //should match typedef in FoliageEdMode.h

namespace FoliagePaletteTreeColumns
{
	/** IDs for list columns */
	static const FName ColumnID_ToggleActive("Toggle");
	static const FName ColumnID_Type("Type");
	static const FName ColumnID_InstanceCount("InstanceCount");
	static const FName ColumnID_Save("Save");
};

class FFoliagePaletteItemModel : public TSharedFromThis<FFoliagePaletteItemModel>
{
public:
	FFoliagePaletteItemModel(FFoliageMeshUIInfoPtr InTypeInfo, TSharedRef<SFoliagePalette> InFoliagePalette, FEdModeFoliage* InFoliageEditMode);

	/** @return The foliage palette that contains the item */
	const TSharedPtr<SFoliagePalette>& GetFoliagePalette() const;

	const class UFoliageType* GetFoliageType() const;

	/** @return The thumbnail widget for this item */
	TSharedRef<SWidget> GetThumbnailWidget() const;

	/** @return The tooltip widget for this item */
	TSharedRef<class SToolTip> CreateTooltipWidget() const;

	/** @return The checkbox widget for activating/deactivating this type in the palette */
	TSharedRef<class SCheckBox> CreateActivationCheckBox(TAttribute<bool> IsItemWidgetSelected, TAttribute<EVisibility> InVisibility = TAttribute<EVisibility>());

	/** @return The save asset button */
	TSharedRef<class SButton> CreateSaveAssetButton(TAttribute<EVisibility> InVisibility = TAttribute<EVisibility>());

	/** Gets the name to display of the foliage type object as text */
	FText GetFoliageTypeNameText() const;

	/** Gets the number of instances of this foliage type exist in the level */
	FText GetInstanceCountText() const;

private:
	/** Handles the change in activation of the item in the palette */
	void HandleCheckStateChanged(const ECheckBoxState NewCheckedState, TAttribute<bool> IsItemWidgetSelected);

	/** Gets whether the foliage type is active in the palette  */
	ECheckBoxState GetCheckBoxState() const;

	/** Whether the save asset button is enabled */
	bool IsSaveEnabled() const;

	/** Saves the foliage type */
	FReply HandleSaveAsset();

	/** Gets the visibility of the entire tooltip */
	EVisibility GetTooltipVisibility() const;

	/** Gets the visibility of the thumbnail in the tooltip */
	EVisibility GetTooltipThumbnailVisibility() const;

private:
	TSharedPtr<SWidget> ThumbnailWidget;

	FFoliageMeshUIInfoPtr TypeInfo;
	TSharedPtr<SFoliagePalette> FoliagePalette;
	FEdModeFoliage* FoliageEditMode;
};

/** A tile representing a foliage type in the palette */
class SFoliagePaletteItemTile : public STableRow<FFoliageMeshUIInfoPtr>
{
public:
	SLATE_BEGIN_ARGS(SFoliagePaletteItemTile) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView, TSharedPtr<FFoliagePaletteItemModel>& InModel);

private:
	FLinearColor GetTileColorAndOpacity() const;
	EVisibility GetCheckBoxVisibility() const;
	EVisibility GetSaveButtonVisibility() const;

private:
	TSharedPtr<FFoliagePaletteItemModel> Model;
};

/** A tree row representing a foliage type in the palette */
class SFoliagePaletteItemRow : public SMultiColumnTableRow<FFoliageMeshUIInfoPtr>
{
public:
	SLATE_BEGIN_ARGS(SFoliagePaletteItemRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView, TSharedPtr<FFoliagePaletteItemModel>& InModel);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FFoliagePaletteItemModel> Model;
};
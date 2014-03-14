// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// STileLayerItem

class STileLayerItem : public SMultiColumnTableRow<class UPaperTileLayer*>
{
public:
	SLATE_BEGIN_ARGS(STileLayerItem) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UPaperTileLayer* InItem, const TSharedRef<STableViewBase>& OwnerTable);

	// SMultiColumnTableRow<> interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) OVERRIDE;
	// End of SMultiColumnTableRow<> interface
protected:
	class UPaperTileLayer* MyLayer;

	TSharedPtr<SButton> VisibilityButton;
protected:
	FText GetLayerDisplayName() const;
	void OnLayerNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo);

	const FSlateBrush* GetVisibilityBrushForLayer() const;
	FSlateColor GetForegroundColorForVisibilityButton() const;
	FReply OnToggleVisibility();

	void OnChangeLayerTileSet(UObject* NewAsset);
	UObject* GetLayerTileset() const;
};

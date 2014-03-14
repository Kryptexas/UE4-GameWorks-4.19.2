// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SDetailTableRowBase.h"

/**
 * A widget for details that span the entire tree row and have no columns                                                              
 */
class SDetailSingleItemRow : public SDetailTableRowBase
{
public:
	SLATE_BEGIN_ARGS( SDetailSingleItemRow )
		: _ColumnSizeData() {}

		SLATE_ARGUMENT( FDetailColumnSizeData, ColumnSizeData )
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 */
	void Construct( const FArguments& InArgs, FDetailLayoutCustomization* InCustomization, bool bHasMultipleColumns, TSharedRef<IDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView );

protected:
	virtual bool OnContextMenuOpening( FMenuBuilder& MenuBuilder ) OVERRIDE;
private:
	void OnLeftColumnResized( float InNewWidth );
	void OnCopyProperty();
	void OnPasteProperty();
	bool CanPasteProperty() const;
private:
	/** Customization for this widget */
	FDetailLayoutCustomization* Customization;
	FDetailColumnSizeData ColumnSizeData;
};
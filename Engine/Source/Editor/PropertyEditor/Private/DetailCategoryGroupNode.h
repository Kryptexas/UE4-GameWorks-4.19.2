// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SDetailTableRowBase.h"

class SDetailCategoryTableRow : public SDetailTableRowBase
{
public:
	SLATE_BEGIN_ARGS( SDetailCategoryTableRow )
		: _InnerCategory( false )
	{}
		SLATE_TEXT_ARGUMENT( DisplayName )
		SLATE_ARGUMENT( bool, InnerCategory )
		SLATE_ARGUMENT( TSharedPtr<SWidget>, HeaderContent )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<IDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView );
private:
	EVisibility IsSeparatorVisible() const;
	const FSlateBrush* GetBackgroundImage() const;
private:
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) OVERRIDE;
private:
	bool bIsInnerCategory;
};


class FDetailCategoryGroupNode : public IDetailTreeNode, public TSharedFromThis<FDetailCategoryGroupNode>
{
public:
	FDetailCategoryGroupNode( const FDetailNodeList& InChildNodes, FName InGroupName, FDetailCategoryImpl& InParentCategory );

private:
	virtual SDetailsView& GetDetailsView() const OVERRIDE { return ParentCategory.GetDetailsView(); }
	virtual void OnItemExpansionChanged( bool bIsExpanded ) OVERRIDE {}
	virtual bool ShouldBeExpanded() const OVERRIDE { return true; }
	virtual ENodeVisibility::Type GetVisibility() const OVERRIDE { return bShouldBeVisible ? ENodeVisibility::Visible : ENodeVisibility::HiddenDueToFiltering; }
	virtual TSharedRef< ITableRow > GenerateNodeWidget( const TSharedRef<STableViewBase>& OwnerTable, const FDetailColumnSizeData& ColumnSizeData, const TSharedRef<IPropertyUtilities>& PropertyUtilities ) OVERRIDE;
	virtual void GetChildren( TArray< TSharedRef<IDetailTreeNode> >& OutChildren )  OVERRIDE;
	virtual void FilterNode( const FDetailFilter& InFilter ) OVERRIDE;
	virtual void Tick( float DeltaTime ) OVERRIDE {}
	virtual bool ShouldShowOnlyChildren() const OVERRIDE { return false; }
	virtual FName GetNodeName() const OVERRIDE { return NAME_None; }
private:
	FDetailNodeList ChildNodes;
	FDetailCategoryImpl& ParentCategory;
	FName GroupName;
	bool bShouldBeVisible;
};

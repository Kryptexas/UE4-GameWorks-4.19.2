// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * A single item in a detail tree                                                              
 */
class FDetailItemNode : public IDetailTreeNode, public TSharedFromThis<FDetailItemNode>
{
public:
	FDetailItemNode( const FDetailLayoutCustomization& InCustomization, TSharedRef<FDetailCategoryImpl> InParentCategory, TAttribute<bool> InIsParentEnabled );
	~FDetailItemNode();

	/**
	 * Initializes this node                                                              
	 */
	void Initialize();


	void ToggleExpansion();

	/**
	 * Generates children for this node
	 *
	 * @param bUpdateFilteredNodes If true, details panel will re-filter to account for new nodes being added
	 */
	void GenerateChildren( bool bUpdateFilteredNodes );

	/**
	 * @return TRUE if this node has a widget with multiple columns                                                              
	 */
	bool HasMultiColumnWidget() const;

	/** IDetailTreeNode interface */
	virtual SDetailsView& GetDetailsView() const OVERRIDE { return ParentCategory.Pin()->GetDetailsView(); }
	virtual TSharedRef< ITableRow > GenerateNodeWidget( const TSharedRef<STableViewBase>& OwnerTable, const FDetailColumnSizeData& ColumnSizeData, const TSharedRef<IPropertyUtilities>& PropertyUtilities ) OVERRIDE;
	virtual void GetChildren( FDetailNodeList& OutChildren )  OVERRIDE;
	virtual void OnItemExpansionChanged( bool bInIsExpanded ) OVERRIDE;
	virtual bool ShouldBeExpanded() const OVERRIDE;
	virtual ENodeVisibility::Type GetVisibility() const OVERRIDE;
	virtual void FilterNode( const FDetailFilter& InFilter ) OVERRIDE;
	virtual void Tick( float DeltaTime ) OVERRIDE;
	virtual bool ShouldShowOnlyChildren() const OVERRIDE;
	virtual FName GetNodeName() const OVERRIDE;

private:

	/**
	 * Initializes the property editor on this node                                                              
	 */
	void InitPropertyEditor();

	/**
	 * Initializes the custom builder on this node                                                              
	 */
	void InitCustomBuilder();

	/**
	 * Initializes the detail group on this node                                                              
	 */
	void InitGroup();

private:
	/** Customization on this node */
	FDetailLayoutCustomization Customization;
	/** Child nodes of this node */
	FDetailNodeList Children;
	/** Parent categories on this node */
	TWeakPtr<FDetailCategoryImpl> ParentCategory;
	/** Attribute for checking if our parent is enabled */
	TAttribute<bool> IsParentEnabled;
	/** Cached visibility of this node */
	EVisibility CachedItemVisibility;
	/** True if this node passes filtering */
	bool bShouldBeVisibleDueToFiltering;
	/** True if this node is visible because its children are filtered successfully */
	bool bShouldBeVisibleDueToChildFiltering;
	/** True if this node should be ticked */
	bool bTickable;
	/** True if this node is expanded */
	bool bIsExpanded;
};
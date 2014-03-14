// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FDetailGroup : public IDetailGroup, public TSharedFromThis<FDetailGroup>
{
public:
	FDetailGroup( const FName InGroupName, TSharedRef<FDetailCategoryImpl> InParentCategory, const FString& InLocalizedDisplayName );

	/** IDetailGroup interface */     
	virtual FDetailWidgetRow& HeaderRow() OVERRIDE;
	virtual IDetailPropertyRow& HeaderProperty( TSharedRef<IPropertyHandle> PropertyHandle ) OVERRIDE;
	virtual FDetailWidgetRow& AddWidgetRow() OVERRIDE;
	virtual IDetailPropertyRow& AddPropertyRow( TSharedRef<IPropertyHandle> PropertyHandle ) OVERRIDE;
	virtual void ToggleExpansion( bool bExpand ) OVERRIDE;

	/** @return The name of the group */
	FName GetGroupName() const { return GroupName; }

	/** Whether or not the group has columns */
	bool HasColumns() const;

	/** @return true if this row should be ticked */
	bool RequiresTick() const;

	/** 
	 * @return The visibility of this group
	 */
	EVisibility GetGroupVisibility() const;

	/**
	 * Called by the owning item node when it has been initialized
	 *
	 * @param InTreeNode		The node that owns this group
	 * @param InParentCategory	The category this group is located in
	 * @param InIsParentEnabled	Whether or not the parent node is enabled
	 */
	void OnItemNodeInitialized( TSharedRef<class FDetailItemNode> InTreeNode, TSharedRef<FDetailCategoryImpl> InParentCategory, const TAttribute<bool>& InIsParentEnabled );

	/** @return the row which should be displayed for this group */
	FDetailWidgetRow GetWidgetRow();

	/**
	 * Called to generate children of this group
	 *
	 * @param OutChildren	The list of children to add to
	 */
	void OnGenerateChildren( FDetailNodeList& OutChildren );
private:
	/**
	 * Called when the name of the group is clicked to expand the group
	 */
	FReply OnNameClicked();

	/** 
	 * Makes a name widget for this group 
	 */
	TSharedRef<SWidget> MakeNameWidget();
private:
	/** Customized group children */
	TArray<FDetailLayoutCustomization> GroupChildren;
	/** User customized header row */
	TSharedPtr<FDetailLayoutCustomization> HeaderCustomization;
	/** Owner node of this group */
	TWeakPtr<class FDetailItemNode> OwnerTreeNode;
	/** Parent category of this group */
	TWeakPtr<FDetailCategoryImpl> ParentCategory;
	/** Whether or not our parent is enabled */
	TAttribute<bool> IsParentEnabled;
	/** Display name of this group */
	FString LocalizedDisplayName;
	/** Name identifier of this group */
	FName GroupName;
};
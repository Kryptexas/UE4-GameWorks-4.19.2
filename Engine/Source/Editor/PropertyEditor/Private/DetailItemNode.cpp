// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "DetailItemNode.h"
#include "PropertyEditorHelpers.h"
#include "DetailCategoryGroupNode.h"
#include "PropertyHandleImpl.h"
#include "DetailGroup.h"
#include "DetailPropertyRow.h"
#include "DetailCustomBuilderRow.h"
#include "SDetailSingleItemRow.h"



FDetailItemNode::FDetailItemNode(const FDetailLayoutCustomization& InCustomization, TSharedRef<FDetailCategoryImpl> InParentCategory, TAttribute<bool> InIsParentEnabled )
	: Customization( InCustomization )
	, ParentCategory( InParentCategory )
	, bShouldBeVisibleDueToFiltering( false )
	, bShouldBeVisibleDueToChildFiltering( false )
	, CachedItemVisibility( EVisibility::Visible )
	, IsParentEnabled( InIsParentEnabled )
	, bTickable( false )
	, bIsExpanded( InCustomization.HasCustomBuilder() ? !InCustomization.CustomBuilderRow->IsInitiallyCollapsed() : false )
{
	
}

void FDetailItemNode::Initialize()
{
	if( ( Customization.HasCustomWidget() && Customization.WidgetDecl->VisibilityAttr.IsBound() )
		|| ( Customization.HasCustomBuilder() && Customization.CustomBuilderRow->RequiresTick() )
		|| ( Customization.HasPropertyNode() && Customization.PropertyRow->RequiresTick() )
		|| ( Customization.HasGroup() && Customization.DetailGroup->RequiresTick() ) )
	{
		// The node needs to be ticked because it has widgets that can dynamically come and go
		bTickable = true;
		ParentCategory.Pin()->AddTickableNode( *this );
	}

	if( Customization.HasPropertyNode() )
	{
		InitPropertyEditor();
	}
	else if( Customization.HasCustomBuilder() )
	{
		InitCustomBuilder();
	}
	else if( Customization.HasGroup() )
	{
		InitGroup();
	}

	// Cache the visibility of customizations that can set it
	if( Customization.HasCustomWidget() )
	{	
		CachedItemVisibility = Customization.WidgetDecl->VisibilityAttr.Get();
	}
	else if( Customization.HasPropertyNode() )
	{
		CachedItemVisibility = Customization.PropertyRow->GetPropertyVisibility();
	}
	else if( Customization.HasGroup() )
	{
		CachedItemVisibility = Customization.DetailGroup->GetGroupVisibility();
	}

	const bool bUpdateFilteredNodes = false;
	GenerateChildren( bUpdateFilteredNodes );
}

FDetailItemNode::~FDetailItemNode()
{
	if( bTickable && ParentCategory.IsValid() )
	{
		ParentCategory.Pin()->RemoveTickableNode( *this );
	}
}

void FDetailItemNode::InitPropertyEditor()
{
	if( Customization.GetPropertyNode()->GetProperty() && Customization.GetPropertyNode()->GetProperty()->IsA<UArrayProperty>() )
	{
		const bool bUpdateFilteredNodes = false;
		FSimpleDelegate OnRegenerateChildren = FSimpleDelegate::CreateSP( this, &FDetailItemNode::GenerateChildren, bUpdateFilteredNodes );

		Customization.GetPropertyNode()->SetOnRebuildChildren( OnRegenerateChildren );
	}

	Customization.PropertyRow->OnItemNodeInitialized( ParentCategory.Pin().ToSharedRef(), IsParentEnabled );
}

void FDetailItemNode::InitCustomBuilder()
{
	Customization.CustomBuilderRow->OnItemNodeInitialized( AsShared(), ParentCategory.Pin().ToSharedRef(), IsParentEnabled );

	// Restore saved expansion state
	FName BuilderName = Customization.CustomBuilderRow->GetCustomBuilderName();
	if( BuilderName != NAME_None )
	{
		bIsExpanded = ParentCategory.Pin()->GetSavedExpansionState( *this );
	}

}

void FDetailItemNode::InitGroup()
{
	Customization.DetailGroup->OnItemNodeInitialized( AsShared(), ParentCategory.Pin().ToSharedRef(), IsParentEnabled );

	// Restore saved expansion state
	FName GroupName = Customization.DetailGroup->GetGroupName();
	if( GroupName != NAME_None )
	{
		bIsExpanded = ParentCategory.Pin()->GetSavedExpansionState( *this );
	}

}

bool FDetailItemNode::HasMultiColumnWidget() const
{
	return ( Customization.HasCustomWidget() && Customization.WidgetDecl->HasColumns() )
		|| ( Customization.HasCustomBuilder() && Customization.CustomBuilderRow->HasColumns() )
		|| ( Customization.HasGroup() && Customization.DetailGroup->HasColumns() )
		|| ( Customization.HasPropertyNode() && Customization.PropertyRow->HasColumns());
}

void FDetailItemNode::ToggleExpansion()
{
	bIsExpanded = !bIsExpanded;
	
	// Expand the child after filtering if it wants to be expanded
	ParentCategory.Pin()->RequestItemExpanded( AsShared(), bIsExpanded );

	OnItemExpansionChanged( bIsExpanded );
}

TSharedRef< ITableRow > FDetailItemNode::GenerateNodeWidget( const TSharedRef<STableViewBase>& OwnerTable, const FDetailColumnSizeData& ColumnSizeData, const TSharedRef<IPropertyUtilities>& PropertyUtilities )
{
	if( Customization.HasPropertyNode() && Customization.GetPropertyNode()->AsCategoryNode() )
	{
		return 
			SNew( SDetailCategoryTableRow, AsShared(), OwnerTable )
			.IsEnabled( IsParentEnabled )
			.DisplayName( Customization.GetPropertyNode()->GetDisplayName() )
			.InnerCategory( true );
	}
	else
	{
		return
			SNew(SDetailSingleItemRow, &Customization, HasMultiColumnWidget(), AsShared(), OwnerTable )
			.IsEnabled( IsParentEnabled )
			.ColumnSizeData( ColumnSizeData ) ;
	}
}

void FDetailItemNode::GetChildren( FDetailNodeList& OutChildren )
{
	for( int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex )
	{
		TSharedRef<IDetailTreeNode>& Child = Children[ChildIndex];

		ENodeVisibility::Type ChildVisibility = Child->GetVisibility();

		// Report the child if the child is visible or we are visible due to filtering and there were no filtered children.  
		// If we are visible due to filtering and so is a child, we only show that child.  
		// If we are visible due to filtering and no child is visible, we show all children

		if( ChildVisibility == ENodeVisibility::Visible ||
			( !bShouldBeVisibleDueToChildFiltering && bShouldBeVisibleDueToFiltering && ChildVisibility != ENodeVisibility::ForcedHidden ) )
		{
			if( Child->ShouldShowOnlyChildren() )
			{
				Child->GetChildren( OutChildren );
			}
			else
			{
				OutChildren.Add( Child );
			}
		}
	}
}

void FDetailItemNode::GenerateChildren( bool bUpdateFilteredNodes )
{
	Children.Empty();

	if( Customization.HasPropertyNode() )
	{
		Customization.PropertyRow->OnGenerateChildren( Children );
	}
	else if( Customization.HasCustomBuilder() )
	{
		Customization.CustomBuilderRow->OnGenerateChildren( Children );

		// Need to refresh the tree for custom builders as we could be regenerating children at any point
		ParentCategory.Pin()->RefreshTree( bUpdateFilteredNodes );
	}
	else if( Customization.HasGroup() )
	{
		Customization.DetailGroup->OnGenerateChildren( Children );
	}
}


void FDetailItemNode::OnItemExpansionChanged( bool bInIsExpanded )
{
	bIsExpanded = bInIsExpanded;
	if( Customization.HasPropertyNode() )
	{
		Customization.GetPropertyNode()->SetNodeFlags( EPropertyNodeFlags::Expanded, bInIsExpanded );
	}

	if( ParentCategory.IsValid() && ( ( Customization.HasCustomBuilder() && Customization.CustomBuilderRow->GetCustomBuilderName() != NAME_None )
		 || ( Customization.HasGroup() && Customization.DetailGroup->GetGroupName() != NAME_None ) ) )
	{
		ParentCategory.Pin()->SaveExpansionState( *this );
	}
}

bool FDetailItemNode::ShouldBeExpanded() const
{
	bool bShouldBeExpanded = bIsExpanded || bShouldBeVisibleDueToChildFiltering;
	if( Customization.HasPropertyNode() )
	{
		FPropertyNode& PropertyNode = *Customization.GetPropertyNode();
		bShouldBeExpanded = PropertyNode.HasNodeFlags( EPropertyNodeFlags::Expanded ) != 0;
		bShouldBeExpanded |= PropertyNode.HasNodeFlags( EPropertyNodeFlags::IsSeenDueToChildFiltering ) != 0;
	}
	return bShouldBeExpanded;
}

ENodeVisibility::Type FDetailItemNode::GetVisibility() const
{
	const bool bHasAnythingToShow = Customization.IsValidCustomization();

	const bool bIsForcedHidden = 
		!bHasAnythingToShow 
		|| (Customization.HasCustomWidget() && Customization.WidgetDecl->VisibilityAttr.Get() != EVisibility::Visible )
		|| (Customization.HasPropertyNode() && Customization.PropertyRow->GetPropertyVisibility() != EVisibility::Visible );

	ENodeVisibility::Type Visibility;
	if( bIsForcedHidden )
	{
		Visibility = ENodeVisibility::ForcedHidden;
	}
	else
	{
		Visibility = (bShouldBeVisibleDueToFiltering || bShouldBeVisibleDueToChildFiltering) ? ENodeVisibility::Visible : ENodeVisibility::HiddenDueToFiltering;
	}

	return Visibility;
}

static bool PassesAllFilters( const FDetailLayoutCustomization& InCustomization, const FDetailFilter& InFilter )
{
	bool bPassesAllFilters = true;
	
	if( InFilter.FilterStrings.Num() > 0 || InFilter.bShowOnlyModifiedProperties == true )
	{
		TSharedPtr<FPropertyNode> PropertyNodePin = InCustomization.GetPropertyNode();

		bPassesAllFilters = false;
		if( PropertyNodePin.IsValid() && !PropertyNodePin->AsCategoryNode() )
		{
			const bool bPassesSearchFilter =  InFilter.FilterStrings.Num()  == 0 || ( ( PropertyNodePin->HasNodeFlags( EPropertyNodeFlags::IsBeingFiltered ) == 0 || PropertyNodePin->HasNodeFlags( EPropertyNodeFlags::IsSeenDueToFiltering ) || PropertyNodePin->HasNodeFlags( EPropertyNodeFlags::IsParentSeenDueToFiltering ) ) );
			const bool bPassesModifiedFilter = bPassesSearchFilter && ( InFilter.bShowOnlyModifiedProperties == false || PropertyNodePin->GetDiffersFromDefault() == true );
			// The property node is visible (note categories are never visible unless they have a child that is visible )
			bPassesAllFilters = bPassesSearchFilter && bPassesModifiedFilter;
		}
		else if( InCustomization.HasCustomWidget() )
		{		
			if( InFilter.FilterStrings.Num() > 0 && InCustomization.WidgetDecl->FilterTextString.Len() > 0  )
			{
				// We default to acceptable
				bPassesAllFilters = true;

				const FString& FilterMatch = InCustomization.WidgetDecl->FilterTextString;

				// Make sure the filter match matches all filter strings
				for (int32 TestNameIndex = 0; TestNameIndex < InFilter.FilterStrings.Num(); ++TestNameIndex)
				{
					const FString& TestName =  InFilter.FilterStrings[TestNameIndex];

					if ( !FilterMatch.Contains( TestName) ) 
					{
						bPassesAllFilters = false;
						break;
					}
				}
			}
		}

	}

	return bPassesAllFilters;
}

void FDetailItemNode::Tick( float DeltaTime )
{
	if( ensure( bTickable ) )
	{
		if( Customization.HasCustomBuilder() && Customization.CustomBuilderRow->RequiresTick() ) 
		{
			Customization.CustomBuilderRow->Tick( DeltaTime );
		}

		// Recache visibility
		EVisibility NewVisibility;
		if( Customization.HasCustomWidget() )
		{	
			NewVisibility = Customization.WidgetDecl->VisibilityAttr.Get();
		}
		else if( Customization.HasPropertyNode() )
		{
			NewVisibility = Customization.PropertyRow->GetPropertyVisibility();
		}
		else if( Customization.HasGroup() )
		{
			NewVisibility = Customization.DetailGroup->GetGroupVisibility();
		}
	
		if( CachedItemVisibility != NewVisibility )
		{
			// The visibility of a node in the tree has changed.  We must refresh the tree to remove the widget
			CachedItemVisibility = NewVisibility;
			const bool bRefilterCategory = true;
			ParentCategory.Pin()->RefreshTree( bRefilterCategory );
		}
	}
}

bool FDetailItemNode::ShouldShowOnlyChildren() const
{
	// Show only children of this node if there is no content for custom details or the property node requests that only children be shown
	return ( Customization.HasCustomBuilder() && Customization.CustomBuilderRow->ShowOnlyChildren() )
		 || (Customization.HasPropertyNode() && Customization.PropertyRow->ShowOnlyChildren() );
}

FName FDetailItemNode::GetNodeName() const
{
	if( Customization.HasCustomBuilder() )
	{
		return Customization.CustomBuilderRow->GetCustomBuilderName();
	}
	else if( Customization.HasGroup() )
	{
		return Customization.DetailGroup->GetGroupName();
	}

	return NAME_None;
}

void FDetailItemNode::FilterNode( const FDetailFilter& InFilter )
{
	bShouldBeVisibleDueToFiltering = PassesAllFilters( Customization, InFilter );

	bShouldBeVisibleDueToChildFiltering = false;

	// Filter each child
	for( int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex )
	{
		TSharedRef<IDetailTreeNode>& Child = Children[ChildIndex];

		Child->FilterNode( InFilter );

		if( Child->GetVisibility() == ENodeVisibility::Visible )
		{
			if( !InFilter.IsEmptyFilter() )
			{
				// The child is visible due to filtering so we must also be visible
				bShouldBeVisibleDueToChildFiltering = true;
			}

			// Expand the child after filtering if it wants to be expanded
			ParentCategory.Pin()->RequestItemExpanded( Child, Child->ShouldBeExpanded() );
		}
	}
}

// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginsEditorPrivatePCH.h"
#include "SPluginCategories.h"
#include "PluginCategoryTreeItem.h"
#include "IPluginManager.h"
#include "SPluginCategoryTreeItem.h"
#include "SPluginsEditor.h"

#define LOCTEXT_NAMESPACE "PluginCategories"



void SPluginCategories::Construct( const FArguments& Args, const TSharedRef< SPluginsEditor > Owner )
{
	OwnerWeak = Owner;

	RootPluginCategories.Reset();

	// Add the root level categories
	TSharedRef<FPluginCategoryTreeItem> InstalledCategory = MakeShareable(new FPluginCategoryTreeItem(NULL, TEXT("Installed"), LOCTEXT("InstalledCategoryName", "Installed")));
	RootPluginCategories.Add( InstalledCategory );
	TSharedRef<FPluginCategoryTreeItem> ProjectCategory = MakeShareable(new FPluginCategoryTreeItem(NULL, TEXT("Project"), LOCTEXT("ProjectCategoryName", "Project")));
	RootPluginCategories.Add( ProjectCategory );

	PluginCategoryTreeView =
		SNew( SPluginCategoryTreeView )

		// For now we only support selecting a single folder in the tree
		.SelectionMode( ESelectionMode::Single )
		.ClearSelectionOnClick( false )		// Don't allow user to select nothing.  We always expect a category to be selected!

		.TreeItemsSource( &RootPluginCategories )
		.OnGenerateRow( this, &SPluginCategories::PluginCategoryTreeView_OnGenerateRow ) 
		.OnGetChildren( this, &SPluginCategories::PluginCategoryTreeView_OnGetChildren )

		.OnSelectionChanged( this, &SPluginCategories::PluginCategoryTreeView_OnSelectionChanged )
		;

	// Expand the root categories by default
	for( auto RootCategoryIt( RootPluginCategories.CreateConstIterator() ); RootCategoryIt; ++RootCategoryIt )
	{
		const auto& Category = *RootCategoryIt;
		PluginCategoryTreeView->SetItemExpansion( Category, true );
	}

	RebuildAndFilterCategoryTree();

	ChildSlot.AttachWidget( PluginCategoryTreeView.ToSharedRef() );
}


SPluginCategories::~SPluginCategories()
{
}


/** @return Gets the owner of this list */
SPluginsEditor& SPluginCategories::GetOwner()
{
	return *OwnerWeak.Pin();
}


void SPluginCategories::RebuildAndFilterCategoryTree()
{
	// Get a plugin from the currently selected category, so we can track it if it's removed
	IPlugin* TrackPlugin = nullptr;
	for(FPluginCategoryTreeItemPtr SelectedItem: PluginCategoryTreeView->GetSelectedItems())
	{
		if(SelectedItem->Plugins.Num() > 0)
		{
			TrackPlugin = SelectedItem->Plugins[0];
			break;
		}
	}

	// Clear the list of plugins in each current category
	for(FPluginCategoryTreeItemPtr RootCategory: RootPluginCategories)
	{
		for(FPluginCategoryTreeItemPtr Category: RootCategory->AccessSubCategories())
		{
			Category->Plugins.Reset();
		}
	}

	// Add all the known plugins into categories
	FPluginCategoryTreeItemPtr SelectCategory;
	for(IPlugin* Plugin: IPluginManager::Get().GetDiscoveredPlugins())
	{
		// Figure out which base category this plugin belongs in
		FPluginCategoryTreeItemPtr RootCategory;
		if( Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine )
		{
			RootCategory = RootPluginCategories[0];
		}
		else
		{
			RootCategory = RootPluginCategories[1];
		}

		// Get the subcategory for this plugin
		FString CategoryName = Plugin->GetDescriptor().Category;
		if(CategoryName.IsEmpty())
		{
			CategoryName = TEXT("Other");
		}

		// Locate this category at the level we're at in the hierarchy
		FPluginCategoryTreeItemPtr FoundCategory = NULL;
		for(FPluginCategoryTreeItemPtr TestCategory: RootCategory->SubCategories)
		{
			if(TestCategory->GetCategoryName() == CategoryName)
			{
				FoundCategory = TestCategory;
				break;
			}
		}

		if( !FoundCategory.IsValid() )
		{
			//@todo Allow for properly localized category names [3/7/2014 justin.sargent]
			FoundCategory = MakeShareable(new FPluginCategoryTreeItem(RootCategory, CategoryName, FText::FromString(CategoryName)));
			RootCategory->SubCategories.Add( FoundCategory );
		}
			
		// Associate the plugin with the category
		FoundCategory->AddPlugin(Plugin);

		// Update the selection if this is the plugin we're tracking
		if(TrackPlugin == Plugin)
		{
			SelectCategory = FoundCategory;
		}
	}

	// Remove any empty categories, keeping track of which items are still selected
	for(FPluginCategoryTreeItemPtr RootCategory: RootPluginCategories)
	{
		for(int32 Idx = 0; Idx < RootCategory->SubCategories.Num(); Idx++)
		{
			if(RootCategory->SubCategories[Idx]->Plugins.Num() == 0)
			{
				RootCategory->SubCategories.RemoveAt(Idx);
			}
		}
	}

	// Sort every single category alphabetically
	for(FPluginCategoryTreeItemPtr RootCategory: RootPluginCategories)
	{
		RootCategory->SubCategories.Sort([](const FPluginCategoryTreeItemPtr& A, const FPluginCategoryTreeItemPtr& B) -> bool { return A->CategoryName < B->CategoryName; });
	}

	// Refresh the view
	PluginCategoryTreeView->RequestTreeRefresh();

	// Make sure we have something selected
	if(SelectCategory.IsValid())
	{
		PluginCategoryTreeView->SetSelection(SelectCategory);
	}
	else if(RootPluginCategories[0]->SubCategories.Num() > 0)
	{
		PluginCategoryTreeView->SetSelection(RootPluginCategories[0]->SubCategories[0]);
	}
	else
	{
		PluginCategoryTreeView->SetSelection(RootPluginCategories[0]);
	}
}

TSharedRef<ITableRow> SPluginCategories::PluginCategoryTreeView_OnGenerateRow( FPluginCategoryTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew( STableRow< FPluginCategoryTreeItemPtr >, OwnerTable )
		[
			SNew( SPluginCategoryTreeItem, SharedThis( this ), Item.ToSharedRef() )
		];
}


void SPluginCategories::PluginCategoryTreeView_OnGetChildren( FPluginCategoryTreeItemPtr Item, TArray< FPluginCategoryTreeItemPtr >& OutChildren )
{
	const auto& SubCategories = Item->GetSubCategories();
	OutChildren.Append( SubCategories );
}


void SPluginCategories::PluginCategoryTreeView_OnSelectionChanged( FPluginCategoryTreeItemPtr Item, ESelectInfo::Type SelectInfo )
{
	// Selection changed, which may affect which plugins are displayed in the list.  We need to invalidate the list.
	OwnerWeak.Pin()->OnCategorySelectionChanged();
}


FPluginCategoryTreeItemPtr SPluginCategories::GetSelectedCategory() const
{
	if( PluginCategoryTreeView.IsValid() )
	{
		auto SelectedItems = PluginCategoryTreeView->GetSelectedItems();
		if( SelectedItems.Num() > 0 )
		{
			const auto& SelectedCategoryItem = SelectedItems[ 0 ];
			return SelectedCategoryItem;
		}
	}

	return NULL;
}

void SPluginCategories::SelectCategory( const FPluginCategoryTreeItemPtr& CategoryToSelect )
{
	if( ensure( PluginCategoryTreeView.IsValid() ) )
	{
		PluginCategoryTreeView->SetSelection( CategoryToSelect );
	}
}

bool SPluginCategories::IsItemExpanded( const FPluginCategoryTreeItemPtr Item ) const
{
	return PluginCategoryTreeView->IsItemExpanded( Item );
}

void SPluginCategories::SetNeedsRefresh()
{
	RegisterActiveTimer (0.f, FWidgetActiveTimerDelegate::CreateSP (this, &SPluginCategories::TriggerCategoriesRefresh));
}

EActiveTimerReturnType SPluginCategories::TriggerCategoriesRefresh(double InCurrentTime, float InDeltaTime)
{
	RebuildAndFilterCategoryTree();
	return EActiveTimerReturnType::Stop;
}

#undef LOCTEXT_NAMESPACE
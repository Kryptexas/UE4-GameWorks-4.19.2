// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginBrowserPrivatePCH.h"
#include "SPluginCategoryTree.h"
#include "IPluginManager.h"
#include "SPluginCategory.h"
#include "SPluginBrowser.h"

#define LOCTEXT_NAMESPACE "PluginCategories"



void SPluginCategoryTree::Construct( const FArguments& Args, const TSharedRef< SPluginBrowser > Owner )
{
	OwnerWeak = Owner;

	RootPluginCategories.Reset();

	// Add the root level categories
	TSharedRef<FPluginCategory> InstalledCategory = MakeShareable(new FPluginCategory(NULL, TEXT("Installed"), LOCTEXT("InstalledCategoryName", "Installed")));
	RootPluginCategories.Add( InstalledCategory );
	TSharedRef<FPluginCategory> ProjectCategory = MakeShareable(new FPluginCategory(NULL, TEXT("Project"), LOCTEXT("ProjectCategoryName", "Project")));
	RootPluginCategories.Add( ProjectCategory );

	PluginCategoryTreeView =
		SNew( STreeView<TSharedPtr<FPluginCategory>> )

		// For now we only support selecting a single folder in the tree
		.SelectionMode( ESelectionMode::Single )
		.ClearSelectionOnClick( false )		// Don't allow user to select nothing.  We always expect a category to be selected!

		.TreeItemsSource( &RootPluginCategories )
		.OnGenerateRow( this, &SPluginCategoryTree::PluginCategoryTreeView_OnGenerateRow ) 
		.OnGetChildren( this, &SPluginCategoryTree::PluginCategoryTreeView_OnGetChildren )

		.OnSelectionChanged( this, &SPluginCategoryTree::PluginCategoryTreeView_OnSelectionChanged )
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


SPluginCategoryTree::~SPluginCategoryTree()
{
}


/** @return Gets the owner of this list */
SPluginBrowser& SPluginCategoryTree::GetOwner()
{
	return *OwnerWeak.Pin();
}


void SPluginCategoryTree::RebuildAndFilterCategoryTree()
{
	// Get a plugin from the currently selected category, so we can track it if it's removed
	TSharedPtr<IPlugin> TrackPlugin = nullptr;
	for(TSharedPtr<FPluginCategory> SelectedItem: PluginCategoryTreeView->GetSelectedItems())
	{
		if(SelectedItem->Plugins.Num() > 0)
		{
			TrackPlugin = SelectedItem->Plugins[0];
			break;
		}
	}

	// Clear the list of plugins in each current category
	for(TSharedPtr<FPluginCategory> RootCategory: RootPluginCategories)
	{
		for(TSharedPtr<FPluginCategory> Category: RootCategory->SubCategories)
		{
			Category->Plugins.Reset();
		}
	}

	// Add all the known plugins into categories
	TSharedPtr<FPluginCategory> SelectCategory;
	for(TSharedRef<IPlugin> Plugin: IPluginManager::Get().GetDiscoveredPlugins())
	{
		// Figure out which base category this plugin belongs in
		TSharedPtr<FPluginCategory> RootCategory;
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
		TSharedPtr<FPluginCategory> FoundCategory = NULL;
		for(TSharedPtr<FPluginCategory> TestCategory: RootCategory->SubCategories)
		{
			if(TestCategory->Name == CategoryName)
			{
				FoundCategory = TestCategory;
				break;
			}
		}

		if( !FoundCategory.IsValid() )
		{
			//@todo Allow for properly localized category names [3/7/2014 justin.sargent]
			FoundCategory = MakeShareable(new FPluginCategory(RootCategory, CategoryName, FText::FromString(CategoryName)));
			RootCategory->SubCategories.Add( FoundCategory );
		}
			
		// Associate the plugin with the category
		FoundCategory->Plugins.Add(Plugin);

		// Update the selection if this is the plugin we're tracking
		if(TrackPlugin == Plugin)
		{
			SelectCategory = FoundCategory;
		}
	}

	// Remove any empty categories, keeping track of which items are still selected
	for(TSharedPtr<FPluginCategory> RootCategory: RootPluginCategories)
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
	for(TSharedPtr<FPluginCategory> RootCategory: RootPluginCategories)
	{
		RootCategory->SubCategories.Sort([](const TSharedPtr<FPluginCategory>& A, const TSharedPtr<FPluginCategory>& B) -> bool { return A->DisplayName.CompareTo(B->DisplayName) < 0; });
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

TSharedRef<ITableRow> SPluginCategoryTree::PluginCategoryTreeView_OnGenerateRow( TSharedPtr<FPluginCategory> Item, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew( STableRow<TSharedPtr<FPluginCategory>>, OwnerTable )
		[
			SNew( SPluginCategory, Item.ToSharedRef() )
		];
}


void SPluginCategoryTree::PluginCategoryTreeView_OnGetChildren(TSharedPtr<FPluginCategory> Item, TArray<TSharedPtr<FPluginCategory>>& OutChildren )
{
	OutChildren.Append(Item->SubCategories);
}


void SPluginCategoryTree::PluginCategoryTreeView_OnSelectionChanged(TSharedPtr<FPluginCategory> Item, ESelectInfo::Type SelectInfo )
{
	// Selection changed, which may affect which plugins are displayed in the list.  We need to invalidate the list.
	OwnerWeak.Pin()->OnCategorySelectionChanged();
}


TSharedPtr<FPluginCategory> SPluginCategoryTree::GetSelectedCategory() const
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

void SPluginCategoryTree::SelectCategory( const TSharedPtr<FPluginCategory>& CategoryToSelect )
{
	if( ensure( PluginCategoryTreeView.IsValid() ) )
	{
		PluginCategoryTreeView->SetSelection( CategoryToSelect );
	}
}

bool SPluginCategoryTree::IsItemExpanded( const TSharedPtr<FPluginCategory> Item ) const
{
	return PluginCategoryTreeView->IsItemExpanded( Item );
}

void SPluginCategoryTree::SetNeedsRefresh()
{
	RegisterActiveTimer (0.f, FWidgetActiveTimerDelegate::CreateSP (this, &SPluginCategoryTree::TriggerCategoriesRefresh));
}

EActiveTimerReturnType SPluginCategoryTree::TriggerCategoriesRefresh(double InCurrentTime, float InDeltaTime)
{
	RebuildAndFilterCategoryTree();
	return EActiveTimerReturnType::Stop;
}

#undef LOCTEXT_NAMESPACE
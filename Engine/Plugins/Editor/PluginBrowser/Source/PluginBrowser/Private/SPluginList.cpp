// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginBrowserPrivatePCH.h"
#include "SPluginList.h"
#include "SPluginBrowser.h"
#include "SPluginListTile.h"
#include "IPluginManager.h"


#define LOCTEXT_NAMESPACE "PluginList"


void SPluginList::Construct( const FArguments& Args, const TSharedRef< SPluginBrowser > Owner )
{
	OwnerWeak = Owner;

	// Find out when the plugin text filter changes
	Owner->GetPluginTextFilter().OnChanged().AddSP( this, &SPluginList::OnPluginTextFilterChanged );

	bIsActiveTimerRegistered = false;
	RebuildAndFilterPluginList();

	// @todo plugedit: Have optional compact version with only plugin icon + name + version?  Only expand selected?

	PluginListView = 
		SNew( SListView<TSharedRef<IPlugin>> )

		.SelectionMode( ESelectionMode::None )		// No need to select plugins!

		.ListItemsSource( &PluginListItems )
		.OnGenerateRow( this, &SPluginList::PluginListView_OnGenerateRow )
		;

	ChildSlot.AttachWidget( PluginListView.ToSharedRef() );
}


SPluginList::~SPluginList()
{
	const TSharedPtr< SPluginBrowser > Owner( OwnerWeak.Pin() );
	if( Owner.IsValid() )
	{
		Owner->GetPluginTextFilter().OnChanged().RemoveAll( this );
	}
}


/** @return Gets the owner of this list */
SPluginBrowser& SPluginList::GetOwner()
{
	return *OwnerWeak.Pin();
}


TSharedRef<ITableRow> SPluginList::PluginListView_OnGenerateRow( TSharedRef<IPlugin> Item, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew( STableRow< TSharedRef<IPlugin> >, OwnerTable )
		[
			SNew( SPluginListTile, SharedThis( this ), Item )
		];
}



void SPluginList::RebuildAndFilterPluginList()
{
	// Build up the initial list of plugins
	{
		PluginListItems.Reset();

		// Get the currently selected category
		const TSharedPtr<FPluginCategory>& SelectedCategory = OwnerWeak.Pin()->GetSelectedCategory();
		if( SelectedCategory.IsValid() )
		{
			for(TSharedRef<IPlugin> Plugin: SelectedCategory->Plugins)
			{
				if(OwnerWeak.Pin()->GetPluginTextFilter().PassesFilter(&Plugin.Get()))
				{
					PluginListItems.Add(Plugin);
				}
			}
		}

		// Sort the plugins alphabetically
		{
			struct FPluginListItemSorter
			{
				bool operator()( const TSharedRef<IPlugin>& A, const TSharedRef<IPlugin>& B ) const
				{
					return A->GetDescriptor().FriendlyName < B->GetDescriptor().FriendlyName;
				}
			};
			PluginListItems.Sort( FPluginListItemSorter() );
		}
	}


	// Update the list widget
	if( PluginListView.IsValid() )
	{
		PluginListView->RequestListRefresh();
	}
}

EActiveTimerReturnType SPluginList::TriggerListRebuild(double InCurrentTime, float InDeltaTime)
{
	RebuildAndFilterPluginList();

	bIsActiveTimerRegistered = false;
	return EActiveTimerReturnType::Stop;
}

void SPluginList::OnPluginTextFilterChanged()
{
	SetNeedsRefresh();
}


void SPluginList::SetNeedsRefresh()
{
	if (!bIsActiveTimerRegistered)
	{
		bIsActiveTimerRegistered = true;
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SPluginList::TriggerListRebuild));
	}
}


#undef LOCTEXT_NAMESPACE
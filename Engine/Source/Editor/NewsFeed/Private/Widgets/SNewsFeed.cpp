// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SNewsFeed.cpp: Implements the SNewsFeed class.
=============================================================================*/

#include "NewsFeedPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SNewsFeed"


/* SNewsFeed structors
 *****************************************************************************/

SNewsFeed::~SNewsFeed( )
{
	GetMutableDefault<UNewsFeedSettings>()->OnSettingChanged().RemoveAll(this);

	NewsFeedCache->OnLoadCompleted().RemoveAll(this);
	NewsFeedCache->OnLoadFailed().RemoveAll(this);
}


/* SNewsFeed interface
 *****************************************************************************/

void SNewsFeed::Construct( const FArguments& InArgs, const FNewsFeedCacheRef& InNewsFeedCache )
{
	NewsFeedCache = InNewsFeedCache;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
					[
						// news list
						SAssignNew(NewsFeedItemListView, SListView<FNewsFeedItemPtr>)
							.ItemHeight(64.0f)
							.ListItemsSource(&NewsFeedItemList)
							.OnGenerateRow(this, &SNewsFeed::HandleNewsListViewGenerateRow)
							.OnSelectionChanged(this, &SNewsFeed::HandleNewsListViewSelectionChanged)
							.SelectionMode(ESelectionMode::Single)
							.Visibility(this, &SNewsFeed::HandleNewsListViewVisibility)
							.HeaderRow
							(
								SNew(SHeaderRow)
									.Visibility(EVisibility::Collapsed)

								+ SHeaderRow::Column("Icon")
									.FixedWidth(48.0f)

								+ SHeaderRow::Column("Content")
							)
					]

				+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.Padding(FMargin(16.0f, 4.0f))
					[
						// no news available
						SNew(STextBlock)
							.Text(this, &SNewsFeed::HandleNewsUnavailableText)
							.Visibility(this, &SNewsFeed::HandleNewsUnavailableVisibility)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSeparator)
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f)
					[
						SNew(SOverlay)

						+ SOverlay::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								// settings button
								SNew(SButton)
									.ButtonStyle(FEditorStyle::Get(), "NoBorder")
									.ContentPadding(0.0f)
									.OnClicked(this, &SNewsFeed::HandleRefreshButtonClicked)
									.ToolTipText(LOCTEXT("RefreshButtonToolTip", "Reload the latest news from the server."))
									.Visibility(this, &SNewsFeed::HandleRefreshButtonVisibility)
									[
										SNew(SImage)
											.Image(FEditorStyle::GetBrush("NewsFeed.ReloadButton"))
									]
							]

						+ SOverlay::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								// refresh throbber
								SNew(SCircularThrobber)
									.Radius(10.0f)
									.ToolTipText(LOCTEXT("RefreshThrobberToolTip", "Loading latest news..."))
									.Visibility(this, &SNewsFeed::HandleRefreshThrobberVisibility)
							]
					]

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Center)
					[
						// mark all as read
						SNew(SHyperlink)
							.IsEnabled(this, &SNewsFeed::HandleMarkAllAsReadIsEnabled)
							.OnNavigate(this, &SNewsFeed::HandleMarkAllAsReadNavigate)
							.Text(LOCTEXT("MarkAllAsReadHyperlink", "Mark all news as read"))
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f)
					[
						// settings button
						SNew(SButton)
							.ButtonStyle(FEditorStyle::Get(), "NoBorder")
							.ContentPadding(0.0f)
							.OnClicked(this, &SNewsFeed::HandleSettingsButtonClicked)
							.ToolTipText(LOCTEXT("SettingsButtonToolTip", "News feed settings."))	
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("NewsFeed.SettingsButton"))
							]	
					]
			]
	];

	NewsFeedCache->OnLoadCompleted().AddRaw(this, &SNewsFeed::HandleNewsFeedLoaderLoadCompleted);
	NewsFeedCache->OnLoadFailed().AddRaw(this, &SNewsFeed::HandleNewsFeedLoaderLoadFailed);
	NewsFeedCache->LoadFeed();

	GetMutableDefault<UNewsFeedSettings>()->OnSettingChanged().AddSP(this, &SNewsFeed::HandleNewsFeedSettingsChanged);
}


/* SNewsFeed implementation
 *****************************************************************************/

void SNewsFeed::ReloadNewsFeedItems( )
{
	NewsFeedItemList.Empty();

	const UNewsFeedSettings* NewsFeedSettings = GetDefault<UNewsFeedSettings>();

	for (auto NewsFeedItem : NewsFeedCache->GetItems())
	{
		if (!GetDefault<UNewsFeedSettings>()->ShowOnlyUnreadItems || !NewsFeedItem->Read)
		{
			NewsFeedItemList.Add(NewsFeedItem);

			if (NewsFeedItemList.Num() == NewsFeedSettings->MaxItemsToShow)
			{
				break;
			}
		}
	}

	NewsFeedItemListView->RequestListRefresh();
}


/* SNewsFeed callbacks
 *****************************************************************************/

EVisibility SNewsFeed::HandleLoadFailedBoxVisibility( ) const
{
	return NewsFeedCache->IsFinished() ? EVisibility::Collapsed : EVisibility::Visible;
}


bool SNewsFeed::HandleMarkAllAsReadIsEnabled( ) const
{
	for (auto NewsFeedItem : NewsFeedItemList)
	{
		if (!NewsFeedItem->Read)
		{
			return true;
		}
	}

	return false;
}


void SNewsFeed::HandleMarkAllAsReadNavigate( )
{
	for (auto NewsFeedItem : NewsFeedItemList)
	{
		NewsFeedItem->Read = true;
	}

	if (GetDefault<UNewsFeedSettings>()->ShowOnlyUnreadItems)
	{
		ReloadNewsFeedItems();
	}
}


void SNewsFeed::HandleNewsFeedLoaderLoadCompleted( )
{
	ReloadNewsFeedItems();
}


void SNewsFeed::HandleNewsFeedLoaderLoadFailed( )
{

}


void SNewsFeed::HandleNewsFeedSettingsChanged( FName PropertyName )
{
	ReloadNewsFeedItems();
}


TSharedRef<ITableRow> SNewsFeed::HandleNewsListViewGenerateRow( FNewsFeedItemPtr NewsFeedItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SNewsFeedListRow, OwnerTable, NewsFeedCache.ToSharedRef())
		.NewsFeedItem(NewsFeedItem);
}


void SNewsFeed::HandleNewsListViewSelectionChanged( FNewsFeedItemPtr Selection, ESelectInfo::Type SelectInfo )
{
	if (Selection.IsValid() && (SelectInfo == ESelectInfo::OnMouseClick))
	{
		TArray<FAnalyticsEventAttribute> EventAttributes;

		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("ItemID"), Selection->ItemId.ToString()));

		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

		if (DesktopPlatform != nullptr)
		{
			bool Succeeded = false;

			if (Selection->Url.StartsWith(TEXT("http://")) || Selection->Url.StartsWith(TEXT("https://")))
			{
				FPlatformProcess::LaunchURL(*Selection->Url, NULL, NULL);
				Succeeded = true;
			}
			else if (Selection->Url.StartsWith(TEXT("ue4://market/")))
			{
				Succeeded = DesktopPlatform->OpenLauncher(false, FString::Printf(TEXT("-OpenMarketItem=%s"), *Selection->Url.RightChop(13)));
			}
//			else if (Selection->Url.StartsWith(TEXT("ue4://")))
//			{
//				Succeeded = DesktopPlatform->OpenLauncher(false, FString::Printf(TEXT("-OpenUrl=%s"), *Selection->Url));
//			}

			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("OpenUrlSucceeded"), Succeeded ? TEXT("TRUE") : TEXT("FALSE")));

			if (FEngineAnalytics::IsAvailable())
			{
				FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.OpenNewsFeedItem"), EventAttributes);
			}
		}

		// hack: mark as read
		Selection->Read = true;
		
		NewsFeedItemListView->SetSelection(nullptr);

		if (GetDefault<UNewsFeedSettings>()->ShowOnlyUnreadItems)
		{
			ReloadNewsFeedItems();
		}
	}
}


FText SNewsFeed::HandleNewsUnavailableText( ) const
{
	if (GetDefault<UNewsFeedSettings>()->ShowOnlyUnreadItems)
	{
		return LOCTEXT("NoUnreadNewsAvailable", "There are currently no unread news");
	}

	return LOCTEXT("NoNewsAvailable", "There are currently no news");
}


EVisibility SNewsFeed::HandleNewsListViewVisibility( ) const
{
	return (NewsFeedItemList.Num() == 0) ? EVisibility::Collapsed : EVisibility::Visible;
}


EVisibility SNewsFeed::HandleNewsUnavailableVisibility( ) const
{
	return (NewsFeedItemList.Num() > 0) ? EVisibility::Collapsed : EVisibility::Visible;
}


FReply SNewsFeed::HandleRefreshButtonClicked( ) const
{
	NewsFeedCache->LoadFeed();

	return FReply::Handled();
}


EVisibility SNewsFeed::HandleRefreshButtonVisibility( ) const
{
	return NewsFeedCache->IsLoading() ? EVisibility::Hidden : EVisibility::Visible;
}


EVisibility SNewsFeed::HandleRefreshThrobberVisibility( ) const
{
	return NewsFeedCache->IsLoading() ? EVisibility::Visible : EVisibility::Hidden;
}


FReply SNewsFeed::HandleSettingsButtonClicked( ) const
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "LevelEditor", "NewsFeed");

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

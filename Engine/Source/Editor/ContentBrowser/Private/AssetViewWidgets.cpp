// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "ContentBrowserPCH.h"
#include "ISourceControlModule.h"
#include "AssetThumbnail.h"
#include "AssetViewTypes.h"
#include "AssetViewWidgets.h"
#include "SThumbnailEditModeTools.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "DragAndDrop/AssetPathDragDropOp.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"


///////////////////////////////
// FAssetViewModeUtils
///////////////////////////////

FReply FAssetViewModeUtils::OnViewModeKeyDown( const TSet< TSharedPtr<FAssetViewItem> >& SelectedItems, const FKeyboardEvent& InKeyboardEvent )
{
	// All asset views use Ctrl-C to copy references to assets
	if ( InKeyboardEvent.IsControlDown() && InKeyboardEvent.GetCharacter() == 'C' )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		TArray<FAssetData> SelectedAssets;
		for ( auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt)
		{
			const TSharedPtr<FAssetViewItem>& Item = *ItemIt;
			if(Item.IsValid())
			{
				if(Item->GetType() == EAssetItemType::Folder)
				{
					// we need to recurse & copy references to all folder contents
					FARFilter Filter;
					Filter.PackagePaths.Add(*StaticCastSharedPtr<FAssetViewFolder>(Item)->FolderPath);

					// Add assets found in the asset registry
					AssetRegistryModule.Get().GetAssets(Filter, SelectedAssets);
				}
				else
				{
					SelectedAssets.Add(StaticCastSharedPtr<FAssetViewAsset>(Item)->Data);
				}
			}
		}

		ContentBrowserUtils::CopyAssetReferencesToClipboard(SelectedAssets);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}


///////////////////////////////
// Asset view modes
///////////////////////////////

FReply SAssetTileView::OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FReply Reply = FAssetViewModeUtils::OnViewModeKeyDown(SelectedItems, InKeyboardEvent);

	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}
	else
	{
		return STileView<TSharedPtr<FAssetViewItem>>::OnKeyDown(InGeometry, InKeyboardEvent);
	}
}

FReply SAssetListView::OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FReply Reply = FAssetViewModeUtils::OnViewModeKeyDown(SelectedItems, InKeyboardEvent);

	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}
	else
	{
		return SListView<TSharedPtr<FAssetViewItem>>::OnKeyDown(InGeometry, InKeyboardEvent);
	}
}

FReply SAssetColumnView::OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FReply Reply = FAssetViewModeUtils::OnViewModeKeyDown(SelectedItems, InKeyboardEvent);

	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}
	else
	{
		return SListView<TSharedPtr<FAssetViewItem>>::OnKeyDown(InGeometry, InKeyboardEvent);
	}
}


///////////////////////////////
// SAssetViewItem
///////////////////////////////

SAssetViewItem::~SAssetViewItem()
{
	if ( AssetItem.IsValid() )
	{
		AssetItem->OnAssetDataChanged.RemoveAll( this );
	}

	OnItemDestroyed.ExecuteIfBound( AssetItem );
}

void SAssetViewItem::Construct( const FArguments& InArgs )
{
	AssetItem = InArgs._AssetItem;
	OnRenameBegin = InArgs._OnRenameBegin;
	OnRenameCommit = InArgs._OnRenameCommit;
	OnVerifyRenameCommit = InArgs._OnVerifyRenameCommit;
	OnItemDestroyed = InArgs._OnItemDestroyed;
	ShouldAllowToolTip = InArgs._ShouldAllowToolTip;
	ThumbnailEditMode = InArgs._ThumbnailEditMode;
	LabelVisibility = InArgs._LabelVisibility;
	ConstructToolTipForAsset = InArgs._ConstructToolTip;
	OnAssetsDragDropped = InArgs._OnAssetsDragDropped;
	OnPathsDragDropped = InArgs._OnPathsDragDropped;
	OnFilesDragDropped = InArgs._OnFilesDragDropped;

	bDraggedOver = false;

	CachePackageName();
	AssetPackage = FindObjectSafe<UPackage>(NULL, *CachedPackageName);
	bPackageDirty = false;
	UpdatePackageDirtyState();

	AssetItem->OnAssetDataChanged.AddSP(this, &SAssetViewItem::OnAssetDataChanged);

	TMap<FName, FString> ImportantStaticMeshTags;
	ImportantStaticMeshTags.Add("CollisionPrims", TEXT("0"));
	ImportantTagMap.Add("StaticMesh", ImportantStaticMeshTags);

	AssetDirtyBrush = FEditorStyle::GetBrush("ContentBrowser.ContentDirty");

	SourceControlStateDelay = 0.0f;
	bSourceControlStateRequested = false;
}

void SAssetViewItem::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	LastGeometry = AllottedGeometry;

	UpdatePackageDirtyState();

	UpdateSourceControlState((float)InDeltaTime);
}

TSharedPtr<SToolTip> SAssetViewItem::GetToolTip()
{
	return ShouldAllowToolTip.Get() ? SCompoundWidget::GetToolTip() : NULL;
}

void SAssetViewItem::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bDraggedOver = false;

	if(IsFolder())
	{
		if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
		{
			TArray< FAssetData > AssetDatas = AssetUtil::ExtractAssetDataFromDrag( DragDropEvent );

			if ( AssetDatas.Num() > 0 )
			{
				TSharedPtr< FAssetDragDropOp > DragDropOp = StaticCastSharedPtr< FAssetDragDropOp >( DragDropEvent.GetOperation() );	
				DragDropOp->SetTooltip( LOCTEXT( "OnDragAssetsOverFolder", "Move or Copy Asset(s)" ).ToString(), FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")) );
			}
			bDraggedOver = true;
		}
		else if( DragDrop::IsTypeMatch<FAssetPathDragDropOp>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr<FAssetPathDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetPathDragDropOp>( DragDropEvent.GetOperation() );
			bool bCanDrop = DragDropOp->PathNames.Num() > 0;
			if ( DragDropOp->PathNames.Contains(StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath) )
			{
				// You can't drop a folder onto itself
				bCanDrop = false;
			}

			if(bCanDrop)
			{
				bDraggedOver = true;
			}
		}
	}
}
	
void SAssetViewItem::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if(IsFolder())
	{
		if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr< FAssetDragDropOp > DragDropOp = StaticCastSharedPtr< FAssetDragDropOp >( DragDropEvent.GetOperation() );	
			DragDropOp->ClearTooltip();
		}
	}

	bDraggedOver = false;
}

FReply SAssetViewItem::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if(IsFolder())
	{
		if ( DragDrop::IsTypeMatch<FExternalDragOperation>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>( DragDropEvent.GetOperation() );
			if ( DragDropOp->HasFiles() )
			{
				bDraggedOver = true;
				return FReply::Handled();
			}
		}
		else if( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
		{
			bDraggedOver = true;
			return FReply::Handled();
		}
		else if( DragDrop::IsTypeMatch<FAssetPathDragDropOp>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr<FAssetPathDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetPathDragDropOp>( DragDropEvent.GetOperation() );
			bool bCanDrop = DragDropOp->PathNames.Num() > 0;
			if ( DragDropOp->PathNames.Contains(StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath) )
			{
				// You can't drop a folder onto itself
				bCanDrop = false;
			}

			if(bCanDrop)
			{
				bDraggedOver = true;
			}
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SAssetViewItem::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bDraggedOver = false;

	if(IsFolder())
	{
		check(AssetItem->GetType() == EAssetItemType::Folder);

		if ( DragDrop::IsTypeMatch<FExternalDragOperation>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>( DragDropEvent.GetOperation() );

			if ( DragDropOp->HasFiles() )
			{
				OnFilesDragDropped.ExecuteIfBound(DragDropOp->GetFiles(), StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath);
			}

			return FReply::Handled();
		}
		else if ( DragDrop::IsTypeMatch<FAssetPathDragDropOp>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr<FAssetPathDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetPathDragDropOp>( DragDropEvent.GetOperation() );

			bool bCanDrop = DragDropOp->PathNames.Num() > 0;

			if ( DragDropOp->PathNames.Contains(StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath) )
			{
				// You can't drop a folder onto itself
				bCanDrop = false;
			}

			if ( bCanDrop )
			{
				OnPathsDragDropped.ExecuteIfBound(DragDropOp->PathNames, StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath);
			}

			return FReply::Handled();
		}
		else if ( DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()) )
		{
			TSharedPtr<FAssetDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>( DragDropEvent.GetOperation() );

			OnAssetsDragDropped.ExecuteIfBound(DragDropOp->AssetData, StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath);

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SAssetViewItem::HandleBeginNameChange( const FText& OriginalText )
{
	OnRenameBegin.ExecuteIfBound(AssetItem, OriginalText.ToString(), LastGeometry.GetClippingRect());
}

void SAssetViewItem::HandleNameCommitted( const FText& NewText, ETextCommit::Type CommitInfo )
{
	OnRenameCommit.ExecuteIfBound(AssetItem, NewText.ToString(), LastGeometry.GetClippingRect(), CommitInfo);
}

bool  SAssetViewItem::HandleVerifyNameChanged( const FText& NewText, FText& OutErrorMessage )
{
	return !OnVerifyRenameCommit.IsBound() || OnVerifyRenameCommit.Execute(AssetItem, NewText, LastGeometry.GetClippingRect(), OutErrorMessage);
}

void SAssetViewItem::OnAssetDataChanged()
{
	CachePackageName();
	AssetPackage = FindObjectSafe<UPackage>(NULL, *CachedPackageName);
	UpdatePackageDirtyState();

	if ( InlineRenameWidget.IsValid() )
	{
		InlineRenameWidget->SetText( GetNameText() );
	}
}

void SAssetViewItem::DirtyStateChanged()
{
}

const FSlateBrush* SAssetViewItem::GetSCCStateImage() const
{

	if ( ISourceControlModule::Get().IsEnabled() && AssetItem.IsValid() && (AssetItem->GetType() == EAssetItemType::Normal) && !AssetItem->IsTemporaryItem() && !CachedPackageFileName.IsEmpty() )
	{
		FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(CachedPackageFileName, EStateCacheUsage::Use);
		if(SourceControlState.IsValid())
		{
			return FEditorStyle::GetBrush(SourceControlState->GetIconName());
		}
	}

	return NULL;
}

const FSlateBrush* SAssetViewItem::GetDirtyImage() const
{
	return IsDirty() ? AssetDirtyBrush : NULL;
}

EVisibility SAssetViewItem::GetThumbnailEditModeUIVisibility() const
{
	return !IsFolder() && ThumbnailEditMode.Get() == true ? EVisibility::Visible : EVisibility::Collapsed;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SToolTip> SAssetViewItem::CreateToolTipWidget() const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			const FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			UClass* AssetClass = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());

			if ( ConstructToolTipForAsset.IsBound() )
			{
				return ConstructToolTipForAsset.Execute( AssetData );
			}

			// The tooltip contains the name, class, path, and asset registry tags
			const FText NameText = FText::FromName( AssetData.AssetName );
			const FText ClassText = FText::Format( LOCTEXT("ClassName", "({0})"), FText::FromName( AssetData.AssetClass ) );


			// Create a box to hold every line of info in the body of the tooltip
			TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

			// Add Path
			AddToToolTipInfoBox( InfoBox, LOCTEXT("TileViewTooltipPath", "Path"), FText::FromName(AssetData.PackagePath), false );

			// If we are using a loaded class, find all the hidden tags so we don't display them
			TSet<FName> ShownTags;
			if ( AssetClass != NULL && AssetClass->GetDefaultObject() != NULL )
			{
				TArray<UObject::FAssetRegistryTag> Tags;
				AssetClass->GetDefaultObject()->GetAssetRegistryTags(Tags);

				for ( auto TagIt = Tags.CreateConstIterator(); TagIt; ++TagIt )
				{
					if ( (*TagIt).Type != UObject::FAssetRegistryTag::TT_Hidden )
					{
						ShownTags.Add((*TagIt).Name);
					}
				}
			}

			// Get the list of important tags for this class
			TMap<FName, FString> ImportantTags = ImportantTagMap.FindRef(AssetData.AssetClass);

			// If an asset class could not be loaded we cannot determine hidden tags so display no tags.  
			if( AssetClass != NULL )
			{
				// Add all asset registry tags and values
				for ( auto TagIt = AssetData.TagsAndValues.CreateConstIterator(); TagIt; ++TagIt )
				{
					// Skip tags that are set to be hidden
					if ( ShownTags.Contains(TagIt.Key()) )
					{
						const FString* ImportantValue = ImportantTags.Find(TagIt.Key());
						const bool bImportant = (ImportantValue && (*ImportantValue) == TagIt.Value());

						AddToToolTipInfoBox( InfoBox, FText::FromName(TagIt.Key()), FText::FromString(TagIt.Value()), bImportant );
					}
				}
			}

			TSharedRef<SVerticalBox> OverallTooltipVBox = SNew(SVerticalBox);

			// Top section (asset name, type, is checked out)
			OverallTooltipVBox->AddSlot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(SBorder)
					.Padding(6)
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0, 0, 4, 0)
							[
								SNew(STextBlock)
								.Text(NameText)
								.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(ClassText)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Visibility(this, &SAssetViewItem::GetCheckedOutByOtherTextVisibility)
							.Text(this, &SAssetViewItem::GetCheckedOutByOtherText)
							.ColorAndOpacity(FLinearColor(0.1f, 0.5f, 1.f, 1.f))
						]
					]
				];

			// Middle section (user description, if present)
			FText UserDescription = GetAssetUserDescription();
			if (!UserDescription.IsEmpty())
			{
				OverallTooltipVBox->AddSlot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(SBorder)
						.Padding(6)
						.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
						[
							SNew(STextBlock)
							.WrapTextAt(300.0f)
							.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.AssetUserDescriptionFont"))
							.Text(UserDescription)
						]
					];
			}

			// Bottom section (asset registry tags)
			OverallTooltipVBox->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.Padding(6)
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						InfoBox
					]
				];


			return SNew(SToolTip)
				.TextMargin(1)
				.BorderImage( FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder") )
				[
					SNew(SBorder)
					.Padding(6)
					.BorderImage( FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder") )
					[
						OverallTooltipVBox
					]
				];
		}
		else
		{
			const FText& FolderName = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderName;
			const FString& FolderPath = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath;

			// Create a box to hold every line of info in the body of the tooltip
			TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

			AddToToolTipInfoBox( InfoBox, LOCTEXT("TileViewTooltipPath", "Path"), FText::FromString(FolderPath), false );

			return SNew(SToolTip)
				.TextMargin(1)
				.BorderImage( FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder") )
				[
					SNew(SBorder)
					.Padding(6)
					.BorderImage( FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder") )
					[
						SNew(SVerticalBox)

						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 4)
						[
							SNew(SBorder)
							.Padding(6)
							.BorderImage( FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder") )
							[
								SNew(SVerticalBox)

								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)

									+SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(0, 0, 4, 0)
									[
										SNew(STextBlock)
										.Text( FolderName )
										.Font( FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont") )
									]

									+SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock) 
										.Text( LOCTEXT("FolderNameBracketed", "(Folder)") )
									]
								]
							]
						]

						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(6)
							.BorderImage( FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder") )
							[
								InfoBox
							]
						]
					]
				];
		}
	}
	else
	{
		// Return an empty tooltip since the asset item wasn't valid
		return SNew(SToolTip);
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

EVisibility SAssetViewItem::GetCheckedOutByOtherTextVisibility() const
{
	return GetCheckedOutByOtherText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

FString SAssetViewItem::GetCheckedOutByOtherText() const
{
	if(AssetItem.IsValid() && AssetItem->GetType() != EAssetItemType::Folder)
	{
		const FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(AssetData.PackageName.ToString()), EStateCacheUsage::Use);
		FString UserWhichHasPackageCheckedOut;
		if (SourceControlState.IsValid() && SourceControlState->IsCheckedOutOther(&UserWhichHasPackageCheckedOut) )
		{
			if ( !UserWhichHasPackageCheckedOut.IsEmpty() )
			{
				return SourceControlState->GetDisplayTooltip().ToString();
			}
		}
	}

	return TEXT("");
}


FText SAssetViewItem::GetAssetUserDescription() const
{
	if (AssetItem.IsValid())
	{
		if (AssetItem->GetType() != EAssetItemType::Folder)
		{
			const FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			FText AssetClass = FText::FromName(AssetData.AssetClass);

			if (UClass* AssetClassObj = FindObject<UClass>(ANY_PACKAGE, *AssetClass.ToString()))
			{
				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
				TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetClassObj);

				if (AssetTypeActions.IsValid())
				{
					FText Description = AssetTypeActions.Pin()->GetAssetDescription(AssetData);
					return Description;
				}
			}
		}
	}

	return FText::GetEmpty();
}


void SAssetViewItem::AddToToolTipInfoBox(const TSharedRef<SVerticalBox>& InfoBox, const FText& Key, const FText& Value, bool bImportant) const
{
	const FLinearColor ImportantColor = FLinearColor(1, 0.5, 0, 1);
	InfoBox->AddSlot()
	.AutoHeight()
	.Padding(0, 1)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 4, 0)
		[
			SNew(STextBlock) .Text( FText::Format(LOCTEXT("AssetViewTooltipFormat", "{0}:"), Key ) )
			.ColorAndOpacity(bImportant ? ImportantColor : FSlateColor::UseForeground())
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock) .Text( Value )
			.ColorAndOpacity(bImportant ? ImportantColor : FSlateColor::UseForeground())
		]
	];
}

void SAssetViewItem::UpdatePackageDirtyState()
{
	bool bNewIsDirty = false;
	if ( AssetPackage.IsValid() )
	{
		bNewIsDirty = AssetPackage->IsDirty();
	}

	if ( bNewIsDirty != bPackageDirty )
	{
		bPackageDirty = bNewIsDirty;
		DirtyStateChanged();
	}
}

bool SAssetViewItem::IsDirty() const
{
	return bPackageDirty;
}

void SAssetViewItem::UpdateSourceControlState(float InDeltaTime)
{
	SourceControlStateDelay += InDeltaTime;

	if ( !bSourceControlStateRequested && SourceControlStateDelay > 1.0f && AssetItem.IsValid() )
	{
		if ( AssetItem.IsValid() && AssetItem->GetType() != EAssetItemType::Folder )
		{
			if ( !AssetItem->IsTemporaryItem() )
			{
				// dont query status for built-in types
				if(!FPackageName::IsScriptPackage(CachedPackageName))
				{
				// Request the most recent SCC state for this asset
				ISourceControlModule::Get().QueueStatusUpdate(CachedPackageFileName);
			}
		}
		}

		bSourceControlStateRequested = true;
	}
}

void SAssetViewItem::CachePackageName()
{
	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			CachedPackageName = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.PackageName.ToString();
			CachedPackageFileName = SourceControlHelpers::PackageFilename(CachedPackageName);
		}
		else
		{
			CachedPackageName = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderName.ToString();
		}
	}
}

const FSlateBrush* SAssetViewItem::GetBorderImage() const
{
	return bDraggedOver ? FEditorStyle::GetBrush("Menu.Background") : FEditorStyle::GetBrush("NoBorder");
}

bool SAssetViewItem::IsFolder() const
{
	return AssetItem.IsValid() && AssetItem->GetType() == EAssetItemType::Folder;
}

FText SAssetViewItem::GetNameText() const
{
	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			return FText::FromName(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.AssetName);
		}
		else
		{
			return StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderName;
		}
	}

	return FText();
}

FSlateColor SAssetViewItem::GetAssetColor() const
{
	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() == EAssetItemType::Folder)
		{
			const TSharedPtr<FLinearColor> Color = ContentBrowserUtils::LoadColor( StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath );
			if ( Color.IsValid() )
			{
				return *Color.Get();
			}
		}
	}
	return ContentBrowserUtils::GetDefaultColor();
}

///////////////////////////////
// SAssetListItem
///////////////////////////////

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAssetListItem::Construct( const FArguments& InArgs )
{
	SAssetViewItem::Construct( SAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ShouldAllowToolTip(InArgs._ShouldAllowToolTip)
		.ThumbnailEditMode(InArgs._ThumbnailEditMode)
		.ConstructToolTip(InArgs._ConstructToolTip)
		.OnAssetsDragDropped(InArgs._OnAssetsDragDropped)
		.OnPathsDragDropped(InArgs._OnPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		);

	AssetThumbnail = InArgs._AssetThumbnail;
	ItemHeight = InArgs._ItemHeight;

	const float ThumbnailPadding = InArgs._ThumbnailPadding;

	FText AssetClass;
	bool bIsDeveloperFolder = false;

	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			const FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			AssetClass = FText::FromName(AssetData.AssetClass);
		}
		else
		{
			AssetClass = LOCTEXT("FolderName", "Folder");
			bIsDeveloperFolder = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->bDeveloperFolder;
		}
	}

	TSharedPtr<SWidget> Thumbnail;
	if ( AssetItem.IsValid() && AssetThumbnail.IsValid() )
	{
		const bool bForceGenericThumbnail = (AssetItem->GetType() == EAssetItemType::Creation);
		const bool bAllowFadeIn = true;
		Thumbnail = AssetThumbnail->MakeThumbnailWidget(bAllowFadeIn, bForceGenericThumbnail, InArgs._ThumbnailLabel, InArgs._HighlightText, InArgs._ThumbnailHintColorAndOpacity, InArgs._AllowThumbnailHintLabel );
	}
	else
	{
		Thumbnail = SNew(SImage) .Image( FEditorStyle::GetDefaultBrush() );
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(this, &SAssetViewItem::GetBorderImage)
		.Padding(0)
		[
			SNew(SHorizontalBox)

			// Viewport
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew( SBox )
				.WidthOverride( this, &SAssetListItem::GetThumbnailBoxSize )
				.HeightOverride( this, &SAssetListItem::GetThumbnailBoxSize )
				[
					// Drop shadow border
					SNew(SBorder)
					.Padding(ThumbnailPadding)
					.BorderImage( IsFolder() ? FEditorStyle::GetBrush("NoBorder") : FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow") )
					[
						SNew(SOverlay)
				
						// Folder base
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Visibility(IsFolder() ? EVisibility::Visible : EVisibility::Collapsed)
							.Image( bIsDeveloperFolder ? FEditorStyle::GetBrush("ContentBrowser.ListViewDeveloperFolderIcon.Base") : FEditorStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Base") )
							.ColorAndOpacity(this, &SAssetViewItem::GetAssetColor)
						]

						// Folder tint
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Visibility(IsFolder() ? EVisibility::Visible : EVisibility::Collapsed)
							.Image( bIsDeveloperFolder ? FEditorStyle::GetBrush("ContentBrowser.ListViewDeveloperFolderIcon.Mask") : FEditorStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Mask") )
						]

						// The actual thumbnail
						+SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							.Visibility(IsFolder() ? EVisibility::Collapsed : EVisibility::Visible)
							+SHorizontalBox::Slot()
							[
								Thumbnail.ToSharedRef()
							]
						]

						+SOverlay::Slot()
						[
							SNew(SThumbnailEditModeTools, AssetThumbnail)
							.SmallView(true)
							.Visibility(this, &SAssetListItem::GetThumbnailEditModeUIVisibility)
						]

						// Source control state
						+SOverlay::Slot()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Top)
						[
							SNew( SBox )
							.WidthOverride( this, &SAssetListItem::GetSCCImageSize )
							.HeightOverride( this, &SAssetListItem::GetSCCImageSize )
							[
								SNew(SImage).Image( this, &SAssetListItem::GetSCCStateImage )
							]
						]

						// Dirty state
						+SOverlay::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Bottom)
						[
							SNew(SImage)
							.Image( this, &SAssetListItem::GetDirtyImage )
						]
					]
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 1)
				[
					SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
					.Font(FEditorStyle::GetFontStyle("ContentBrowser.AssetTileViewNameFont"))
					.Text( GetNameText() )
					.OnBeginTextEdit(this, &SAssetListItem::HandleBeginNameChange)
					.OnTextCommitted(this, &SAssetListItem::HandleNameCommitted)
					.OnVerifyTextChanged(this, &SAssetListItem::HandleVerifyNameChanged)
					.HighlightText(InArgs._HighlightText)
					.IsSelected(InArgs._IsSelected)
					.IsReadOnly(ThumbnailEditMode)
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 1)
				[
					// Class
					SNew(STextBlock)
					.Font(FEditorStyle::GetFontStyle("ContentBrowser.AssetListViewClassFont"))
					.Text(AssetClass)
				]
			]
		]
	];

	SetToolTip( CreateToolTipWidget() );

	if(AssetItem.IsValid())
	{
		AssetItem->RenamedRequestEvent.BindSP( InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode );
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetListItem::OnAssetDataChanged()
{
	SAssetViewItem::OnAssetDataChanged();

	SetToolTip( CreateToolTipWidget() );
}

FOptionalSize SAssetListItem::GetThumbnailBoxSize() const
{
	return FOptionalSize( ItemHeight.Get() );
}

FOptionalSize SAssetListItem::GetSCCImageSize() const
{
	return GetThumbnailBoxSize().Get() * 0.3;
}


///////////////////////////////
// SAssetTileItem
///////////////////////////////

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAssetTileItem::Construct( const FArguments& InArgs )
{
	SAssetViewItem::Construct( SAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ShouldAllowToolTip(InArgs._ShouldAllowToolTip)
		.ThumbnailEditMode(InArgs._ThumbnailEditMode)
		.LabelVisibility(InArgs._LabelVisibility)
		.ConstructToolTip(InArgs._ConstructToolTip)
		.OnAssetsDragDropped(InArgs._OnAssetsDragDropped)
		.OnPathsDragDropped(InArgs._OnPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		);

	AssetThumbnail = InArgs._AssetThumbnail;
	ItemWidth = InArgs._ItemWidth;
	ThumbnailPadding = IsFolder() ? InArgs._ThumbnailPadding + 5.0f : InArgs._ThumbnailPadding;

	TSharedPtr<SWidget> Thumbnail;
	if ( AssetItem.IsValid() && AssetThumbnail.IsValid() )
	{
		const bool bForceGenericThumbnail = (AssetItem->GetType() == EAssetItemType::Creation);
		const bool bAllowFadeIn = true;
		Thumbnail = AssetThumbnail->MakeThumbnailWidget(bAllowFadeIn, bForceGenericThumbnail, InArgs._ThumbnailLabel, InArgs._HighlightText, InArgs._ThumbnailHintColorAndOpacity, InArgs._AllowThumbnailHintLabel);
	}
	else
	{
		Thumbnail = SNew(SImage) .Image( FEditorStyle::GetDefaultBrush() );
	}

	bool bIsDeveloperFolder = false;

	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() == EAssetItemType::Folder)
		{
			bIsDeveloperFolder = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->bDeveloperFolder;
		}
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(this, &SAssetViewItem::GetBorderImage)
		.Padding(0)
		[
			SNew(SVerticalBox)

			// Thumbnail
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				// Use ItemWidth for both the width and height to make it square.
				// The remainder of the space is reserved for the name.
				SNew(SBox)
				.WidthOverride( this, &SAssetTileItem::GetThumbnailBoxSize )
				.HeightOverride( this, &SAssetTileItem::GetThumbnailBoxSize )
				[
					// Drop shadow border
					SNew(SBorder)
					.Padding(ThumbnailPadding)
					.BorderImage(IsFolder() ? FEditorStyle::GetBrush("NoBorder") : FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
					[
						SNew(SOverlay)

						// Folder base
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Visibility(IsFolder() ? EVisibility::Visible : EVisibility::Collapsed)
							.Image( bIsDeveloperFolder ? FEditorStyle::GetBrush("ContentBrowser.TileViewDeveloperFolderIcon.Base") : FEditorStyle::GetBrush("ContentBrowser.TileViewFolderIcon.Base") )
							.ColorAndOpacity(this, &SAssetViewItem::GetAssetColor)
						]

						// Folder tint
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Visibility(IsFolder() ? EVisibility::Visible : EVisibility::Collapsed)
							.Image( bIsDeveloperFolder ? FEditorStyle::GetBrush("ContentBrowser.TileViewDeveloperFolderIcon.Mask") : FEditorStyle::GetBrush("ContentBrowser.TileViewFolderIcon.Mask") )
						]

						// The actual thumbnail
						+SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							.Visibility(IsFolder() ? EVisibility::Collapsed : EVisibility::Visible)
							+SHorizontalBox::Slot()
							[
								Thumbnail.ToSharedRef()
							]
						]

						// Tools for thumbnail edit mode
						+SOverlay::Slot()
						[
							SNew(SThumbnailEditModeTools, AssetThumbnail)
							.Visibility(this, &SAssetTileItem::GetThumbnailEditModeUIVisibility)
						]

						// Source control state
						+SOverlay::Slot()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Top)
						.Padding(FMargin(0, 2, 2, 0))
						[
							SNew( SBox )
							.WidthOverride( this, &SAssetTileItem::GetSCCImageSize )
							.HeightOverride( this, &SAssetTileItem::GetSCCImageSize )
							[
								SNew(SImage)
								.Image( this, &SAssetTileItem::GetSCCStateImage )
							]
						]

						// Dirty state
						+SOverlay::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Bottom)
						.Padding(FMargin(2, 0, 0, 2))
						[
							SNew(SImage)
							.Image( this, &SAssetTileItem::GetDirtyImage )
						]
					]
				]
			]

			+SVerticalBox::Slot()
			.Padding(FMargin(ThumbnailPadding, 0, ThumbnailPadding, ThumbnailPadding))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			[
				SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
					.Font(FEditorStyle::GetFontStyle("ContentBrowser.AssetTileViewNameFont"))
					.Text( GetNameText() )
					.OnBeginTextEdit(this, &SAssetTileItem::HandleBeginNameChange)
					.OnTextCommitted(this, &SAssetTileItem::HandleNameCommitted)
					.OnVerifyTextChanged(this, &SAssetTileItem::HandleVerifyNameChanged)
					.HighlightText(InArgs._HighlightText)
					.IsSelected(InArgs._IsSelected)
					.IsReadOnly(ThumbnailEditMode)
					.WrapTextAt(this, &SAssetTileItem::GetNameTextWrapWidth)
			]
		]
	
	];

	SetToolTip( CreateToolTipWidget() );

	if(AssetItem.IsValid())
	{
		AssetItem->RenamedRequestEvent.BindSP( InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode );
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetTileItem::OnAssetDataChanged()
{
	SAssetViewItem::OnAssetDataChanged();

	SetToolTip( CreateToolTipWidget() );
}

FOptionalSize SAssetTileItem::GetThumbnailBoxSize() const
{
	return FOptionalSize( ItemWidth.Get() );
}

FOptionalSize SAssetTileItem::GetSCCImageSize() const
{
	return GetThumbnailBoxSize().Get() * 0.2;
}

float SAssetTileItem::GetNameTextWrapWidth() const
{
	return ItemWidth.Get() - ThumbnailPadding * 2.f;
}

///////////////////////////////
// SAssetColumnItem
///////////////////////////////

void SAssetColumnItem::Construct( const FArguments& InArgs )
{
	SAssetViewItem::Construct( SAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ConstructToolTip(InArgs._ConstructToolTip )
		.OnAssetsDragDropped(InArgs._OnAssetsDragDropped)
		.OnPathsDragDropped(InArgs._OnPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		);
	
	HighlightText = InArgs._HighlightText;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SWidget> SAssetColumnItem::GenerateWidgetForColumn( const FName& ColumnName, FIsSelected InIsSelected )
{
	TSharedPtr<SWidget> Content;

	if ( ColumnName == "Name" )
	{
		const FSlateBrush* IconBrush;
		if(IsFolder())
		{
			if(AssetItem.IsValid() && StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->bDeveloperFolder)
			{
				IconBrush = FEditorStyle::GetBrush("ContentBrowser.ColumnViewDeveloperFolderIcon");
			}
			else
			{
				IconBrush = FEditorStyle::GetBrush("ContentBrowser.ColumnViewFolderIcon");
			}
		}
		else
		{
			IconBrush = FEditorStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon");;
		}

		// Make icon overlays (eg, SCC and dirty status) a reasonable size in relation to the icon size (note: it is assumed this icon is square)
		const float IconOverlaySize = IconBrush->ImageSize.X * 0.6f;

		Content = SNew(SHorizontalBox)
			.ToolTipText(this, &SAssetColumnItem::GetAssetNameToolTipText)

			// Icon
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SOverlay)
				
				// The actual icon
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Image( IconBrush )
					.ColorAndOpacity(this, &SAssetColumnItem::GetAssetColor)					
				]

				// Source control state
				+SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.WidthOverride(IconOverlaySize)
					.HeightOverride(IconOverlaySize)
					[
						SNew(SImage)
						.Image(this, &SAssetColumnItem::GetSCCStateImage)
					]
				]

				// Dirty state
				+SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBox)
					.WidthOverride(IconOverlaySize)
					.HeightOverride(IconOverlaySize)
					[
						SNew(SImage)
						.Image(this, &SAssetColumnItem::GetDirtyImage)
					]
				]
			]

			// Editable Name
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
				.Text( GetNameText() )
				.OnBeginTextEdit(this, &SAssetColumnItem::HandleBeginNameChange)
				.OnTextCommitted(this, &SAssetColumnItem::HandleNameCommitted)
				.OnVerifyTextChanged(this, &SAssetColumnItem::HandleVerifyNameChanged)
				.HighlightText(HighlightText)
				.IsSelected(InIsSelected)
				.IsReadOnly(ThumbnailEditMode)
			];

		if(AssetItem.IsValid())
		{
			AssetItem->RenamedRequestEvent.BindSP( InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode );
		}
	}
	else if ( ColumnName == "Class" )
	{
		Content = SAssignNew(ClassText, STextBlock)
			.ToolTipText( this, &SAssetColumnItem::GetAssetClassText )
			.Text( GetAssetClassText() );
	}
	else if ( ColumnName == "Path" )
	{
		Content = SAssignNew(PathText, STextBlock)
			.ToolTipText( this, &SAssetColumnItem::GetAssetPathText )
			.Text( GetAssetPathText() );
	}
	else
	{
		Content = SNew(STextBlock)
			.ToolTipText( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP(this, &SAssetColumnItem::GetAssetTagText, ColumnName) ) )
			.Text( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP(this, &SAssetColumnItem::GetAssetTagText, ColumnName) ) );
	}

	return SNew(SBox)
		.Padding(FMargin(0, 0, 6, 0)) // Add a little right padding so text from this column does not run directly into text from the next.
		[
			Content.ToSharedRef()
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetColumnItem::OnAssetDataChanged()
{
	SAssetViewItem::OnAssetDataChanged();

	if ( ClassText.IsValid() )
	{
		ClassText->SetText( GetAssetClassText() );
	}

	if ( PathText.IsValid() )
	{
		PathText->SetText( GetAssetPathText() );
	}
}

FSlateColor SAssetColumnItem::GetAssetColor() const
{
	FSlateColor AssetTypeColor = SAssetViewItem::GetAssetColor();

	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			const FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			FText AssetClass = FText::FromName(AssetData.AssetClass);

			if (UClass* AssetClassObj = FindObject<UClass>(ANY_PACKAGE, *AssetClass.ToString()))
			{
				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
				TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetClassObj);

				if (AssetTypeActions.IsValid())
				{
					AssetTypeColor = AssetTypeActions.Pin()->GetTypeColor().ReinterpretAsLinear();
				}
			}
		}
	}

	return AssetTypeColor;
}

FString SAssetColumnItem::GetAssetNameToolTipText() const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() == EAssetItemType::Folder)
		{
			FString Result = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderName.ToString();
			Result += TEXT("\n");
			Result += LOCTEXT("FolderName", "Folder").ToString();

			return Result;
		}
		else
		{
			const FString AssetName = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.AssetName.ToString();
			const FString AssetType = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.AssetClass.ToString();

			FString Result = AssetName;
			Result += TEXT("\n");
			Result += AssetType;

			return Result;
		}
	}
	else
	{
		return FString();
	}
}

FText SAssetColumnItem::GetAssetClassText() const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			return FText::FromName(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.AssetClass);
		}
		else
		{
			return LOCTEXT("FolderName", "Folder");
		}
	}

	return FText();
}

FText SAssetColumnItem::GetAssetPathText() const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			return FText::FromName(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.PackagePath);
		}
		else
		{
			return FText::FromString(StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath);
		}
	}
	else
	{
		return FText();
	}
}

FText SAssetColumnItem::GetAssetTagText(FName AssetRegistryTag) const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			const FString* TagValue = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.TagsAndValues.Find(AssetRegistryTag);
			if ( TagValue != NULL )
			{
				return FText::FromString(*TagValue);
			}
		}
	}
	
	return FText();
}

#undef LOCTEXT_NAMESPACE

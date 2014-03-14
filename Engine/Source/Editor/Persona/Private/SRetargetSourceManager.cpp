// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "SRetargetSourceManager.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "AssetRegistryModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "AssetNotifications.h"

#define LOCTEXT_NAMESPACE "SRetargetSourceManager"

static const FName ColumnId_RetargetSourceNameLabel( "Retarget Source Name" );
static const FName ColumnID_BaseReferenceMeshLabel( "Reference Mesh" );

DECLARE_DELEGATE_TwoParams( FOnRenameCommit, const FName & /*OldName*/,  const FString& /*NewName*/ )
DECLARE_DELEGATE_RetVal_ThreeParams( bool, FOnVerifyRenameCommit, const FName & /*OldName*/, const FString& /*NewName*/, FText& /*OutErrorMessage*/)

//////////////////////////////////////////////////////////////////////////
// SRetargetSourceListRow

typedef TSharedPtr< FDisplayedRetargetSourceInfo > FDisplayedRetargetSourceInfoPtr;

class SRetargetSourceListRow
	: public SMultiColumnTableRow< FDisplayedRetargetSourceInfoPtr >
{
public:

	SLATE_BEGIN_ARGS( SRetargetSourceListRow ) {}

	/** The item for this row **/
	SLATE_ARGUMENT( FDisplayedRetargetSourceInfoPtr, Item )

	/* The SRetargetSourceManager that handles all retarget sources */
	SLATE_ARGUMENT( class SRetargetSourceManager*, RetargetSourceManager )

	/* Widget used to display the list of retarget sources*/
	SLATE_ARGUMENT( TSharedPtr<SRetargetSourceListType>, RetargetSourceListView )

	/* Persona used to update the viewport when a weight slider is dragged */
	SLATE_ARGUMENT( TWeakPtr<FPersona>, Persona )

	/** Delegate for when an asset name has been entered for an item that is in a rename state */
	SLATE_EVENT( FOnRenameCommit, OnRenameCommit )

	/** Delegate for when an asset name has been entered for an item to verify the name before commit */
	SLATE_EVENT( FOnVerifyRenameCommit, OnVerifyRenameCommit )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView );

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE;

private:

	/* Returns the reference mesh this pose is based on	*/
	FString GetReferenceMeshName() { return Item->ReferenceMesh->GetPathName(); }

	/* The SRetargetSourceManager that handles all retarget sources*/
	SRetargetSourceManager* RetargetSourceManager;

	/** Widget used to display the list of retarget sources*/
	TSharedPtr<SRetargetSourceListType> RetargetSourceListView;

	/** The name and weight of the retarget source*/
	FDisplayedRetargetSourceInfoPtr	Item;

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;

	/** Delegate for when an asset name has been entered for an item that is in a rename state */
	FOnRenameCommit OnRenameCommit;

	/** Delegate for when an asset name has been entered for an item to verify the name before commit */
	FOnVerifyRenameCommit OnVerifyRenameCommit;

	/** Handles committing a name change */
	virtual void HandleNameCommitted( const FText& NewText, ETextCommit::Type CommitInfo );

	/** Handles verifying a name change */
	virtual bool HandleVerifyNameChanged( const FText& NewText, FText& OutErrorMessage );
};

void SRetargetSourceListRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Item = InArgs._Item;
	RetargetSourceManager = InArgs._RetargetSourceManager;
	RetargetSourceListView = InArgs._RetargetSourceListView;
	OnRenameCommit = InArgs._OnRenameCommit;
	OnVerifyRenameCommit = InArgs._OnVerifyRenameCommit;

	PersonaPtr = InArgs._Persona;

	check( Item.IsValid() );

	SMultiColumnTableRow< FDisplayedRetargetSourceInfoPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > SRetargetSourceListRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	if ( ColumnName == ColumnId_RetargetSourceNameLabel )
	{
		TSharedPtr< SInlineEditableTextBlock > InlineWidget;
		TSharedRef< SWidget > NewWidget = 
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 4.0f )
			.VAlign( VAlign_Center )
			[
				SAssignNew(InlineWidget, SInlineEditableTextBlock)
				.Text( FText::FromString(Item->Name.ToString()) )
				.OnTextCommitted(this, &SRetargetSourceListRow::HandleNameCommitted)
				.OnVerifyTextChanged(this, &SRetargetSourceListRow::HandleVerifyNameChanged)
				.HighlightText( RetargetSourceManager->GetFilterText() )
				.IsReadOnly(false)
				.IsSelected(this, &SMultiColumnTableRow< FDisplayedRetargetSourceInfoPtr >::IsSelectedExclusively)
			];

		Item->OnEnterEditingMode.BindSP( InlineWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);

		return NewWidget;
	}
	else
	{
		// Encase the SSpinbox in an SVertical box so we can apply padding. Setting ItemHeight on the containing SListView has no effect :-(
		return
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 1.0f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( Item->GetReferenceMeshName() )
				.HighlightText( RetargetSourceManager->GetFilterText() )
			];
	}
}

/** Handles committing a name change */
void SRetargetSourceListRow::HandleNameCommitted( const FText& NewText, ETextCommit::Type CommitInfo )
{
	OnRenameCommit.ExecuteIfBound(Item->Name, NewText.ToString());
}

/** Handles verifying a name change */
bool SRetargetSourceListRow::HandleVerifyNameChanged( const FText& NewText, FText& OutErrorMessage )
{
	return !OnVerifyRenameCommit.IsBound() || OnVerifyRenameCommit.Execute(Item->Name, NewText.ToString(), OutErrorMessage);
}
//////////////////////////////////////////////////////////////////////////
// SRetargetSourceManager

void SRetargetSourceManager::Construct(const FArguments& InArgs)
{
	PersonaPtr = InArgs._Persona;
	Skeleton = NULL;

	if ( PersonaPtr.IsValid() )
	{
		Skeleton = PersonaPtr.Pin()->GetSkeleton();
		PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP( this, &SRetargetSourceManager::PostUndo ) );
	}

	FString SkeletonName = Skeleton ? Skeleton->GetName() : LOCTEXT( "RetargetSourceMeshNameLabel", "No Skeleton Present" ).ToString();

	ChildSlot
	[
		SNew( SVerticalBox )
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( STextBlock )
			.Text( SkeletonName )
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2)
		[
			SNew(SHorizontalBox)
			// Filter entry
			+SHorizontalBox::Slot()
			.FillWidth( 1 )
			[
				SAssignNew( NameFilterBox, SSearchBox )
				.SelectAllTextWhenFocused( true )
				.OnTextChanged( this, &SRetargetSourceManager::OnFilterTextChanged )
				.OnTextCommitted( this, &SRetargetSourceManager::OnFilterTextCommitted )
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SAssignNew( RetargetSourceListView, SRetargetSourceListType )
			.ListItemsSource( &RetargetSourceList )
			.OnGenerateRow( this, &SRetargetSourceManager::GenerateRetargetSourceRow )
			.OnContextMenuOpening( this, &SRetargetSourceManager::OnGetContextMenuContent )
			.ItemHeight( 22.0f )
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column( ColumnId_RetargetSourceNameLabel )
				.DefaultLabel( LOCTEXT( "RetargetSourceNameLabel", "Retarget Source Name" ).ToString() )

				+ SHeaderRow::Column( ColumnID_BaseReferenceMeshLabel )
				.DefaultLabel( LOCTEXT( "RetargetSourceWeightLabel", "Source Mesh" ).ToString() )
			)
		]
	];

	CreateRetargetSourceList();
}

void SRetargetSourceManager::OnFilterTextChanged( const FText& SearchText )
{
	FilterText = SearchText;

	CreateRetargetSourceList( SearchText.ToString() );
}

void SRetargetSourceManager::OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo )
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged( SearchText );
}

TSharedRef<ITableRow> SRetargetSourceManager::GenerateRetargetSourceRow(TSharedPtr<FDisplayedRetargetSourceInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check( InInfo.IsValid() );

	return
		SNew( SRetargetSourceListRow, OwnerTable )
		.Persona( PersonaPtr )
		.Item( InInfo )
		.RetargetSourceManager( this )
		.RetargetSourceListView( RetargetSourceListView )
		.OnRenameCommit(this, &SRetargetSourceManager::OnRenameCommit)
		.OnVerifyRenameCommit(this, &SRetargetSourceManager::OnVerifyRenameCommit);
}


void SRetargetSourceManager::OnRenameCommit( const FName & OldName,  const FString& NewName )
{
	FString NewNameString = NewName;
	if (OldName != FName(*NewNameString.Trim().TrimTrailing()))
	{
		const FScopedTransaction Transaction( LOCTEXT("RetargetSourceManager_Rename", "Rename Retarget Source") );

		FReferencePose * Pose = Skeleton->AnimRetargetSources.Find(OldName);
		if (Pose)
		{
			FReferencePose NewPose = *Pose;
			FName NewPoseName = FName(*NewName);
			NewPose.PoseName = NewPoseName;

			Skeleton->Modify();

			Skeleton->AnimRetargetSources.Remove(OldName);
			Skeleton->AnimRetargetSources.Add(NewPoseName, NewPose);

			// need to verify if these pose is used by anybody else
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			TArray<FAssetData> AssetList;
			TMultiMap<FName, FString> TagsAndValues;
			TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(UAnimSequence, RetargetSource), OldName.ToString());
			AssetRegistryModule.Get().GetAssetsByTagValues(TagsAndValues, AssetList);

			// ask users if they'd like to continue and/or fix up
			if (AssetList.Num() > 0 )
			{
				FString ListOfAssets;
				// if users is sure to delete, delete
				for (auto Iter=AssetList.CreateConstIterator(); Iter; ++Iter)
				{
					ListOfAssets += Iter->AssetName.ToString();
					ListOfAssets += TEXT("\n");
				}


				// if so, ask if they'd like us to fix the animations as well
				FString Message = NSLOCTEXT("RetargetSourceEditor", "RetargetSourceRename_FixUpAnimation_Message", "Would you like to fix up the following animation(s)? You'll have to save all the assets in the list.").ToString();
				Message += TEXT("\n");
				Message += ListOfAssets;

				EAppReturnType::Type Result = FMessageDialog::Open( EAppMsgType::YesNo, FText::FromString(Message) );

				if (Result == EAppReturnType::Yes)
				{
					// now fix up all assets
					TArray<UObject*> ObjectsToUpdate;
					for (auto Iter=AssetList.CreateConstIterator(); Iter; ++Iter)
					{
						UAnimSequence * AnimSequence = Cast<UAnimSequence>(Iter->GetAsset());
						if (AnimSequence)
						{
							ObjectsToUpdate.Add(AnimSequence);

							AnimSequence->Modify();
							// clear name
							AnimSequence->RetargetSource = NewPoseName;
						}
					}
				}
			}

			Skeleton->CallbackRetargetSourceChanged();
			FAssetNotifications::SkeletonNeedsToBeSaved(Skeleton);
			CreateRetargetSourceList( NameFilterBox->GetText().ToString() );
		}
	}
}

bool SRetargetSourceManager::OnVerifyRenameCommit( const FName & OldName, const FString& NewName, FText& OutErrorMessage)
{
	// if same reject
	FString NewString = NewName;

	if (OldName == FName(*NewString.Trim().TrimTrailing()))
	{
		OutErrorMessage = FText::Format (LOCTEXT("RetargetSourceManagerNameSame", "{0} Nothing modified"), FText::FromString(OldName.ToString()) );
		return false;
	}

	FReferencePose * Pose = Skeleton->AnimRetargetSources.Find(OldName);
	if (!Pose)
	{
		OutErrorMessage = FText::Format (LOCTEXT("RetargetSourceManagerNameNotFound", "{0} Not found"), FText::FromString(OldName.ToString()) );
		return false;
	}

	Pose = Skeleton->AnimRetargetSources.Find(FName(*NewName));

	if (Pose)
	{
		OutErrorMessage = FText::Format (LOCTEXT("RetargetSourceManagerNameDuplicated", "{0} already exists"), FText::FromString(NewName) );
		return false;
	}

	return true;
}

TSharedPtr<SWidget> SRetargetSourceManager::OnGetContextMenuContent() const
{
	if ( Skeleton )
	{
		const bool bShouldCloseWindowAfterMenuSelection = true;
		FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, NULL);

		MenuBuilder.BeginSection("RetargetSourceAction", LOCTEXT( "New", "New" ) );
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SRetargetSourceManager::OnAddRetargetSource ) );
			const FText Label = LOCTEXT("AddRetargetSourceActionLabel", "Add...");
			const FText ToolTip = LOCTEXT("AddRetargetSourceActionTooltip", "Add new retarget source.");
			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("RetargetSourceAction", LOCTEXT( "Selected", "Selected Item Actions" ) );
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SRetargetSourceManager::OnRenameRetargetSource ), 
				FCanExecuteAction::CreateSP( this, &SRetargetSourceManager::CanPerformRename ) );
			const FText Label = LOCTEXT("RenameRetargetSourceActionLabel", "Rename");
			const FText ToolTip = LOCTEXT("RenameRetargetSourceActionTooltip", "Rename the selected retarget source.");
			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
		}
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SRetargetSourceManager::OnDeleteRetargetSource ), 
				FCanExecuteAction::CreateSP( this, &SRetargetSourceManager::CanPerformDelete ) );
			const FText Label = LOCTEXT("DeleteRetargetSourceActionLabel", "Delete");
			const FText ToolTip = LOCTEXT("DeleteRetargetSourceActionTooltip", "Deletes the selected retarget sources.");
			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
		}
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SRetargetSourceManager::OnRefreshRetargetSource ), 
				FCanExecuteAction::CreateSP( this, &SRetargetSourceManager::CanPerformRefresh ) );
			const FText Label = LOCTEXT("RefreshRetargetSourceActionLabel", "Refresh");
			const FText ToolTip = LOCTEXT("RefreshRetargetSourceActionTooltip", "Refreshs the selected retarget sources.");
			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	return TSharedPtr<SWidget>();


}

/** @TODO: FIXME: Item to rename. Only valid for adding and this is so hacky, I know**/
//TSharedPtr<FDisplayedBasePoseInfo> ItemToRename;

void SRetargetSourceManager::CreateRetargetSourceList( const FString& SearchText, const FName  NewName )
{
	RetargetSourceList.Empty();

	if ( Skeleton )
	{
		bool bDoFiltering = !SearchText.IsEmpty();

		for (auto Iter=Skeleton->AnimRetargetSources.CreateConstIterator(); Iter; ++Iter)
		{
			const FName & Name = Iter.Key();
			const FReferencePose & RefPose = Iter.Value();

			if ( bDoFiltering )
			{
				if (!Name.ToString().Contains( SearchText ))
				{
					continue; // Skip items that don't match our filter
				}
				if (RefPose.ReferenceMesh && RefPose.ReferenceMesh->GetPathName().Contains( SearchText ))
				{
					continue;
				}
			}

			TSharedRef<FDisplayedRetargetSourceInfo> Info = FDisplayedRetargetSourceInfo::Make( Name, RefPose.ReferenceMesh );

			if (Name == NewName)
			{
				ItemToRename = Info;
			}

			RetargetSourceList.Add( Info );
		}
	}

	RetargetSourceListView->RequestListRefresh();
}

int32 SRetargetSourceManager::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	int32 Result = SCompoundWidget::OnPaint( AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );

	// I need to do this after first paint	
	if (ItemToRename.IsValid())
	{
		ItemToRename->RequestRename();
		ItemToRename = NULL;
	}

	return Result;
}

void SRetargetSourceManager::AddRetargetSource( const FName Name, USkeletalMesh * ReferenceMesh  )
{
	// need to verify if the name is unique, if not create unique name
	if ( Skeleton )
	{
		int32 IntSuffix = 1;
		FReferencePose * ExistingPose = NULL;
		FString NewSourceName = TEXT("");
		do
		{
			if ( IntSuffix <= 1 )
			{
				NewSourceName = FString::Printf( TEXT("%s"), *Name.ToString());
			}
			else
			{
				NewSourceName = FString::Printf( TEXT("%s%d"), *Name.ToString(), IntSuffix );
			}

			ExistingPose = Skeleton->AnimRetargetSources.Find(FName(*NewSourceName));
			IntSuffix++;
		}
		while (ExistingPose != NULL);

		// add new one
		// remap to skeleton refpose
		// we have to do this whenever skeleton changes
		FReferencePose RefPose;
		RefPose.PoseName = FName(*NewSourceName);
		RefPose.ReferenceMesh = ReferenceMesh;

		const FScopedTransaction Transaction( LOCTEXT("RetargetSourceManager_Add", "Add New Retarget Source") );
		Skeleton->Modify();

		Skeleton->AnimRetargetSources.Add(Name, RefPose);
		// ask skeleton to update retarget source for the given name
		Skeleton->UpdateRetargetSource(Name);

		Skeleton->CallbackRetargetSourceChanged();
		FAssetNotifications::SkeletonNeedsToBeSaved(Skeleton);

		// clear search filter
		NameFilterBox->SetText(FText::GetEmpty());
		CreateRetargetSourceList( NameFilterBox->GetText().ToString(), Name );
	}
}

void SRetargetSourceManager::OnAddRetargetSource()
{
	// show list of skeletalmeshes that they can choose from
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SRetargetSourceManager::OnAssetSelectedFromMeshPicker);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.ThumbnailScale = 0.0f;

	if(Skeleton)
	{
		FString SkeletonString = FAssetData(Skeleton).GetExportTextName();
		AssetPickerConfig.Filter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(USkeletalMesh, Skeleton), SkeletonString);
	}

	TSharedRef<SWidget> Widget = SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(768)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.25f, 0.25f, 0.25f, 1.f))
			.Padding( 2 )
			[
				SNew(SBorder)
				.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
				.Padding( 8 )
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		];

	FSlateApplication::Get().PushMenu(
		AsShared(),
		Widget,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TopMenu )
		);
}

void SRetargetSourceManager::OnAssetSelectedFromMeshPicker(const FAssetData& AssetData)
{
	USkeletalMesh * SelectedMesh = CastChecked<USkeletalMesh> (AssetData.GetAsset());

	// make sure you don't have any more retarget sources from the same mesh
	for (auto Iter=Skeleton->AnimRetargetSources.CreateConstIterator(); Iter; ++Iter)
	{
		const FName & Name = Iter.Key();
		const FReferencePose & RefPose = Iter.Value();

		if (RefPose.ReferenceMesh == SelectedMesh)
		{
			// redundant source exists
			
			FSlateApplication::Get().DismissAllMenus();
			return;
		}
	}

	// give temporary name, and make it editable first time
	AddRetargetSource(FName(*SelectedMesh->GetName()), SelectedMesh);
	FSlateApplication::Get().DismissAllMenus();
}

bool SRetargetSourceManager::CanPerformDelete() const
{
	TArray< TSharedPtr< FDisplayedRetargetSourceInfo > > SelectedRows = RetargetSourceListView->GetSelectedItems();
	return SelectedRows.Num() > 0;
}

void SRetargetSourceManager::OnDeleteRetargetSource()
{
	TArray< TSharedPtr< FDisplayedRetargetSourceInfo > > SelectedRows = RetargetSourceListView->GetSelectedItems();
	
	const FScopedTransaction Transaction( LOCTEXT("RetargetSourceManager_Delete", "Delete Retarget Source") );
	for (int RowIndex = 0; RowIndex < SelectedRows.Num(); ++RowIndex)
	{
		const FReferencePose * PoseFound = Skeleton->AnimRetargetSources.Find(SelectedRows[RowIndex]->Name);
		if(PoseFound)
		{
			// need to verify if these pose is used by anybody else
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			TArray<FAssetData> AssetList;
			TMultiMap<FName, FString> TagsAndValues;
			TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(UAnimSequence, RetargetSource), PoseFound->PoseName.ToString());
			AssetRegistryModule.Get().GetAssetsByTagValues(TagsAndValues, AssetList);

			// ask users if they'd like to continue and/or fix up
			if (AssetList.Num() > 0 )
			{
				FString ListOfAssets;
				// if users is sure to delete, delete
				for (auto Iter=AssetList.CreateConstIterator(); Iter; ++Iter)
				{
					ListOfAssets += Iter->AssetName.ToString();
					ListOfAssets += TEXT("\n");
				}

				// ask if they'd like to continue deleting this pose regardless animation references
				FString Message = NSLOCTEXT("RetargetSourceEditor", "RetargetSourceDeleteMessage", "Following animation(s) is(are) referencing this pose. Are you sure if you'd like to delete this pose?").ToString();
				Message += TEXT("\n\n");
				Message += ListOfAssets;

				EAppReturnType::Type Result = FMessageDialog::Open( EAppMsgType::YesNo, FText::FromString(Message) );

				if (Result == EAppReturnType::No)
				{
					continue;
				}

				// if so, ask if they'd like us to fix the animations as well
				Message = NSLOCTEXT("RetargetSourceEditor", "RetargetSourceDelete_FixUpAnimation_Message", "Would you like to fix up the following animation(s)? You'll have to save all the assets in the list.").ToString();
				Message += TEXT("\n");
				Message += ListOfAssets;

				Result = FMessageDialog::Open( EAppMsgType::YesNo, FText::FromString(Message) );

				if (Result == EAppReturnType::No)
				{
					continue;
				}

				// now fix up all assets
				TArray<UObject *> ObjectsToUpdate;
				for (auto Iter=AssetList.CreateConstIterator(); Iter; ++Iter)
				{
					UAnimSequence * AnimSequence = Cast<UAnimSequence>(Iter->GetAsset());
					if (AnimSequence)
					{
						ObjectsToUpdate.Add(AnimSequence);

						AnimSequence->Modify();
						// clear name
						AnimSequence->RetargetSource = NAME_None;
					}
				}
			}

			Skeleton->Modify();
			// delete now
			Skeleton->AnimRetargetSources.Remove(SelectedRows[RowIndex]->Name);
			Skeleton->CallbackRetargetSourceChanged();
			FAssetNotifications::SkeletonNeedsToBeSaved(Skeleton);
		}
	}

	CreateRetargetSourceList( NameFilterBox->GetText().ToString() );
}

bool SRetargetSourceManager::CanPerformRename() const
{
	TArray< TSharedPtr< FDisplayedRetargetSourceInfo > > SelectedRows = RetargetSourceListView->GetSelectedItems();
	return SelectedRows.Num() == 1;
}

void SRetargetSourceManager::OnRenameRetargetSource()
{
	TArray< TSharedPtr< FDisplayedRetargetSourceInfo > > SelectedRows = RetargetSourceListView->GetSelectedItems();

	if (ensure (SelectedRows.Num() == 1))
	{
		int32 RowIndex = 0;
		const FReferencePose * PoseFound = Skeleton->AnimRetargetSources.Find(SelectedRows[RowIndex]->Name);
		if(PoseFound)
		{
			// we used to verify if there is any animation referencing and warn them, but that doesn't really help
			// because you can rename by just double click as well, and it slows process, so removed it
			// request rename
			SelectedRows[RowIndex]->RequestRename();
		}
	}
}

bool SRetargetSourceManager::CanPerformRefresh() const
{
	TArray< TSharedPtr< FDisplayedRetargetSourceInfo > > SelectedRows = RetargetSourceListView->GetSelectedItems();
	return SelectedRows.Num() > 0;
}

void SRetargetSourceManager::OnRefreshRetargetSource()
{
	TArray< TSharedPtr< FDisplayedRetargetSourceInfo > > SelectedRows = RetargetSourceListView->GetSelectedItems();

	const FScopedTransaction Transaction( LOCTEXT("RetargetSourceManager_Refresh", "Refresh Retarget Source") );
	for (int RowIndex = 0; RowIndex < SelectedRows.Num(); ++RowIndex)
	{
		Skeleton->Modify();
		// refresh pose now
		FName RetargetSourceName = SelectedRows[RowIndex]->Name;
		Skeleton->UpdateRetargetSource(RetargetSourceName);

		// feedback of pose has been updated
		FFormatNamedArguments Args;
		Args.Add( TEXT("RetargetSourceName"), FText::FromString( RetargetSourceName.ToString() ) );
		FNotificationInfo Info( FText::Format( LOCTEXT("RetargetSourceManager_RefreshPose", "Retarget Source {RetargetSourceName} is refreshed"), Args ) );
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if ( Notification.IsValid() )
		{
			Notification->SetCompletionState( SNotificationItem::CS_None );
		}
	}

	FAssetNotifications::SkeletonNeedsToBeSaved(Skeleton);
}

SRetargetSourceManager::~SRetargetSourceManager()
{
	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->UnregisterOnPostUndo(this);
	}
}

void SRetargetSourceManager::PostUndo()
{
	CreateRetargetSourceList();
}

#undef LOCTEXT_NAMESPACE


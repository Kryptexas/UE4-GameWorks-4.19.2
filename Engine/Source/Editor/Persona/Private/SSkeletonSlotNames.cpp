// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "Persona.h"
#include "SSkeletonSlotNames.h"
#include "AssetRegistryModule.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SkeletonSlotNames"

static const FName ColumnId_SlotNameLabel( "SlotName" );

//////////////////////////////////////////////////////////////////////////
// SMorphTargetListRow

typedef TSharedPtr< FDisplayedSlotNameInfo > FDisplayedSlotNameInfoPtr;

class SSlotNameListRow
	: public SMultiColumnTableRow< FDisplayedSlotNameInfoPtr >
{
public:

	SLATE_BEGIN_ARGS( SSlotNameListRow ) {}

	/** The item for this row **/
	SLATE_ARGUMENT( FDisplayedSlotNameInfoPtr, Item )

	/* Widget used to display the list of morph targets */
	SLATE_ARGUMENT( TSharedPtr<SSkeletonSlotNames>, SlotNameListView )

	/* Persona used to update the viewport when a weight slider is dragged */
	SLATE_ARGUMENT( TWeakPtr<FPersona>, Persona )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView );

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

private:

	/** Widget used to display the list of slot name */
	TSharedPtr<SSkeletonSlotNames> SlotNameListView;

	/** The notify being displayed by this row */
	FDisplayedSlotNameInfoPtr	Item;

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;
};

void SSlotNameListRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Item = InArgs._Item;
	SlotNameListView = InArgs._SlotNameListView;
	PersonaPtr = InArgs._Persona;

	check( Item.IsValid() );

	SMultiColumnTableRow< FDisplayedSlotNameInfoPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > SSlotNameListRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	check( ColumnName == ColumnId_SlotNameLabel );

	return SNew( SVerticalBox )
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding( 0.0f, 4.0f )
	.VAlign( VAlign_Center )
	[
		SAssignNew(Item->InlineEditableText, SInlineEditableTextBlock)
		.Text( FText::FromName(Item->Name) )
		.OnVerifyTextChanged(SlotNameListView.Get(), &SSkeletonSlotNames::OnVerifyNotifyNameCommit, Item)
		.OnTextCommitted(SlotNameListView.Get(), &SSkeletonSlotNames::OnNotifyNameCommitted, Item)
		.IsSelected(SlotNameListView.Get(), &SSkeletonSlotNames::IsSelected)
	];
}

/////////////////////////////////////////////////////
// FSkeletonSlotNamesSummoner

FSkeletonSlotNamesSummoner::FSkeletonSlotNamesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::SkeletonSlotNamesID, InHostingApp)
{
	TabLabel = LOCTEXT("SkeletonSlotNamesTabTitle", "Animation SlotName");

	EnableTabPadding();
	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("SkeletonSlotNamesMenu", "Animation SlotName");
	ViewMenuTooltip = LOCTEXT("SkeletonSlotNames_ToolTip", "Shows the skeletons slot name list");
}

TSharedRef<SWidget> FSkeletonSlotNamesSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SSkeletonSlotNames)
		.Persona(StaticCastSharedPtr<FPersona>(HostingApp.Pin()));
}

/////////////////////////////////////////////////////
// SSkeletonSlotNames

void SSkeletonSlotNames::Construct(const FArguments& InArgs)
{
	PersonaPtr = InArgs._Persona;
	TargetSkeleton = PersonaPtr.Pin()->GetSkeleton();

	PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP( this, &SSkeletonSlotNames::PostUndo ) );

	this->ChildSlot
	[
		SNew( SVerticalBox )

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin( 0.0f, 0.0f, 0.0f, 4.0f ) )
		[
			SAssignNew( NameFilterBox, SSearchBox )
			.SelectAllTextWhenFocused( true )
			.OnTextChanged( this, &SSkeletonSlotNames::OnFilterTextChanged )
			.OnTextCommitted( this, &SSkeletonSlotNames::OnFilterTextCommitted )
			.HintText( LOCTEXT( "SlotNameSearchBoxHint", "Search Animation SlotName...") )
		]

		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SAssignNew( SlotNameListView, SSlotNameListType )
			.ListItemsSource( &NotifyList )
			.OnGenerateRow( this, &SSkeletonSlotNames::GenerateNotifyRow )
			.OnContextMenuOpening( this, &SSkeletonSlotNames::OnGetContextMenuContent )
			.OnSelectionChanged( this, &SSkeletonSlotNames::OnNotifySelectionChanged )
			.ItemHeight( 22.0f )
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column( ColumnId_SlotNameLabel )
				.DefaultLabel( LOCTEXT( "SlotNameNameLabel", "Slot Name" ).ToString() )
			)
		]
	];

	CreateSlotNameList();
}

SSkeletonSlotNames::~SSkeletonSlotNames()
{
	if ( PersonaPtr.IsValid() )
	{
		PersonaPtr.Pin()->UnregisterOnPostUndo( this );
	}
}

void SSkeletonSlotNames::OnFilterTextChanged( const FText& SearchText )
{
	FilterText = SearchText;

	RefreshSlotNameListWithFilter();
}

void SSkeletonSlotNames::OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo )
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged( SearchText );
}

TSharedRef<ITableRow> SSkeletonSlotNames::GenerateNotifyRow(TSharedPtr<FDisplayedSlotNameInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check( InInfo.IsValid() );

	return
		SNew( SSlotNameListRow, OwnerTable )
		.Persona( PersonaPtr )
		.Item( InInfo )
		.SlotNameListView( SharedThis( this ) );
}

TSharedPtr<SWidget> SSkeletonSlotNames::OnGetContextMenuContent() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, NULL);

	MenuBuilder.BeginSection("SlotNameAction", LOCTEXT( "SlotNameActions", "Selected Notify Actions" ) );
	{
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SSkeletonSlotNames::OnDeleteSlotName ), 
				FCanExecuteAction::CreateSP( this, &SSkeletonSlotNames::CanPerformDelete ) );
			const FText Label = LOCTEXT("DeleteSlotNameButtonLabel", "Delete");
			const FText ToolTip = LOCTEXT("DeleteSlotNameButtonTooltip", "Deletes the selected anim SlotName.");
			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
		}

		// for now disable rename, it doesn't work correctly
// 		{
// 			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SSkeletonSlotNames::OnRenameSlotName ), 
// 				FCanExecuteAction::CreateSP( this, &SSkeletonSlotNames::CanPerformRename ) );
// 			const FText Label = LOCTEXT("RenameSlotNameButtonLabel", "Rename");
// 			const FText ToolTip = LOCTEXT("RenameSlotNameButtonTooltip", "Renames the selected anim SlotName.");
// 			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
// 		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SSkeletonSlotNames::OnNotifySelectionChanged(TSharedPtr<FDisplayedSlotNameInfo> Selection, ESelectInfo::Type SelectInfo)
{
	if(Selection.IsValid())
	{
		ShowNotifyInDetailsView(Selection->Name);
	}
}

bool SSkeletonSlotNames::CanPerformDelete() const
{
	TArray< TSharedPtr< FDisplayedSlotNameInfo > > SelectedRows = SlotNameListView->GetSelectedItems();
	return SelectedRows.Num() > 0;
}

bool SSkeletonSlotNames::CanPerformRename() const
{
	TArray< TSharedPtr< FDisplayedSlotNameInfo > > SelectedRows = SlotNameListView->GetSelectedItems();
	return SelectedRows.Num() == 1;
}

void SSkeletonSlotNames::OnDeleteSlotName()
{
	TArray< TSharedPtr< FDisplayedSlotNameInfo > > SelectedRows = SlotNameListView->GetSelectedItems();

	const FScopedTransaction Transaction( LOCTEXT("DeleteSlotName", "Delete Slot Name") );

	// this one deletes all SlotName with same name. 
	TArray<FName> SelectedSlotNames;
	TargetSkeleton->Modify();

	for(int Selection = 0; Selection < SelectedRows.Num(); ++Selection)
	{
		FName& SelectedName = SelectedRows[Selection]->Name;
		TargetSkeleton->RemoveSlotNodeName(SelectedName);
		SelectedSlotNames.Add(SelectedName);
	}

	TArray<FAssetData> CompatibleAnimMontages;
	GetCompatibleAnimMontage(CompatibleAnimMontages);

	int32 NumAnimationsModified = 0;

	for( int32 AssetIndex = 0; AssetIndex < CompatibleAnimMontages.Num(); ++AssetIndex )
	{
		const FAssetData& PossibleAnimAsset = CompatibleAnimMontages[AssetIndex];
		UAnimMontage* Montage = CastChecked<UAnimMontage>(PossibleAnimAsset.GetAsset());
	
		for (auto & SlotAnim : Montage->SlotAnimTracks)
		{
			if ( SelectedSlotNames.Contains(SlotAnim.SlotName) )
			{
				// found one, set transaction mark
				Montage->Modify();

				// if this name is deleted, just clear it
				// I don't think changing to something else will help
				SlotAnim.SlotName = NAME_None;
				// although people shouldn't have same slot name in the same montage, 
				// just in case anybody does, it will loop
				// this might cause issue because Modify will get called again.

				++NumAnimationsModified;
			}
		}
	}

// 	TMultiMap<class UAnimBlueprint *, class UAnimGraphNode_Slot *> AssetsToFix;
// 	GetCompatibleAnimBlueprints( AssetsToFix );

	if(NumAnimationsModified > 0)
	{
		// Tell the user that the socket is a duplicate
		FFormatNamedArguments Args;
		Args.Add( TEXT("NumAnimationsModified"), NumAnimationsModified );
		FNotificationInfo Info( FText::Format( LOCTEXT( "SlotNamesDeleted", "{NumAnimationsModified} animation(s) modified to delete notifications" ), Args ) );

		Info.bUseLargeFont = false;
		Info.ExpireDuration = 5.0f;

		NotifyUser( Info );
	}

	CreateSlotNameList( NameFilterBox->GetText().ToString() );
}

// void SSkeletonSlotNames::GetCompatibleAnimBlueprints( TMultiMap<class UAnimBlueprint *, class UAnimGraphNode_Slot *>& OutAssets )
// {
// 	// second, go through all animgraphnode_slot
// 	for(FObjectIterator Iter(UAnimGraphNode_Slot::StaticClass()); Iter; ++Iter)
// 	{
// 		// if I find one, add the BP and slot nodes to the list
// 		UAnimGraphNode_Slot * SlotNode = CastChecked<UAnimGraphNode_Slot>(Iter);
// 		UAnimBlueprint * AnimBlueprint = Cast<UAnimBlueprint>(SlotNode->GetBlueprint());
// 		if (AnimBlueprint)
// 		{
// 			OutAssets.AddUnique(AnimBlueprint, SlotNode);
// 		}
// 	}
// }

void SSkeletonSlotNames::OnRenameSlotName()
{
	TArray< TSharedPtr< FDisplayedSlotNameInfo > > SelectedRows = SlotNameListView->GetSelectedItems();

	check(SelectedRows.Num() == 1); // Should be guaranteed by CanPerformRename

	SelectedRows[0]->InlineEditableText->EnterEditingMode();
}

bool SSkeletonSlotNames::OnVerifyNotifyNameCommit( const FText& NewName, FText& OutErrorMessage, TSharedPtr<FDisplayedSlotNameInfo> Item )
{
	bool bValid(true);

	if(NewName.IsEmpty())
	{
		OutErrorMessage = LOCTEXT( "NameMissing_Error", "You must provide a name." );
		bValid = false;
	}

	FName NotifyName( *NewName.ToString() );
	if(NotifyName != Item->Name)
	{
		if(TargetSkeleton->DoesHaveSlotNodeName(NotifyName))
		{
			OutErrorMessage = FText::Format( LOCTEXT("AlreadyInUseMessage", "'{0}' is already in use."), NewName );
			bValid = false;
		}
	}

	return bValid;
}

void SSkeletonSlotNames::OnNotifyNameCommitted( const FText& NewName, ETextCommit::Type, TSharedPtr<FDisplayedSlotNameInfo> Item )
{
	const FScopedTransaction Transaction( LOCTEXT("RenameSlotName", "Rename Anim Notify") );

	FName NewSlotName = FName( *NewName.ToString() );
	FName SlotToRename = Item->Name;

	// rename notify in skeleton
	TargetSkeleton->Modify();

	TArray<FAssetData> CompatibleAnimMontages;
	GetCompatibleAnimMontage(CompatibleAnimMontages);

	int32 NumAnimationsModified = 0;

	for(int32 AssetIndex = 0; AssetIndex < CompatibleAnimMontages.Num(); ++AssetIndex)
	{
		const FAssetData& PossibleAnimAsset = CompatibleAnimMontages[AssetIndex];
		UAnimMontage* Montage = Cast<UAnimMontage>(PossibleAnimAsset.GetAsset());

		for (auto & SlotAnim : Montage->SlotAnimTracks)
		{
			if(SlotAnim.SlotName == SlotToRename)
			{
				// found one, set transaction mark
				Montage->Modify();

				// if this name is deleted, just clear it
				// I don't think changing to something else will help
				SlotAnim.SlotName = NewSlotName;
				// although people shouldn't have same slot name in the same montage, 
				// just in case anybody does, it will loop
				// this might cause issue because Modify will get called again.

				++NumAnimationsModified;
			}
		}
	}

	if(NumAnimationsModified > 0)
	{
		// Tell the user that the socket is a duplicate
		FFormatNamedArguments Args;
		Args.Add( TEXT("NumAnimationsModified"), NumAnimationsModified );
		FNotificationInfo Info( FText::Format( LOCTEXT( "SlotNamesRenamed", "{NumAnimationsModified} animation(s) modified to rename notification" ), Args ) );

		Info.bUseLargeFont = false;
		Info.ExpireDuration = 5.0f;

		NotifyUser( Info );
	}

	RefreshSlotNameListWithFilter();
}

void SSkeletonSlotNames::RefreshSlotNameListWithFilter()
{
	CreateSlotNameList( NameFilterBox->GetText().ToString() );
}

void SSkeletonSlotNames::CreateSlotNameList( const FString& SearchText )
{
	NotifyList.Empty();

	if ( TargetSkeleton )
	{
		const TArray<FName> & SlotNodeNames = TargetSkeleton->GetSlotNodeNames();
		for(int i = 0; i < SlotNodeNames.Num(); ++i)
		{
			const FName& NotifyName = SlotNodeNames[i];
			if ( !SearchText.IsEmpty() )
			{
				if ( NotifyName.ToString().Contains( SearchText ) )
				{
					NotifyList.Add( FDisplayedSlotNameInfo::Make( NotifyName ) );
				}
			}
			else
			{
				NotifyList.Add( FDisplayedSlotNameInfo::Make( NotifyName ) );
			}
		}
	}

	SlotNameListView->RequestListRefresh();
}

void SSkeletonSlotNames::ShowNotifyInDetailsView(FName NotifyName)
{
	// @todo nothing to show now, but in the future
	// we can show the list of montage that are used by this slot node?
}

void SSkeletonSlotNames::GetCompatibleAnimMontage(TArray<class FAssetData>& OutAssets)
{
	//Get the skeleton tag to search for
	FString SkeletonExportName = FAssetData(TargetSkeleton).GetExportTextName();

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UAnimMontage::StaticClass()->GetFName(), AssetDataList, true);

	OutAssets.Empty(AssetDataList.Num());

	for( int32 AssetIndex = 0; AssetIndex < AssetDataList.Num(); ++AssetIndex )
	{
		const FAssetData& PossibleAnimMontage = AssetDataList[AssetIndex];
		if( SkeletonExportName == PossibleAnimMontage.TagsAndValues.FindRef("Skeleton") )
		{
			OutAssets.Add( PossibleAnimMontage );
		}
	}
}

UObject* SSkeletonSlotNames::ShowInDetailsView( UClass* EdClass )
{
	UObject *Obj = EditorObjectTracker.GetEditorObjectForClass(EdClass);

	if(Obj != NULL)
	{
		PersonaPtr.Pin()->SetDetailObject(Obj);
	}
	return Obj;
}

void SSkeletonSlotNames::ClearDetailsView()
{
	PersonaPtr.Pin()->SetDetailObject(NULL);
}

void SSkeletonSlotNames::PostUndo()
{
	RefreshSlotNameListWithFilter();
}

void SSkeletonSlotNames::AddReferencedObjects( FReferenceCollector& Collector )
{
	EditorObjectTracker.AddReferencedObjects(Collector);
}

void SSkeletonSlotNames::NotifyUser( FNotificationInfo& NotificationInfo )
{
	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification( NotificationInfo );
	if ( Notification.IsValid() )
	{
		Notification->SetCompletionState( SNotificationItem::CS_Fail );
	}
}
#undef LOCTEXT_NAMESPACE

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "Persona.h"
#include "SSkeletonSlotGroupNames.h"
#include "AssetRegistryModule.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SkeletonSlotGroupNames"

static const FName ColumnId_SlotGroupNameLabel( "SlotGroupName" );

//////////////////////////////////////////////////////////////////////////
// SMorphTargetListRow

typedef TSharedPtr< FDisplayedSlotGroupNameInfo > FDisplayedSlotGroupNameInfoPtr;

class SSlotGroupNameListRow
	: public SMultiColumnTableRow< FDisplayedSlotGroupNameInfoPtr >
{
public:

	SLATE_BEGIN_ARGS( SSlotGroupNameListRow ) {}

	/** The item for this row **/
	SLATE_ARGUMENT( FDisplayedSlotGroupNameInfoPtr, Item )

	/* Widget used to display the list of morph targets */
	SLATE_ARGUMENT( TSharedPtr<SSkeletonSlotGroupNames>, SlotGroupNameListView )

	/* Persona used to update the viewport when a weight slider is dragged */
	SLATE_ARGUMENT( TWeakPtr<FPersona>, Persona )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView );

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE;

private:

	/** Widget used to display the list of slot name */
	TSharedPtr<SSkeletonSlotGroupNames> SlotGroupNameListView;

	/** The notify being displayed by this row */
	FDisplayedSlotGroupNameInfoPtr	Item;

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;
};

void SSlotGroupNameListRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Item = InArgs._Item;
	SlotGroupNameListView = InArgs._SlotGroupNameListView;
	PersonaPtr = InArgs._Persona;

	check( Item.IsValid() );

	SMultiColumnTableRow< FDisplayedSlotGroupNameInfoPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > SSlotGroupNameListRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	check( ColumnName == ColumnId_SlotGroupNameLabel );

	return SNew( SVerticalBox )
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding( 0.0f, 4.0f )
	.VAlign( VAlign_Center )
	[
		SAssignNew(Item->InlineEditableText, SInlineEditableTextBlock)
		.Text( FText::FromName(Item->Name) )
		.OnVerifyTextChanged(SlotGroupNameListView.Get(), &SSkeletonSlotGroupNames::OnVerifyNotifyNameCommit, Item)
		.OnTextCommitted(SlotGroupNameListView.Get(), &SSkeletonSlotGroupNames::OnNotifyNameCommitted, Item)
		.IsSelected(SlotGroupNameListView.Get(), &SSkeletonSlotGroupNames::IsSelected)
	];
}

/////////////////////////////////////////////////////
// FSkeletonSlotGroupNamesSummoner

FSkeletonSlotGroupNamesSummoner::FSkeletonSlotGroupNamesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::SkeletonSlotGroupNamesID, InHostingApp)
{
	TabLabel = LOCTEXT("SkeletonSlotGroupNamesTabTitle", "Animation Slot Group Name");

	EnableTabPadding();
	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("SkeletonSlotGroupNamesMenu", "Animation Slot Group Name");
	ViewMenuTooltip = LOCTEXT("SkeletonSlotGroupNames_ToolTip", "Shows the skeletons slot name list");
}

TSharedRef<SWidget> FSkeletonSlotGroupNamesSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SSkeletonSlotGroupNames)
		.Persona(StaticCastSharedPtr<FPersona>(HostingApp.Pin()));
}

/////////////////////////////////////////////////////
// SSkeletonSlotGroupNames

void SSkeletonSlotGroupNames::Construct(const FArguments& InArgs)
{
	PersonaPtr = InArgs._Persona;
	TargetSkeleton = PersonaPtr.Pin()->GetSkeleton();

	PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP( this, &SSkeletonSlotGroupNames::PostUndo ) );

	this->ChildSlot
	[
		SNew( SVerticalBox )

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin( 0.0f, 0.0f, 0.0f, 4.0f ) )
		[
			SAssignNew( NameFilterBox, SSearchBox )
			.SelectAllTextWhenFocused( true )
			.OnTextChanged( this, &SSkeletonSlotGroupNames::OnFilterTextChanged )
			.OnTextCommitted( this, &SSkeletonSlotGroupNames::OnFilterTextCommitted )
			.HintText( LOCTEXT( "SlotGroupNameSearchBoxHint", "Search Animation Slot Group Name...") )
		]

		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SAssignNew( SlotGroupNameListView, SSlotGroupNameListType )
			.ListItemsSource( &NotifyList )
			.OnGenerateRow( this, &SSkeletonSlotGroupNames::GenerateNotifyRow )
			.OnContextMenuOpening( this, &SSkeletonSlotGroupNames::OnGetContextMenuContent )
			.OnSelectionChanged( this, &SSkeletonSlotGroupNames::OnNotifySelectionChanged )
			.ItemHeight( 22.0f )
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column( ColumnId_SlotGroupNameLabel )
				.DefaultLabel( LOCTEXT( "SlotGroupNameNameLabel", "Notify Name" ).ToString() )
			)
		]
	];

	CreateSlotGroupNameList();
}

SSkeletonSlotGroupNames::~SSkeletonSlotGroupNames()
{
	if ( PersonaPtr.IsValid() )
	{
		PersonaPtr.Pin()->UnregisterOnPostUndo( this );
	}
}

void SSkeletonSlotGroupNames::OnFilterTextChanged( const FText& SearchText )
{
	FilterText = SearchText;

	RefreshSlotGroupNameListWithFilter();
}

void SSkeletonSlotGroupNames::OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo )
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged( SearchText );
}

TSharedRef<ITableRow> SSkeletonSlotGroupNames::GenerateNotifyRow(TSharedPtr<FDisplayedSlotGroupNameInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check( InInfo.IsValid() );

	return
		SNew( SSlotGroupNameListRow, OwnerTable )
		.Persona( PersonaPtr )
		.Item( InInfo )
		.SlotGroupNameListView( SharedThis( this ) );
}

TSharedPtr<SWidget> SSkeletonSlotGroupNames::OnGetContextMenuContent() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, NULL);

	MenuBuilder.BeginSection("SlotGroupNameAction", LOCTEXT( "SlotGroupNameActions", "Selected Notify Actions" ) );
	{
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SSkeletonSlotGroupNames::OnDeleteSlotGroupName ), 
				FCanExecuteAction::CreateSP( this, &SSkeletonSlotGroupNames::CanPerformDelete ) );
			const FText Label = LOCTEXT("DeleteSlotGroupNameButtonLabel", "Delete");
			const FText ToolTip = LOCTEXT("DeleteSlotGroupNameButtonTooltip", "Deletes the selected anim SlotGroupName.");
			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
		}

		// for now disable rename, it doesn't work correctly
// 		{
// 			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SSkeletonSlotGroupNames::OnRenameSlotGroupName ), 
// 				FCanExecuteAction::CreateSP( this, &SSkeletonSlotGroupNames::CanPerformRename ) );
// 			const FText Label = LOCTEXT("RenameSlotGroupNameButtonLabel", "Rename");
// 			const FText ToolTip = LOCTEXT("RenameSlotGroupNameButtonTooltip", "Renames the selected anim SlotGroupName.");
// 			MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
// 		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SSkeletonSlotGroupNames::OnNotifySelectionChanged(TSharedPtr<FDisplayedSlotGroupNameInfo> Selection, ESelectInfo::Type SelectInfo)
{
	if(Selection.IsValid())
	{
		ShowNotifyInDetailsView(Selection->Name);
	}
}

bool SSkeletonSlotGroupNames::CanPerformDelete() const
{
	TArray< TSharedPtr< FDisplayedSlotGroupNameInfo > > SelectedRows = SlotGroupNameListView->GetSelectedItems();
	
	// if it's trying to delete default slot group name, they can't
	if (SelectedRows.Num() == 1 && SelectedRows[0]->Name == USkeleton::DefaultSlotGroupName)
	{
		return false;
	}

	return SelectedRows.Num() > 0;
}

bool SSkeletonSlotGroupNames::CanPerformRename() const
{
	TArray< TSharedPtr< FDisplayedSlotGroupNameInfo > > SelectedRows = SlotGroupNameListView->GetSelectedItems();
	return SelectedRows.Num() == 1;
}

void SSkeletonSlotGroupNames::OnDeleteSlotGroupName()
{
	TArray< TSharedPtr< FDisplayedSlotGroupNameInfo > > SelectedRows = SlotGroupNameListView->GetSelectedItems();

	const FScopedTransaction Transaction( LOCTEXT("DeleteSlotGroupName", "Delete Slot Name") );

	// this one deletes all SlotGroupName with same name. 
	TargetSkeleton->Modify();

	for(int Selection = 0; Selection < SelectedRows.Num(); ++Selection)
	{
		FName& SelectedName = SelectedRows[Selection]->Name;
		TargetSkeleton->RemoveSlotGroupName(SelectedName);
	}

	CreateSlotGroupNameList( NameFilterBox->GetText().ToString() );
}

// void SSkeletonSlotGroupNames::GetCompatibleAnimBlueprints( TMultiMap<class UAnimBlueprint *, class UAnimGraphNode_Slot *>& OutAssets )
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

void SSkeletonSlotGroupNames::OnRenameSlotGroupName()
{
	TArray< TSharedPtr< FDisplayedSlotGroupNameInfo > > SelectedRows = SlotGroupNameListView->GetSelectedItems();

	check(SelectedRows.Num() == 1); // Should be guaranteed by CanPerformRename

	SelectedRows[0]->InlineEditableText->EnterEditingMode();
}

bool SSkeletonSlotGroupNames::OnVerifyNotifyNameCommit( const FText& NewName, FText& OutErrorMessage, TSharedPtr<FDisplayedSlotGroupNameInfo> Item )
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
		if(TargetSkeleton->DoesHaveSlotGroupName(NotifyName))
		{
			OutErrorMessage = FText::Format( LOCTEXT("AlreadyInUseMessage", "'{0}' is already in use."), NewName );
			bValid = false;
		}
	}

	return bValid;
}

void SSkeletonSlotGroupNames::OnNotifyNameCommitted( const FText& NewName, ETextCommit::Type, TSharedPtr<FDisplayedSlotGroupNameInfo> Item )
{
	const FScopedTransaction Transaction( LOCTEXT("RenameSlotGroupName", "Rename Anim Notify") );

	FName NewSlotGroupName = FName( *NewName.ToString() );
	FName SlotToRename = Item->Name;

	// rename notify in skeleton
	TargetSkeleton->Modify();

	TArray<FAssetData> CompatibleAnimMontages;
	GetCompatibleAnimMontage(CompatibleAnimMontages);

	// @todo need to fix up anim blueprint

	RefreshSlotGroupNameListWithFilter();
}

void SSkeletonSlotGroupNames::RefreshSlotGroupNameListWithFilter()
{
	CreateSlotGroupNameList( NameFilterBox->GetText().ToString() );
}

void SSkeletonSlotGroupNames::CreateSlotGroupNameList( const FString& SearchText )
{
	NotifyList.Empty();

	if ( TargetSkeleton )
	{
		const TArray<FName> & SlotGroupNames = TargetSkeleton->GetSlotGroupNames();
		for(int i = 0; i < SlotGroupNames.Num(); ++i)
		{
			const FName& NotifyName = SlotGroupNames[i];
			if ( !SearchText.IsEmpty() )
			{
				if ( NotifyName.ToString().Contains( SearchText ) )
				{
					NotifyList.Add( FDisplayedSlotGroupNameInfo::Make( NotifyName ) );
				}
			}
			else
			{
				NotifyList.Add( FDisplayedSlotGroupNameInfo::Make( NotifyName ) );
			}
		}
	}

	SlotGroupNameListView->RequestListRefresh();
}

void SSkeletonSlotGroupNames::ShowNotifyInDetailsView(FName NotifyName)
{
	// @todo nothing to show now, but in the future
	// we can show the list of montage that are used by this slot node?
}

void SSkeletonSlotGroupNames::GetCompatibleAnimMontage(TArray<class FAssetData>& OutAssets)
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

UObject* SSkeletonSlotGroupNames::ShowInDetailsView( UClass* EdClass )
{
	UObject *Obj = EditorObjectTracker.GetEditorObjectForClass(EdClass);

	if(Obj != NULL)
	{
		PersonaPtr.Pin()->SetDetailObject(Obj);
	}
	return Obj;
}

void SSkeletonSlotGroupNames::ClearDetailsView()
{
	PersonaPtr.Pin()->SetDetailObject(NULL);
}

void SSkeletonSlotGroupNames::PostUndo()
{
	RefreshSlotGroupNameListWithFilter();
}

void SSkeletonSlotGroupNames::AddReferencedObjects( FReferenceCollector& Collector )
{
	EditorObjectTracker.AddReferencedObjects(Collector);
}

void SSkeletonSlotGroupNames::NotifyUser( FNotificationInfo& NotificationInfo )
{
	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification( NotificationInfo );
	if ( Notification.IsValid() )
	{
		Notification->SetCompletionState( SNotificationItem::CS_Fail );
	}
}
#undef LOCTEXT_NAMESPACE

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "Editor/StaticMeshEditor/Public/IStaticMeshEditor.h"

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#include "SSocketManager.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SSCSSocketManagerEditor"

struct SocketListItem
{
public:
	SocketListItem(UStaticMeshSocket* InSocket)
		: Socket(InSocket)
	{
	}

	/** The static mesh socket this represents */
	UStaticMeshSocket* Socket;

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE(FOnRenameRequested);
	FOnRenameRequested OnRenameRequested;
};

class SSocketDisplayItem : public STableRow< TSharedPtr<FString> >
{
public:
	
	SLATE_BEGIN_ARGS( SSocketDisplayItem )
		{}

		/** The socket this item displays. */
		SLATE_ARGUMENT( TWeakPtr< SocketListItem >, SocketItem )

		/** Pointer back to the socket manager */
		SLATE_ARGUMENT( TWeakPtr< SSocketManager >, SocketManagerPtr )
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 *
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		SocketItem = InArgs._SocketItem;
		SocketManagerPtr = InArgs._SocketManagerPtr;

		TSharedPtr< SInlineEditableTextBlock > InlineWidget;

		this->ChildSlot
		.Padding( 0.0f, 3.0f, 6.0f, 3.0f )
		.VAlign(VAlign_Center)
		[
			SAssignNew( InlineWidget, SInlineEditableTextBlock )
				.Text( this, &SSocketDisplayItem::GetSocketName )
				.OnVerifyTextChanged( this, &SSocketDisplayItem::OnVerifySocketNameChanged )
				.OnTextCommitted( this, &SSocketDisplayItem::OnCommitSocketName )
				.IsSelected( this, &STableRow< TSharedPtr<FString> >::IsSelectedExclusively )
		];

		SocketItem.Pin()->OnRenameRequested.BindSP( InlineWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode );

		STableRow< TSharedPtr<FString> >::ConstructInternal(
			STableRow::FArguments()
				.ShowSelection(true),
			InOwnerTableView
		);	
	}
private:
	/** Returns the socket name */
	FText GetSocketName() const
	{
		return FText::FromName(SocketItem.Pin()->Socket->SocketName);
	}

	bool OnVerifySocketNameChanged( const FText& InNewText, FText& OutErrorMessage )
	{
		bool bVerifyName = true;

		if(InNewText.IsEmpty())
		{
			OutErrorMessage = LOCTEXT( "EmptySocketName_Error", "Sockets must have a name!");
			bVerifyName = false;
		}
		else if(SocketItem.Pin()->Socket->SocketName.ToString() != InNewText.ToString() && SocketManagerPtr.IsValid() && SocketManagerPtr.Pin()->CheckForDuplicateSocket(InNewText.ToString()))
		{
			OutErrorMessage = LOCTEXT( "DuplicateSocket_Error", "Socket name in use!");
			bVerifyName = false;
		}

		return bVerifyName;
	}

	void OnCommitSocketName( const FText& InText, ETextCommit::Type CommitInfo )
	{
		TSharedPtr<SocketListItem> PinnedSocketItem = SocketItem.Pin();
		if (PinnedSocketItem.IsValid())
		{
			UStaticMeshSocket* SelectedSocket = PinnedSocketItem->Socket;
			if (SelectedSocket != NULL)
			{
				FScopedTransaction Transaction( LOCTEXT("SetSocketName", "Set Socket Name") );
				
				UProperty* ChangedProperty = FindField<UProperty>( UStaticMeshSocket::StaticClass(), "SocketName" );
				
				// Pre edit, calls modify on the object
				SelectedSocket->PreEditChange(ChangedProperty);

				// Edit the property itself
				SelectedSocket->SocketName = FName(*InText.ToString());

				// Post edit
				FPropertyChangedEvent PropertyChangedEvent( ChangedProperty );
				SelectedSocket->PostEditChangeProperty(PropertyChangedEvent);
			}
		}
	}

private:
	/** The Socket to display. */
	TWeakPtr< SocketListItem > SocketItem;

	/** Pointer back to the socket manager */
	TWeakPtr< SSocketManager > SocketManagerPtr; 
};

TSharedPtr<ISocketManager> ISocketManager::CreateSocketManager(TSharedPtr<class IStaticMeshEditor> InStaticMeshEditor, FSimpleDelegate InOnSocketSelectionChanged )
{
	TSharedPtr<SSocketManager> SocketManager;
	SAssignNew(SocketManager, SSocketManager)
		.StaticMeshEditorPtr(InStaticMeshEditor)
		.OnSocketSelectionChanged( InOnSocketSelectionChanged );

	TSharedPtr<ISocketManager> ISocket;
	ISocket = StaticCastSharedPtr<ISocketManager>(SocketManager);
	return ISocket;
}

void SSocketManager::Construct(const FArguments& InArgs)
{
	StaticMeshEditorPtr = InArgs._StaticMeshEditorPtr;

	OnSocketSelectionChanged = InArgs._OnSocketSelectionChanged;

	// Register a post undo function which keeps the socket manager list view consistent with the static mesh
	StaticMeshEditorPtr.Pin()->RegisterOnPostUndo( IStaticMeshEditor::FOnPostUndo::CreateSP( this, &SSocketManager::PostUndo ) );

	StaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = false;
	Args.bAllowSearch = true;
	Args.NotifyHook = this;
	Args.bObjectsUseNameArea = true;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	SocketDetailsView = PropertyModule.CreateDetailView(Args);

	WorldSpaceRotation = FVector::ZeroVector;

	this->ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
						.Text( this, &SSocketManager::GetSocketHeaderText )
				]

				+SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SBorder)
						[
							SAssignNew(SocketListView, SListView<TSharedPtr< SocketListItem > >)

							.SelectionMode(ESelectionMode::Single)

							.ListItemsSource( &SocketList )

							// Generates the actual widget for a tree item
							.OnGenerateRow( this, &SSocketManager::MakeWidgetFromOption ) 

							// Find out when the user selects something in the tree
							.OnSelectionChanged( this, &SSocketManager::SocketSelectionChanged_Execute )

							// Allow for some spacing between items with a larger item height.
							.ItemHeight(20.0f)

							.OnContextMenuOpening( this, &SSocketManager::OnContextMenuOpening )
							.OnItemScrolledIntoView( this, &SSocketManager::OnItemScrolledIntoView )

							.HeaderRow
							(
								SNew(SHeaderRow)
								.Visibility(EVisibility::Collapsed)
								+SHeaderRow::Column(TEXT("Socket"))
							)
						]
					]

				+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Right)
						.FillWidth(1.0f)
						.Padding(9.0f, 0.0f, 16.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("CreateSocket", "Create Socket").ToString())
							.OnClicked(this, &SSocketManager::CreateSocket_Execute)
						]
						+SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.FillWidth(1.0f)
							.Padding(16.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
								.Text(LOCTEXT("DeleteSocket", "Delete Socket").ToString())
								.OnClicked(this, &SSocketManager::DeleteSelectedSocket_Execute)
							]
					]

				+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 12.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("WorldSpaceRotation", "World Space Rotation").ToString())
					]

				+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(10.0f, 4.0f, 10.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(0.25f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Pitch", "Pitch").ToString())
						]
						+SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SAssignNew(PitchRotation, SSpinBox<float>)
								.MaxValue(360.f)
								.MinValue(0.f)
								.OnValueChanged(this, &SSocketManager::PitchRotation_ValueChanged)
								.Value(this, &SSocketManager::GetWorldSpacePitchValue)
							]
					]

				+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(10.0f, 4.0f, 10.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(0.25f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Yaw", "Yaw").ToString())
						]
						+SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SAssignNew(YawRotation, SSpinBox<float>)
								.MaxValue(360.f)
								.MinValue(0.f)
								.OnValueChanged(this, &SSocketManager::YawRotation_ValueChanged)
								.Value(this, &SSocketManager::GetWorldSpaceYawValue)
							]
					]

				+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(10.0f, 4.0f, 10.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(0.25f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("Roll", "Roll").ToString())
						]
						+SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SAssignNew(RollRotation, SSpinBox<float>)
								.MaxValue(360.f)
								.MinValue(0.f)
								.OnValueChanged(this, &SSocketManager::RollRotation_ValueChanged)
								.Value(this, &SSocketManager::GetWorldSpaceRollValue)
							]
					]
			]
			+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SocketDetailsView.ToSharedRef()
				]
		];

	RefreshSocketList();

	AddPropertyChangeListenerToSockets();
}

SSocketManager::~SSocketManager()
{
	if( StaticMeshEditorPtr.IsValid() )
	{
		StaticMeshEditorPtr.Pin()->UnregisterOnPostUndo( this );
	}

	RemovePropertyChangeListenerFromSockets();
}

UStaticMeshSocket* SSocketManager::GetSelectedSocket() const
{
	if( SocketListView->GetSelectedItems().Num())
	{
		return SocketListView->GetSelectedItems()[0]->Socket;
	}

	return NULL;
}

void SSocketManager::SetSelectedSocket(UStaticMeshSocket* InSelectedSocket)
{
	for( int32 i=0; i < SocketList.Num(); i++)
	{
		if(SocketList[i]->Socket == InSelectedSocket)
		{
			SocketListView->SetSelection(SocketList[i]);

			SocketListView->RequestListRefresh();

			SocketSelectionChanged(InSelectedSocket);

			return;
		}
	}
}

TSharedRef< ITableRow > SSocketManager::MakeWidgetFromOption( TSharedPtr<SocketListItem> InItem, const TSharedRef< STableViewBase >& OwnerTable )
{
	return SNew( SSocketDisplayItem, OwnerTable )
				.SocketItem(InItem)
				.SocketManagerPtr(SharedThis(this));
}

void SSocketManager::CreateSocket()
{
	if (UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh())
	{
		const FScopedTransaction Transaction( LOCTEXT( "CreateSocket", "Create Socket" ) );

		UStaticMeshSocket* NewSocket = ConstructObject<UStaticMeshSocket>(UStaticMeshSocket::StaticClass(), CurrentStaticMesh);
		check(NewSocket);

		FString SocketNameString = TEXT("Socket");
		FName SocketName = FName(*SocketNameString);

		// Make sure the new name is valid
		int32 Index = 0;
		while (CheckForDuplicateSocket(SocketName.ToString()))
		{
			SocketName = FName(*FString::Printf(TEXT("%s%i"), *SocketNameString, Index));
			++Index;
		}


		NewSocket->SocketName = SocketName;
		NewSocket->SetFlags( RF_Transactional );
		NewSocket->OnPropertyChanged().AddSP( this, &SSocketManager::OnSocketPropertyChanged );

		CurrentStaticMesh->PreEditChange(NULL);
		CurrentStaticMesh->Sockets.Add(NewSocket);
		CurrentStaticMesh->PostEditChange();
		CurrentStaticMesh->MarkPackageDirty();

		TSharedPtr< SocketListItem > SocketItem = MakeShareable( new SocketListItem(NewSocket) );
		SocketList.Add( SocketItem );
		SocketListView->RequestListRefresh();

		SocketListView->SetSelection(SocketItem);
		RequestRenameSelectedSocket();
	}
}

void SSocketManager::DuplicateSelectedSocket()
{
	UStaticMeshSocket* SelectedSocket = GetSelectedSocket();

	if(SelectedSocket)
	{
		const FScopedTransaction Transaction( LOCTEXT( "SocketManager_DuplicateSocket", "Duplicate Socket" ) );

		UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();

		UStaticMeshSocket* NewSocket = DuplicateObject(SelectedSocket, CurrentStaticMesh);

		// Create a unique name for this socket
		NewSocket->SocketName = MakeUniqueObjectName(CurrentStaticMesh, UStaticMeshSocket::StaticClass(), NewSocket->SocketName);

		// Add the new socket to the static mesh
		CurrentStaticMesh->PreEditChange(NULL);
		CurrentStaticMesh->Sockets.Add(NewSocket);
		CurrentStaticMesh->PostEditChange();
		CurrentStaticMesh->MarkPackageDirty();

		RefreshSocketList();

		// Select the duplicated socket
		SetSelectedSocket(NewSocket);
	}
}


void SSocketManager::UpdateStaticMesh()
{
	RefreshSocketList();
}

void SSocketManager::RequestRenameSelectedSocket()
{
	if(SocketListView->GetSelectedItems().Num() == 1)
	{
		TSharedPtr< SocketListItem > SocketItem = SocketListView->GetSelectedItems()[0];
		SocketListView->RequestScrollIntoView(SocketItem);
		DeferredRenameRequest = SocketItem;
	}
}

void SSocketManager::DeleteSelectedSocket()
{
	if(SocketListView->GetSelectedItems().Num())
	{
		const FScopedTransaction Transaction( LOCTEXT( "DeleteSocket", "Delete Socket" ) );

		if (UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh())
		{
			CurrentStaticMesh->PreEditChange(NULL);
			UStaticMeshSocket* SelectedSocket = SocketListView->GetSelectedItems()[ 0 ]->Socket;
			SelectedSocket->OnPropertyChanged().RemoveAll( this );
			CurrentStaticMesh->Sockets.Remove(SelectedSocket);
			CurrentStaticMesh->PostEditChange();

			RefreshSocketList();
		}
	}
}

void SSocketManager::RefreshSocketList()
{
	if( StaticMesh.IsValid() )
	{
		// The static mesh might not be the same one we built the SocketListView with
		// check it here and update it if necessary.
		bool bIsSameStaticMesh = true;
		UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();
		if(CurrentStaticMesh != StaticMesh.Get())
		{
			StaticMesh = CurrentStaticMesh;
			bIsSameStaticMesh = false;
		}
		// Only rebuild the socket list if it differs from the static meshes socket list
		// This is done so that an undo on a socket property doesn't cause the selected
		// socket to be de-selected, thus hiding the socket properties on the detail view.
		// NB: Also force a rebuild if the underlying StaticMesh has been changed.
		if( StaticMesh->Sockets.Num() != SocketList.Num() || !bIsSameStaticMesh )
		{
			SocketList.Empty();
			for(int32 i=0; i < StaticMesh->Sockets.Num(); i++)
			{
				UStaticMeshSocket* Socket = StaticMesh->Sockets[i];
				SocketList.Add( MakeShareable( new SocketListItem(Socket) ) );
			}

			SocketListView->RequestListRefresh();
		}

		// Set the socket on the detail view to keep it in sync with the sockets properties
		if( SocketListView->GetSelectedItems().Num() )
		{
			TArray< UObject* > ObjectList;
			ObjectList.Add( SocketListView->GetSelectedItems()[0]->Socket );
			SocketDetailsView->SetObjects( ObjectList, true );
		}

		StaticMeshEditorPtr.Pin()->RefreshViewport();
	}
	else
	{
		SocketList.Empty();
		SocketListView->ClearSelection();
		SocketListView->RequestListRefresh();
	}
}

bool SSocketManager::CheckForDuplicateSocket(const FString& InSocketName)
{
	for( int32 i=0; i < SocketList.Num(); i++)
	{
		if(SocketList[i]->Socket->SocketName.ToString() == InSocketName)
		{
			return true;
		}
	}

	return false;
}

void SSocketManager::SocketSelectionChanged(UStaticMeshSocket* InSocket)
{
	TArray<UObject*> SelectedObject;

	if(InSocket)
	{
		SelectedObject.Add(InSocket);
	}

	SocketDetailsView->SetObjects(SelectedObject);

	// Notify listeners
	OnSocketSelectionChanged.ExecuteIfBound();
}

void SSocketManager::SocketSelectionChanged_Execute( TSharedPtr<SocketListItem> InItem, ESelectInfo::Type /*SelectInfo*/ )
{
	if(InItem.IsValid())
	{
		SocketSelectionChanged(InItem->Socket);
	}
	else
	{
		SocketSelectionChanged(NULL);
	}
}

FReply SSocketManager::CreateSocket_Execute()
{
	CreateSocket();

	return FReply::Handled();
}

FReply SSocketManager::DeleteSelectedSocket_Execute()
{
	DeleteSelectedSocket();
	return FReply::Handled();
}

FString SSocketManager::GetSocketHeaderText() const
{
	UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();
	return FString::Printf(*LOCTEXT("SocketHeader_Total", "Sockets (%d Total)").ToString(), (CurrentStaticMesh != NULL) ? CurrentStaticMesh->Sockets.Num() : 0);
}

void SSocketManager::SocketName_TextChanged(const FText& InText)
{
	CheckForDuplicateSocket(InText.ToString());
}

void SSocketManager::PitchRotation_ValueChanged(float InValue)
{
	if( InValue != WorldSpaceRotation.X )
	{
		WorldSpaceRotation.X = InValue;

		RotateSocket_WorldSpace();
	}
}

void SSocketManager::YawRotation_ValueChanged(float InValue)
{
	if( InValue != WorldSpaceRotation.Y )
	{
		WorldSpaceRotation.Y = InValue;

		RotateSocket_WorldSpace();
	}
}

void SSocketManager::RollRotation_ValueChanged(float InValue)
{
	if( InValue != WorldSpaceRotation.Z )
	{
		WorldSpaceRotation.Z = InValue;

		RotateSocket_WorldSpace();
	}
}

float SSocketManager::GetWorldSpacePitchValue() const
{
	return WorldSpaceRotation.X;
}

float SSocketManager::GetWorldSpaceYawValue() const
{
	return WorldSpaceRotation.Y;
}

float SSocketManager::GetWorldSpaceRollValue() const
{
	return WorldSpaceRotation.Z;
}

void SSocketManager::RotateSocket_WorldSpace()
{
	if( StaticMeshEditorPtr.IsValid() && SocketListView->GetSelectedItems().Num() )
	{
		UProperty* ChangedProperty = FindField<UProperty>( UStaticMeshSocket::StaticClass(), "RelativeRotation" );
		UStaticMeshSocket* SelectedSocket = SocketListView->GetSelectedItems()[0]->Socket;
		SelectedSocket->PreEditChange(ChangedProperty);
		SelectedSocket->RelativeRotation = FRotator( WorldSpaceRotation.X, WorldSpaceRotation.Y, WorldSpaceRotation.Z );

		FPropertyChangedEvent PropertyChangedEvent( ChangedProperty );
		SelectedSocket->PostEditChangeProperty(PropertyChangedEvent);
	}
}

TSharedPtr<SWidget> SSocketManager::OnContextMenuOpening()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;

	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, StaticMeshEditorPtr.Pin()->GetToolkitCommands());

	{
		MenuBuilder.BeginSection("BasicOperations");
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SSocketManager::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged )
{
	TArray< TSharedPtr< SocketListItem > > SelectedList = SocketListView->GetSelectedItems();
	if(SelectedList.Num())
	{
		if(PropertyThatChanged->GetName() == TEXT("Pitch") || PropertyThatChanged->GetName() == TEXT("Yaw") || PropertyThatChanged->GetName() == TEXT("Roll"))
		{
			const UStaticMeshSocket* Socket = SelectedList[0]->Socket;
			WorldSpaceRotation.Set( Socket->RelativeRotation.Pitch, Socket->RelativeRotation.Yaw, Socket->RelativeRotation.Roll );
		}
	}
}

void SSocketManager::AddPropertyChangeListenerToSockets()
{
	if (UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh())
	{
		for (int32 i = 0; i < CurrentStaticMesh->Sockets.Num(); ++i)
		{
			CurrentStaticMesh->Sockets[i]->OnPropertyChanged().AddSP(this, &SSocketManager::OnSocketPropertyChanged);
		}
	}
}

void SSocketManager::RemovePropertyChangeListenerFromSockets()
{
	if( StaticMeshEditorPtr.IsValid() )
	{
		UStaticMesh* CurrentStaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();
		if (CurrentStaticMesh)
		{
			for (int32 i = 0; i < CurrentStaticMesh->Sockets.Num(); ++i)
			{
				CurrentStaticMesh->Sockets[i]->OnPropertyChanged().RemoveAll(this);
			}
		}
	}
}

void SSocketManager::OnSocketPropertyChanged( const UStaticMeshSocket* Socket, const UProperty* ChangedProperty )
{
	if( ChangedProperty->GetName() == TEXT( "RelativeRotation" ) )
	{
		const UStaticMeshSocket* SelectedSocket = GetSelectedSocket();

		if( Socket == SelectedSocket )
		{
			WorldSpaceRotation.Set( Socket->RelativeRotation.Pitch, Socket->RelativeRotation.Yaw, Socket->RelativeRotation.Roll );
		}
	}
}

void SSocketManager::PostUndo()
{
	RefreshSocketList();
}

void SSocketManager::OnItemScrolledIntoView( TSharedPtr<SocketListItem> InItem, const TSharedPtr<ITableRow>& InWidget)
{
	if( DeferredRenameRequest.IsValid() )
	{
		DeferredRenameRequest.Pin()->OnRenameRequested.ExecuteIfBound();
		DeferredRenameRequest.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

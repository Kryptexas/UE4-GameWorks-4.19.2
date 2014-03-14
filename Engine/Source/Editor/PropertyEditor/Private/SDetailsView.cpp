// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PropertyEditorPrivatePCH.h"
#include "AssetSelection.h"
#include "PropertyNode.h"
#include "ItemPropertyNode.h"
#include "CategoryPropertyNode.h"
#include "ObjectPropertyNode.h"
#include "ScopedTransaction.h"
#include "AssetThumbnail.h"
#include "SDetailNameArea.h"
#include "IPropertyUtilities.h"
#include "PropertyEditorHelpers.h"
#include "PropertyEditor.h"
#include "PropertyDetailsUtilities.h"
#include "SPropertyEditorEditInline.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "SDetailsView"

SDetailsView::~SDetailsView()
{
	SaveExpandedItems();

	if( ThumbnailPool.IsValid() )
	{
		ThumbnailPool->ReleaseResources();
	}
};

/**
 * Constructs the widget
 *
 * @param InArgs   Declaration from which to construct this widget.              
 */
void SDetailsView::Construct(const FArguments& InArgs)
{
	DetailsViewArgs = InArgs._DetailsViewArgs;
	bHasActiveFilter = false;
	bIsLocked = false;
	bViewingClassDefaultObject = false;
	bHasOpenColorPicker = false;

	// Create the root property now
	RootPropertyNode = MakeShareable( new FObjectPropertyNode );
		
	PropertyUtilities = MakeShareable( new FPropertyDetailsUtilities( *this ) );
	
	ColumnWidth = 0.65f;
	ColumnSizeData.LeftColumnWidth = TAttribute<float>( this, &SDetailsView::OnGetLeftColumnWidth );
	ColumnSizeData.RightColumnWidth = TAttribute<float>( this, &SDetailsView::OnGetRightColumnWidth );
	ColumnSizeData.OnWidthChanged = SSplitter::FOnSlotResized::CreateSP( this, &SDetailsView::OnSetColumnWidth );

	TSharedRef<SScrollBar> ExternalScrollbar = 
		SNew(SScrollBar)
		.AlwaysShowScrollbar( true );

		FMenuBuilder DetailViewOptions( true, NULL );

		FUIAction ShowOnlyModifiedAction( 
			FExecuteAction::CreateSP( this, &SDetailsView::OnShowOnlyModifiedClicked ),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP( this, &SDetailsView::IsShowOnlyModifiedChecked )
		);

		if (DetailsViewArgs.bShowModifiedPropertiesOption)
		{
			DetailViewOptions.AddMenuEntry( 
				LOCTEXT("ShowOnlyModified", "Show Only Modified Properties"),
				LOCTEXT("ShowOnlyModified_ToolTip", "Displays only properties which have been changed from their default"),
				FSlateIcon(),
				ShowOnlyModifiedAction,
				NAME_None,
				EUserInterfaceActionType::ToggleButton 
			);
		}

		FUIAction ShowAllAdvancedAction( 
			FExecuteAction::CreateSP( this, &SDetailsView::OnShowAllAdvancedClicked ),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP( this, &SDetailsView::IsShowAllAdvancedChecked )
		);

		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowAllAdvanced", "Show All Advanced Details"),
			LOCTEXT("ShowAllAdvanced_ToolTip", "Shows all advanced detail sections in each category"),
			FSlateIcon(),
			ShowAllAdvancedAction,
			NAME_None,
			EUserInterfaceActionType::ToggleButton 
			);

		DetailViewOptions.AddMenuEntry(
			LOCTEXT("CollapseAll", "Collapse All Categories"),
			LOCTEXT("CollapseAll_ToolTip", "Collapses all root level categories"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDetailsView::SetRootExpansionStates, /*bExpanded=*/false, /*bRecurse=*/false )));
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ExpandAll", "Expand All Categories"),
			LOCTEXT("ExpandAll_ToolTip", "Expands all root level categories"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDetailsView::SetRootExpansionStates, /*bExpanded=*/true, /*bRecurse=*/false )));

	TSharedRef<SHorizontalBox> FilterRow = SNew( SHorizontalBox )
		.Visibility( this, &SDetailsView::GetFilterBoxVisibility )
		+SHorizontalBox::Slot()
		.FillWidth( 1 )
		.VAlign( VAlign_Center )
		[
			// Create the search box
			SAssignNew( SearchBox, SSearchBox )
			.OnTextChanged( this, &SDetailsView::OnFilterTextChanged  )
		]
		+SHorizontalBox::Slot()
		.Padding( 4.0f, 0.0f, 0.0f, 0.0f )
		.AutoWidth()
		[
			// Create the search box
			SNew( SButton )
			.OnClicked( this, &SDetailsView::OnOpenRawPropertyEditorClicked )
			.ToolTipText( LOCTEXT("RawPropertyEditorButtonLabel", "Open Selection in Property Matrix") )
			[
				SNew( SImage )
				.Image( FEditorStyle::GetBrush("DetailsView.EditRawProperties") )
			]
		];

	if (DetailsViewArgs.bShowOptions)
	{
		FilterRow->AddSlot()
			.HAlign(HAlign_Right)
			.AutoWidth()
			[
				SNew( SComboButton )
				.ContentPadding(0)
				.ForegroundColor( FSlateColor::UseForeground() )
				.ButtonStyle( FEditorStyle::Get(), "ToggleButton" )
				.MenuContent()
				[
					DetailViewOptions.MakeWidget()
				]
				.ButtonContent()
				[
					SNew(SImage)
					.Image( FEditorStyle::GetBrush("GenericViewButton") )
				]
			];
	}


	ChildSlot
	[
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0.0f, 0.0f, 0.0f, 4.0f )
		[
			// Create the name area which does not change when selection changes
			SAssignNew( NameArea, SDetailNameArea, &SelectedObjects )
			// the name area is only for actors
			.Visibility( this, &SDetailsView::GetActorNameAreaVisibility  )
			.OnLockButtonClicked( this, &SDetailsView::OnLockButtonClicked )
			.IsLocked( this, &SDetailsView::IsLocked )
			.ShowLockButton( DetailsViewArgs.bLockable )
			.ShowActorLabel( DetailsViewArgs.bShowActorLabel )
			// only show the selection tip if we're not selecting objects
			.SelectionTip( !DetailsViewArgs.bHideSelectionTip )
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0.0f, 0.0f, 0.0f, 2.0f )
		[
			FilterRow
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(0)
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			[
				ConstructTreeView( ExternalScrollbar )
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( 16.0f )
				[
					ExternalScrollbar
				]
			]
		]
	];
}

TSharedRef<SDetailTree> SDetailsView::ConstructTreeView( TSharedRef<SScrollBar>& ScrollBar )
{
	check( !DetailTree.IsValid() || DetailTree.IsUnique() );

	return 
	SAssignNew( DetailTree, SDetailTree )
	.Visibility( this, &SDetailsView::GetTreeVisibility )
	.TreeItemsSource( &RootTreeNodes )
	.OnGetChildren( this, &SDetailsView::OnGetChildrenForDetailTree )
	.OnSetExpansionRecursive( this, &SDetailsView::SetNodeExpansionStateRecursive )
	.OnGenerateRow( this, &SDetailsView::OnGenerateRowForDetailTree )	
	.OnExpansionChanged( this, &SDetailsView::OnItemExpansionChanged )
	.SelectionMode( ESelectionMode::None )
	.ExternalScrollbar( ScrollBar );
}

void SDetailsView::OnGetChildrenForDetailTree( TSharedRef<IDetailTreeNode> InTreeNode, TArray< TSharedRef<IDetailTreeNode> >& OutChildren )
{
	InTreeNode->GetChildren( OutChildren );
}


TSharedRef<ITableRow> SDetailsView::OnGenerateRowForDetailTree( TSharedRef<IDetailTreeNode> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable )
{
	return InTreeNode->GenerateNodeWidget( OwnerTable, ColumnSizeData, PropertyUtilities.ToSharedRef() );
}

void SDetailsView::SetRootExpansionStates( const bool bExpand, const bool bRecurse )
{
	for (auto Iter = RootTreeNodes.CreateIterator(); Iter; ++Iter)
	{
		SetNodeExpansionState(*Iter, bExpand, bRecurse );
	}
}

void SDetailsView::SetNodeExpansionState( TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded, bool bRecursive )
{
	TArray< TSharedRef<IDetailTreeNode> > Children;
	InTreeNode->GetChildren( Children );

	if( Children.Num() )
	{
		RequestItemExpanded( InTreeNode, bIsItemExpanded );
		InTreeNode->OnItemExpansionChanged( bIsItemExpanded );

		if( bRecursive )
		{
			for( int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex )
			{
				TSharedRef<IDetailTreeNode> Child = Children[ChildIndex];

				SetNodeExpansionState( Child, bIsItemExpanded, bRecursive );
			}
		}
	}
}

void SDetailsView::SetNodeExpansionStateRecursive( TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded )
{
	SetNodeExpansionState( InTreeNode, bIsItemExpanded, true );
}

void SDetailsView::OnItemExpansionChanged( TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded )
{
	SetNodeExpansionState( InTreeNode, bIsItemExpanded, false );
}


void SDetailsView::SetColorPropertyFromColorPicker(FLinearColor NewColor)
{
	const TSharedPtr< FPropertyNode > PinnedColorPropertyNode = ColorPropertyNode.Pin();
	if( ensure(PinnedColorPropertyNode.IsValid()) )
	{
		UProperty* Property = PinnedColorPropertyNode->GetProperty();
		check(Property);

		FObjectPropertyNode* ObjectNode = PinnedColorPropertyNode->FindObjectItemParent();

		if( ObjectNode && ObjectNode->GetNumObjects() )
		{
			FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "SetColorProperty", "Set Color Property") );

			PinnedColorPropertyNode->NotifyPreChange(Property, GetNotifyHook());

			FPropertyChangedEvent ChangeEvent(Property, false, EPropertyChangeType::ValueSet);
			PinnedColorPropertyNode->NotifyPostChange( ChangeEvent, GetNotifyHook() );
		}
	}
}

FReply SDetailsView::OnOpenRawPropertyEditorClicked()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
	PropertyEditorModule.CreatePropertyEditorToolkit( EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), SelectedObjects );

	return FReply::Handled();
}

FReply SDetailsView::OnLockButtonClicked()
{
	bIsLocked = !bIsLocked;
	return FReply::Handled();
}

void SDetailsView::OnShowOnlyModifiedClicked()
{
	CurrentFilter.bShowOnlyModifiedProperties = !CurrentFilter.bShowOnlyModifiedProperties;

	UpdateFilteredDetails();
}

void SDetailsView::OnShowAllAdvancedClicked()
{
	CurrentFilter.bShowAllAdvanced = !CurrentFilter.bShowAllAdvanced;

	UpdateFilteredDetails();
}

/** Called when the filter text changes.  This filters specific property nodes out of view */
void SDetailsView::OnFilterTextChanged( const FText& InFilterText )
{
	FString InFilterString = InFilterText.ToString();
	InFilterString.Trim().TrimTrailing();

	// Was the filter just cleared
	bool bFilterCleared = InFilterString.Len() == 0 && CurrentFilter.FilterStrings.Num() > 0;

	FilterView( InFilterString );

}

EVisibility SDetailsView::GetActorNameAreaVisibility() const
{
	const bool bVisible = !DetailsViewArgs.bHideActorNameArea && !bViewingClassDefaultObject;
	return bVisible ? EVisibility::Visible : EVisibility::Collapsed; 
}

void SDetailsView::HideFilterArea(bool bHide)
{
	DetailsViewArgs.bAllowSearch = !bHide;
}

EVisibility SDetailsView::GetFilterBoxVisibility() const
{
	// Visible if we allow search and we have anything to search otherwise collapsed so it doesn't take up room
	return DetailsViewArgs.bAllowSearch && RootPropertyNode->GetNumObjects() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SDetailsView::GetTreeVisibility() const
{
	return DetailLayout.IsValid() && DetailLayout->HasDetails() ? EVisibility::Visible : EVisibility::Collapsed;
}

/** Returns the image used for the icon on the filter button */ 
const FSlateBrush* SDetailsView::OnGetFilterButtonImageResource() const
{
	if( bHasActiveFilter )
	{
		return FEditorStyle::GetBrush(TEXT("PropertyWindow.FilterCancel"));
	}
	else
	{
		return FEditorStyle::GetBrush(TEXT("PropertyWindow.FilterSearch"));
	}
}

/**
 * Determines whether or not a property should be visible in the default generated detail layout
 *
 * @param PropertyNode	The property node to check
 * @param ParentNode	The parent property node to check
 * @return true if the property should be visible
 */
static bool IsVisibleStandaloneProperty( const FPropertyNode& PropertyNode, const FPropertyNode& ParentNode )
{
	const UProperty* Property = PropertyNode.GetProperty();
	const UArrayProperty* ParentArrayProperty = Cast<const UArrayProperty>( ParentNode.GetProperty() );
	bool bIsVisibleStandalone = false;
	if( Property )
	{
		if( Property->IsA( UObjectPropertyBase::StaticClass() ) )
		{
			// Do not add this child node to the current map if its a single object property in a category (serves no purpose for UI)
			bIsVisibleStandalone = !ParentArrayProperty && (PropertyNode.GetNumChildNodes() == 0 || PropertyNode.GetNumChildNodes() > 1) ;
		}
		else if( Property->IsA( UArrayProperty::StaticClass() ) || Property->ArrayDim > 1 && PropertyNode.GetArrayIndex() == INDEX_NONE )
		{
			// Base array properties are always visible
			bIsVisibleStandalone = true;
		}
		else
		{
			bIsVisibleStandalone = true;
		}
		
	}

	return bIsVisibleStandalone;
}

void SDetailsView::UpdatePropertyMapRecursive( FPropertyNode& InNode, FDetailLayoutBuilderImpl& InDetailLayout, FName CurCategory, FObjectPropertyNode* CurObjectNode, const FCustomStructLayoutMap& StructLayoutMap )
{
	UProperty* ParentProperty = InNode.GetProperty();
	UStructProperty* ParentStructProp = Cast<UStructProperty>(ParentProperty);
	
	for( int32 ChildIndex = 0; ChildIndex < InNode.GetNumChildNodes(); ++ChildIndex )
	{
		TSharedPtr<FPropertyNode> ChildNodePtr = InNode.GetChildNode(ChildIndex);
		FPropertyNode& ChildNode = *ChildNodePtr;
		UProperty* Property = ChildNode.GetProperty();

		{
			FObjectPropertyNode* ObjNode = ChildNode.AsObjectNode();
			FCategoryPropertyNode* CategoryNode = ChildNode.AsCategoryNode();
			if( ObjNode )
			{
				// Currently object property nodes do not provide any useful information other than being a container for its children.  We do not draw anything for them.
				// When we encounter object property nodes, add their children instead of adding them to the tree.
				UpdatePropertyMapRecursive( ChildNode, InDetailLayout, CurCategory, ObjNode, StructLayoutMap );
			}
			else if( CategoryNode )
			{
				// For category nodes, we just set the current category and recurse through the children
				UpdatePropertyMapRecursive( ChildNode, InDetailLayout, CategoryNode->GetCategoryName(), CurObjectNode, StructLayoutMap );
			}
			else
			{
				// Whether or not the property can be visible in the default detail layout
				bool bVisibleByDefault = IsVisibleStandaloneProperty( ChildNode, InNode );

				// Whether or not the property is a struct
				UStructProperty* StructProperty = Cast<UStructProperty>( Property );

				bool bIsStruct = StructProperty != NULL;

				static FName ShowOnlyInners("ShowOnlyInnerProperties");

				bool bIsChildOfCustomizedStruct = false;
				bool bIsCustomizedStruct = false;
				if( StructProperty && StructProperty->Struct )
				{
					bIsCustomizedStruct = StructLayoutMap.Contains( StructProperty->Struct->GetFName() );
				}

				if( ParentStructProp && ParentStructProp->Struct )
				{
					bIsChildOfCustomizedStruct = StructLayoutMap.Contains( ParentStructProp->Struct->GetFName() );
				}

				// Whether or not to push out struct properties to their own categories or show them inside an expandable struct 
				bool bPushOutStructProps = bIsStruct && !bIsCustomizedStruct && !ParentStructProp && Property->HasMetaData(ShowOnlyInners);

				// Is the property edit inline new 
				const bool bIsEditInlineNew = SPropertyEditorEditInline::Supports( &ChildNode, ChildNode.GetArrayIndex() );

				// Is this a property of an array
				bool bIsChildOfArray = PropertyEditorHelpers::IsChildOfArray( ChildNode );

				// Edit inline new properties should be visible by default
				bVisibleByDefault |= bIsEditInlineNew;

				// Children of arrays are not visible directly,
				bVisibleByDefault &= !bIsChildOfArray;

				// See if the user requested specific property visibility 
				const bool bIsUserVisible = IsPropertyVisible.IsBound() ? IsPropertyVisible.Execute( Property ) : true;

				// Inners of customized in structs should not be taken into consideration for customizing.  They are not designed to be individually customized when their parent is already customized
				if( !bIsChildOfCustomizedStruct )
				{
					// Add any object classes with properties so we can ask them for custom property layouts later
					ClassesWithProperties.Add( Property->GetOwnerStruct() );
				}

				// If there is no outer object then the class is the object root and there is only one instance
				FName InstanceName = NAME_None;
				if( CurObjectNode && CurObjectNode->GetParentNode() )
				{
					InstanceName = CurObjectNode->GetParentNode()->GetProperty()->GetFName();
				}
				else if( ParentStructProp )
				{
					InstanceName = ParentStructProp->GetFName();
				}

				// Do not add children of customized in struct properties or arrays
				if( !bIsChildOfCustomizedStruct && !bIsChildOfArray )
				{
					// Get the class property map
					FClassInstanceToPropertyMap& ClassInstanceMap = ClassToPropertyMap.FindOrAdd( Property->GetOwnerStruct()->GetFName() );

					FPropertyNodeMap& PropertyNodeMap = ClassInstanceMap.FindOrAdd( InstanceName );

					if( !PropertyNodeMap.ParentObjectProperty )
					{
						PropertyNodeMap.ParentObjectProperty = CurObjectNode;
					}
					else
					{
						ensure( PropertyNodeMap.ParentObjectProperty == CurObjectNode );
					}

					checkSlow( !PropertyNodeMap.Contains( Property->GetFName() ) );

					PropertyNodeMap.Add( Property->GetFName(), ChildNodePtr );
				}

				if( bVisibleByDefault && bIsUserVisible && !bPushOutStructProps )
				{
					FName CategoryName = CurCategory;

					// For properties inside a struct, add them to their own category unless they just take the name of the parent struct.  
					// In that case push them to the parent category
					FName PropertyCatagoryName = FObjectEditorUtils::GetCategoryFName(Property);
					if( !ParentStructProp || (PropertyCatagoryName != ParentStructProp->Struct->GetFName()) )
					{
						CategoryName = PropertyCatagoryName;
					}

					// Add a property to the default category
					FDetailCategoryImpl& CategoryImpl = InDetailLayout.DefaultCategory( CategoryName );
					CategoryImpl.AddPropertyNode( ChildNodePtr.ToSharedRef(), InstanceName );
				}

				bool bRecurseIntoChildren = 
						!bIsChildOfCustomizedStruct // Don't recurse into built in struct children, we already know what they are and how to display them
					&&  !bIsCustomizedStruct // Don't recurse into customized structs
					&&	!bIsChildOfArray // Do not recurse into arrays, the children are drawn by the array property parent
					&&	!bIsEditInlineNew // Edit inline new children are not supported for customization yet
					&&	bIsUserVisible // Properties must be allowed to be visible by a user if they are not then their children are not visible either
					&&  (!bIsStruct || bPushOutStructProps); //  Only recurse into struct properties if they are going to be displayed as standalone properties in categories instead of inside an expandable area inside a category

				if( bRecurseIntoChildren )
				{
					// Built in struct properties or children of arras 
					UpdatePropertyMapRecursive( ChildNode, InDetailLayout, CurCategory, CurObjectNode, StructLayoutMap );
				}
			}		
		}
	}
}

void SDetailsView::UpdatePropertyMap()
{
	check( RootPropertyNode.IsValid() );

	// Reset everything
	ClassToPropertyMap.Empty();
	ClassesWithProperties.Empty();

	// We need to be able to create a new detail layout and properly clean up the old one in the process
	check( !DetailLayout.IsValid() || DetailLayout.IsUnique() );

	RootTreeNodes.Empty();

	DetailLayout = MakeShareable( new FDetailLayoutBuilderImpl( ClassToPropertyMap, PropertyUtilities.ToSharedRef(), SharedThis( this ) ) );
	
	FPropertyEditorModule& ParentPlugin = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Get the registered classes that customize details
	FCustomStructLayoutMap& GlobalStructLayoutMap = ParentPlugin.StructTypeToLayoutMap;

	// Currently object property nodes do not provide any useful information other than being a container for its children.  We do not draw anything for them.
	// When we encounter object property nodes, add their children instead of adding them to the tree.
	UpdatePropertyMapRecursive( *RootPropertyNode, *DetailLayout, NAME_None, RootPropertyNode.Get(), GlobalStructLayoutMap );

	// Ask for custom detail layouts
	QueryCustomDetailLayout( *DetailLayout );

	DetailLayout->GenerateDetailLayout();

	UpdateFilteredDetails();
}

void SDetailsView::ForceRefresh()
{
	TArray< TWeakObjectPtr< UObject > > NewObjectList;

	// Simply re-add the same existing objects to cause a refresh
	for ( TPropObjectIterator Itor( RootPropertyNode->ObjectIterator() ); Itor; ++Itor )
	{
		TWeakObjectPtr<UObject> Object = *Itor;
		if( Object.IsValid() )
		{
			NewObjectList.Add( Object.Get() );
		}
	}

	SetObjectArrayPrivate( NewObjectList );
}


void SDetailsView::SetObjects( const TArray<UObject*>& InObjects, bool bForceRefresh/* = false*/ )
{
	if( !IsLocked() )
	{
		TArray< TWeakObjectPtr< UObject > > ObjectWeakPtrs;
		
		for( auto ObjectIter = InObjects.CreateConstIterator(); ObjectIter; ++ObjectIter )
		{
			ObjectWeakPtrs.Add( *ObjectIter );
		}

 		if( bForceRefresh || ShouldSetNewObjects( ObjectWeakPtrs ) )
		{
			SetObjectArrayPrivate( ObjectWeakPtrs );
		}
	}
}

void SDetailsView::SetObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects, bool bForceRefresh/* = false*/ )
{
	if( !IsLocked() )
	{
		if( bForceRefresh || ShouldSetNewObjects( InObjects ) )
		{
			SetObjectArrayPrivate( InObjects );
		}
	}
}

void SDetailsView::SetObject( UObject* InObject, bool bForceRefresh )
{
	TArray< TWeakObjectPtr< UObject > > ObjectWeakPtrs;
	ObjectWeakPtrs.Add( InObject );

	SetObjects( ObjectWeakPtrs, bForceRefresh );
}


bool SDetailsView::ShouldSetNewObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects ) const
{
	bool bShouldSetObjects = false;
	if( InObjects.Num() != RootPropertyNode->GetNumObjects() )
	{
		// If the object arrys differ in size then at least one object is different so we must reset
		bShouldSetObjects = true;
	}
	else
	{
		// Check to see if the objects passed in are different. If not we do not need to set anything
		TSet< TWeakObjectPtr< UObject > > NewObjects;
		NewObjects.Append( InObjects );
		for ( TPropObjectIterator Itor( RootPropertyNode->ObjectIterator() ); Itor; ++Itor )
		{
			TWeakObjectPtr<UObject> Object = *Itor;
			if( Object.IsValid() && !NewObjects.Contains( Object ) )
			{
				// An existing object is not in the list of new objects to set
				bShouldSetObjects = true;
				break;
			}
			else if( !Object.IsValid() )
			{
				// An existing object is invalid
				bShouldSetObjects = true;
				break;
			}
		}
	}

	return bShouldSetObjects;
}

void SDetailsView::SetObjectArrayPrivate( const TArray< TWeakObjectPtr< UObject > >& InObjects )
{
	double StartTime = FPlatformTime::Seconds();

	PreSetObject();

	check( RootPropertyNode.IsValid() );

	// Selected actors for building SelectedActorInfo
	TArray<AActor*> SelectedRawActors;

	bViewingClassDefaultObject = false;
	bool bOwnedByLockedLevel = false;
	for( int32 ObjectIndex = 0 ; ObjectIndex < InObjects.Num() ; ++ObjectIndex )
	{
		TWeakObjectPtr< UObject > Object = InObjects[ObjectIndex];

		if( Object.IsValid() )
		{
			bViewingClassDefaultObject |= Object->HasAnyFlags( RF_ClassDefaultObject );

			RootPropertyNode->AddObject( Object.Get() );
			SelectedObjects.Add( Object );
			AActor* Actor = Cast<AActor>( Object.Get() );
			if( Actor )
			{
				SelectedActors.Add( Actor );
				SelectedRawActors.Add( Actor );
			}
		}
	}

	if( InObjects.Num() == 0 )
	{
		// Unlock the view automatically if we are viewing nothing
		bIsLocked = false;
	}

	// Selection changed, refresh the detail area
	if ( DetailsViewArgs.bObjectsUseNameArea )
	{
		NameArea->Refresh( SelectedObjects );
	}
	else
	{
		NameArea->Refresh( SelectedActors );
	}
	
	// When selection changes rebuild information about the selection
	SelectedActorInfo = AssetSelectionUtils::BuildSelectedActorInfo( SelectedRawActors );

	// @todo Slate Property Window
	//SetFlags(EPropertyWindowFlags::ReadOnly, bOwnedByLockedLevel);


	PostSetObject();

	// Set the title of the window based on the objects we are viewing
	// Or call the delegate for handling when the title changed
	FString Title;

	if( !RootPropertyNode->GetObjectBaseClass() )
	{
		Title = NSLOCTEXT("PropertyView", "NothingSelectedTitle", "Nothing selected").ToString();
	}
	else if( RootPropertyNode->GetNumObjects() == 1 )
	{
		// if the object is the default metaobject for a UClass, use the UClass's name instead
		const UObject* Object = RootPropertyNode->ObjectConstIterator()->Get();
		FString ObjectName = Object->GetName();
		if ( Object->GetClass()->GetDefaultObject() == Object )
		{
			ObjectName = Object->GetClass()->GetName();
		}
		else
		{
			// Is this an actor?  If so, it might have a friendly name to display
			const AActor* Actor = Cast<const  AActor >( Object );
			if( Actor != NULL )
			{
				// Use the friendly label for this actor
				ObjectName = Actor->GetActorLabel();
			}
		}

		Title = ObjectName;
	}
	else
	{
		Title = FString::Printf( *NSLOCTEXT("PropertyView", "MultipleSelected", "%s (%i selected)").ToString(), *RootPropertyNode->GetObjectBaseClass()->GetName(), RootPropertyNode->GetNumObjects() );
	}

	OnObjectArrayChanged.ExecuteIfBound(Title, InObjects);

	double ElapsedTime = FPlatformTime::Seconds() - StartTime;
}

void SDetailsView::EnqueueDeferredAction( FSimpleDelegate& DeferredAction )
{
	DeferredActions.AddUnique( DeferredAction );
}


void SDetailsView::ReplaceObjects( const TMap<UObject*, UObject*>& OldToNewObjectMap )
{
	TArray< TWeakObjectPtr< UObject > > NewObjectList;
	bool bObjectsReplaced = false;

	TArray< FObjectPropertyNode* > ObjectNodes;
	PropertyEditorHelpers::CollectObjectNodes( RootPropertyNode, ObjectNodes );

	for( int32 ObjectNodeIndex = 0; ObjectNodeIndex < ObjectNodes.Num(); ++ObjectNodeIndex )
	{
		FObjectPropertyNode* CurrentNode = ObjectNodes[ObjectNodeIndex];

		// Scan all objects and look for objects which need to be replaced
		for ( TPropObjectIterator Itor( CurrentNode->ObjectIterator() ); Itor; ++Itor )
		{
			UObject* Replacement = OldToNewObjectMap.FindRef( Itor->Get() );
			if( Replacement && Replacement->GetClass() == Itor->Get()->GetClass() )
			{
				bObjectsReplaced = true;
				if( CurrentNode == RootPropertyNode.Get() )
				{
					// Note: only root objects count for the new object list. Sub-Objects (i.e components count as needing to be replaced but they don't belong in the top level object list
					NewObjectList.Add( Replacement );
				}
			}
			else if( CurrentNode == RootPropertyNode.Get() )
			{
				// Note: only root objects count for the new object list. Sub-Objects (i.e components count as needing to be replaced but they don't belong in the top level object list
				NewObjectList.Add( Itor->Get() );
			}
		}
	}


	if( bObjectsReplaced )
	{
		SetObjectArrayPrivate( NewObjectList );
	}

}

void SDetailsView::RemoveDeletedObjects( const TArray<UObject*>& DeletedObjects )
{
	TArray< TWeakObjectPtr< UObject > > NewObjectList;
	bool bObjectsRemoved = false;

	// Scan all objects and look for objects which need to be replaced
	for ( TPropObjectIterator Itor( RootPropertyNode->ObjectIterator() ); Itor; ++Itor )
	{
		if( DeletedObjects.Contains( Itor->Get() ) )
		{
			// An object we had needs to be removed
			bObjectsRemoved = true;
		}
		else
		{
			// If the deleted object list does not contain the current object, its ok to keep it in the list
			NewObjectList.Add( Itor->Get() );
		}
	}

	// if any objects were replaced update the observed objects
	if( bObjectsRemoved )
	{
		SetObjectArrayPrivate( NewObjectList );
	}
}

/**
 * Removes actors from the property nodes object array which are no longer available
 * 
 * @param ValidActors	The list of actors which are still valid
 */
void SDetailsView::RemoveInvalidActors( const TSet<AActor*>& ValidActors )
{
	TArray< TWeakObjectPtr< UObject > > ResetArray;

	bool bAllFound = true;
	for ( TPropObjectIterator Itor( RootPropertyNode->ObjectIterator() ); Itor; ++Itor )
	{
		AActor* Actor = Cast<AActor>( Itor->Get() );

		bool bFound = ValidActors.Contains( Actor );

		// If the selected actor no longer exists, remove it from the property window.
		if( bFound )
		{
			ResetArray.Add(Actor);
		}
		else
		{
			bAllFound = false;
		}
	}

	if ( !bAllFound ) 
	{
		SetObjectArrayPrivate( ResetArray );
	}
}

/**
 * Recursively gets expanded items for a node
 * 
 * @param InPropertyNode			The node to get expanded items from
 * @param OutExpandedItems	List of expanded items that were found
 */
void GetExpandedItems( TSharedPtr<FPropertyNode> InPropertyNode, TArray<FString>& OutExpandedItems )
{
	if( InPropertyNode->HasNodeFlags(EPropertyNodeFlags::Expanded) )
	{
		const bool bWithArrayIndex = true;
		FString Path;
		Path.Empty(128);
		InPropertyNode->GetQualifiedName(Path, bWithArrayIndex);

		OutExpandedItems.Add( Path );
	}

	for( int32 ChildIndex = 0; ChildIndex < InPropertyNode->GetNumChildNodes(); ++ChildIndex )
	{
		GetExpandedItems( InPropertyNode->GetChildNode(ChildIndex), OutExpandedItems );
	}

}

/**
 * Recursively sets expanded items for a node
 * 
 * @param InNode			The node to set expanded items on
 * @param OutExpandedItems	List of expanded items to set
 */
void SetExpandedItems( TSharedPtr<FPropertyNode> InPropertyNode, const TArray<FString>& InExpandedItems )
{
	if( InExpandedItems.Num() > 0 )
	{
		const bool bWithArrayIndex = true;
		FString Path;
		Path.Empty(128);
		InPropertyNode->GetQualifiedName(Path, bWithArrayIndex);

		for (int32 ItemIndex = 0; ItemIndex < InExpandedItems.Num(); ++ItemIndex)
		{
			if ( InExpandedItems[ItemIndex] == Path )
			{
				InPropertyNode->SetNodeFlags( EPropertyNodeFlags::Expanded, true );
				break;
			}
		}

		for( int32 NodeIndex = 0; NodeIndex < InPropertyNode->GetNumChildNodes(); ++NodeIndex )
		{
			SetExpandedItems( InPropertyNode->GetChildNode( NodeIndex ), InExpandedItems );
		}
	}
}

void SDetailsView::SaveExpandedItems()
{
	UClass* BestBaseClass = RootPropertyNode->GetObjectBaseClass();

	TArray<FString> ExpandedPropertyItems;
	GetExpandedItems( RootPropertyNode, ExpandedPropertyItems );

	TArray<FString> ExpandedCustomItems = ExpandedDetailNodes.Array();

	// Expanded custom items may have spaces but SetSingleLineArray doesnt support spaces (treats it as another element in the array)
	// Append a '|' after each element instead
	FString ExpandedCustomItemsString;
	for( auto It = ExpandedDetailNodes.CreateConstIterator(); It; ++It )
	{
		ExpandedCustomItemsString += *It;
		ExpandedCustomItemsString += TEXT(",");
	}

	//while a valid class, and we're either the same as the base class (for multiple actors being selected and base class is AActor) OR we're not down to AActor yet)
	for( UClass* Class = BestBaseClass; Class && ((BestBaseClass == Class) || (Class!=AActor::StaticClass())); Class = Class->GetSuperClass() )
	{
		if( RootPropertyNode->GetNumChildNodes() > 0 && ExpandedPropertyItems.Num() > 0 )
		{
			GConfig->SetSingleLineArray(TEXT("DetailPropertyExpansion"), *Class->GetName(), ExpandedPropertyItems, GEditorUserSettingsIni);
		}
	}

	if( DetailLayout.IsValid() && BestBaseClass && !ExpandedCustomItemsString.IsEmpty() )
	{
		GConfig->SetString(TEXT("DetailCustomWidgetExpansion"), *BestBaseClass->GetName(), *ExpandedCustomItemsString, GEditorUserSettingsIni);
	}
}

void SDetailsView::RestoreExpandedItems( TSharedPtr<FPropertyNode> InitialStartNode )
{
	TSharedPtr<FPropertyNode> StartNode = InitialStartNode;
	if( !StartNode.IsValid() )
	{
		StartNode = RootPropertyNode;
	}

	ExpandedDetailNodes.Empty();

	TArray<FString> ExpandedPropertyItems;
	FString ExpandedCustomItems;
	UClass* BestBaseClass = RootPropertyNode->GetObjectBaseClass();
	//while a valid class, and we're either the same as the base class (for multiple actors being selected and base class is AActor) OR we're not down to AActor yet)
	for( UClass* Class = BestBaseClass; Class && ((BestBaseClass == Class) || (Class!=AActor::StaticClass())); Class = Class->GetSuperClass() )
	{
		GConfig->GetSingleLineArray(TEXT("DetailPropertyExpansion"), *Class->GetName(), ExpandedPropertyItems, GEditorUserSettingsIni);
		SetExpandedItems( StartNode, ExpandedPropertyItems );
	}

	if( BestBaseClass )
	{
		GConfig->GetString(TEXT("DetailCustomWidgetExpansion"), *BestBaseClass->GetName(), ExpandedCustomItems, GEditorUserSettingsIni);
		TArray<FString> ExpandedCustomItemsArray;
		ExpandedCustomItems.ParseIntoArray( &ExpandedCustomItemsArray, TEXT(","), true );

		ExpandedDetailNodes.Append( ExpandedCustomItemsArray );
	}

}

/** Called before during SetObjectArray before we change the objects being observed */
void SDetailsView::PreSetObject()
{
	ExternalRootPropertyNodes.Empty();

	// Save existing expanded items first
	SaveExpandedItems();

	RootNodePendingKill = RootPropertyNode;

	RootPropertyNode = MakeShareable( new FObjectPropertyNode );

	SelectedActors.Empty();
	SelectedObjects.Empty();
}

/** Called at the end of SetObjectArray after we change the objects being observed */
void SDetailsView::PostSetObject()
{
	DestroyColorPicker();
	ColorPropertyNode = NULL;

	FPropertyNodeInitParams InitParams;
	InitParams.ParentNode = NULL;
	InitParams.Property = NULL;
	InitParams.ArrayOffset = 0;
	InitParams.ArrayIndex = INDEX_NONE;
	InitParams.bAllowChildren = true;
	InitParams.bForceHiddenPropertyVisibility =  FPropertySettings::Get().ShowHiddenProperties();

	RootPropertyNode->InitNode( InitParams );

	bool bInitiallySeen = true;
	bool bParentAllowsVisible = true;
	// Restore existing expanded items
	RestoreExpandedItems();

	UpdatePropertyMap();
}

void SDetailsView::QueryLayoutForClass( FDetailLayoutBuilderImpl& CustomDetailLayout, UStruct* Class )
{
	CustomDetailLayout.SetCurrentCustomizationClass( CastChecked<UClass>( Class ), NAME_None );

	FPropertyEditorModule& ParentPlugin = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FCustomDetailLayoutMap& GlobalCustomLayoutMap = ParentPlugin.ClassToDetailLayoutMap;
	FCustomDetailLayoutNameMap& GlobalCustomLayoutNameMap = ParentPlugin.ClassNameToDetailLayoutNameMap;

	// Check the instanced map first
	FDetailLayoutCallback* Callback = InstancedClassToDetailLayoutMap.Find( TWeakObjectPtr<UStruct>(Class) );

	if( !Callback )
	{
		// callback wasn't found in the per instance map, try the global instances instead
		Callback = GlobalCustomLayoutMap.Find( TWeakObjectPtr<UStruct>(Class) );
	}

	if( !Callback )
	{
		// callback wasn't found in the global class map, try the global name map instead
		Callback = GlobalCustomLayoutNameMap.Find( Class->GetFName() );
	}

	if( Callback && Callback->DetailLayoutDelegate.IsBound() )
	{
		// Create a new instance of the custom detail layout for the current class
		TSharedRef<IDetailCustomization> CustomizationInstance = Callback->DetailLayoutDelegate.Execute();

		// Ask for details immediately
		CustomizationInstance->CustomizeDetails( CustomDetailLayout );

		// Save the instance from destruction until we refresh
		CustomizationClassInstances.Add( CustomizationInstance );
	}
}

void SDetailsView::QueryCustomDetailLayout( FDetailLayoutBuilderImpl& CustomDetailLayout )
{
	FPropertyEditorModule& ParentPlugin = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Get the registered classes that customize details
	FCustomDetailLayoutMap& GlobalCustomLayoutMap = ParentPlugin.ClassToDetailLayoutMap;
	FCustomDetailLayoutNameMap& GlobalCustomLayoutNameMap = ParentPlugin.ClassNameToDetailLayoutNameMap;
	
	UClass* BaseClass = GetBaseClass();

	// All the current customization instances need to be deleted when it is safe
	CustomizationClassInstancesPendingDelete = CustomizationClassInstances;

	CustomizationClassInstances.Empty();

	//Ask for generic details not specific to an object being viewed 
	if( GenericLayoutDelegate.IsBound() )
	{
		// Create a new instance of the custom detail layout for the current class
		TSharedRef<IDetailCustomization> CustomizationInstance = GenericLayoutDelegate.Execute();

		// Ask for details immediately
		CustomizationInstance->CustomizeDetails( CustomDetailLayout );

		// Save the instance from destruction until we refresh
		CustomizationClassInstances.Add( CustomizationInstance );
	}


	// Sort them by query order.  @todo not good enough
	struct FCompareFDetailLayoutCallback
	{
		FORCEINLINE bool operator()( const FDetailLayoutCallback& A, const FDetailLayoutCallback& B ) const
		{
			return A.Order < B.Order;
		}
	};

	TMap< TWeakObjectPtr<UStruct>, FDetailLayoutCallback*> FinalCallbackMap;

	for( auto ClassIt = ClassesWithProperties.CreateConstIterator(); ClassIt; ++ClassIt )
	{
		// Check the instanced map first
		FDetailLayoutCallback* Callback = InstancedClassToDetailLayoutMap.Find( *ClassIt );

		if( !Callback )
		{
			// callback wasn't found in the per instance map, try the global instances instead
			Callback = GlobalCustomLayoutMap.Find( *ClassIt );
		}

		if( !Callback )
		{
			// callback wasn't found in the global class map, try the global name map instead
			Callback = GlobalCustomLayoutNameMap.Find( (*ClassIt)->GetFName() );
		}

		if( Callback )
		{
			FinalCallbackMap.Add( *ClassIt, Callback );
		}
	}


	FinalCallbackMap.ValueSort( FCompareFDetailLayoutCallback() );

	TSet<UStruct*> QueriedClasses;

	if( FinalCallbackMap.Num() > 0 )
	{
		// Ask each class that we have properties for to customize its layout
		for( auto LayoutIt(FinalCallbackMap.CreateConstIterator()); LayoutIt; ++LayoutIt )
		{
			const TWeakObjectPtr<UStruct> WeakClass = LayoutIt.Key();

			if( WeakClass.IsValid() )
			{
				UStruct* Class = WeakClass.Get();

				FClassInstanceToPropertyMap& InstancedPropertyMap = ClassToPropertyMap.FindChecked( Class->GetFName() );
				for( FClassInstanceToPropertyMap::TIterator InstanceIt(InstancedPropertyMap); InstanceIt; ++InstanceIt )
				{
					CustomDetailLayout.SetCurrentCustomizationClass( CastChecked<UClass>( Class ), InstanceIt.Key() );

					const FOnGetDetailCustomizationInstance& DetailDelegate = LayoutIt.Value()->DetailLayoutDelegate;

					if( DetailDelegate.IsBound() )
					{
						QueriedClasses.Add( Class );

						// Create a new instance of the custom detail layout for the current class
						TSharedRef<IDetailCustomization> CustomizationInstance = DetailDelegate.Execute();
							
						// Ask for details immediately
						CustomizationInstance->CustomizeDetails( CustomDetailLayout );

						// Save the instance from destruction until we refresh
						CustomizationClassInstances.Add( CustomizationInstance );
					}
				}
			}
		}
	}

	// Ensure that the base class and its parents are always queried
	TSet<UStruct*> ParentClassesToQuery;
	if( BaseClass && !QueriedClasses.Contains( BaseClass ) )
	{
		ParentClassesToQuery.Add( BaseClass );
		ClassesWithProperties.Add( BaseClass );
	}

	// Find base classes of queried classes that were not queried and add them to the query list
	// this supports cases where a parent class has no properties but still wants to add customization
	for( auto QueriedClassIt = ClassesWithProperties.CreateConstIterator();QueriedClassIt; ++QueriedClassIt )
	{
		UStruct* ParentStruct = (*QueriedClassIt)->GetSuperStruct();
		
		while( ParentStruct && ParentStruct->IsA( UClass::StaticClass() ) && !QueriedClasses.Contains(ParentStruct)  && !ClassesWithProperties.Contains( ParentStruct ) )
		{
			ParentClassesToQuery.Add( ParentStruct );
			ParentStruct = ParentStruct->GetSuperStruct();

		}
	}

	// Query extra base classes
	for( auto ParentIt = ParentClassesToQuery.CreateConstIterator(); ParentIt; ++ParentIt )
	{
		QueryLayoutForClass( CustomDetailLayout, *ParentIt );
	}
}

/** 
 * Hides or shows properties based on the passed in filter text
 * 
 * @param InFilterText	The filter text
 */
void SDetailsView::FilterView( const FString& InFilterText )
{
	TArray<FString> CurrentFilterStrings;

	FString ParseString = InFilterText;
	// Remove whitespace from the front and back of the string
	ParseString.Trim();
	ParseString.TrimTrailing();
	ParseString.ParseIntoArray(&CurrentFilterStrings, TEXT(" "), true);

	bHasActiveFilter = CurrentFilterStrings.Num() > 0;

	CurrentFilter.FilterStrings = CurrentFilterStrings;

	UpdateFilteredDetails();
}

void SDetailsView::UpdateFilteredDetails()
{
	RootPropertyNode->FilterNodes( CurrentFilter.FilterStrings );
	RootPropertyNode->ProcessSeenFlags( true );

	for( int32 NodeIndex = 0; NodeIndex < ExternalRootPropertyNodes.Num(); ++NodeIndex )
	{
		TSharedPtr<FObjectPropertyNode> ObjectNode = ExternalRootPropertyNodes[NodeIndex].Pin();

		if( ObjectNode.IsValid() )
		{
			ObjectNode->FilterNodes( CurrentFilter.FilterStrings );
			ObjectNode->ProcessSeenFlags( true );
		}
	}

	if( DetailLayout.IsValid() )
	{
		DetailLayout->FilterDetailLayout( CurrentFilter );
	}

	RootTreeNodes = DetailLayout->GetRootTreeNodes();

	DetailTree->RequestTreeRefresh();
}


/** Ticks the property view.  This function performs a data consistency check */
void SDetailsView::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	for( int32 i = 0; i < CustomizationClassInstancesPendingDelete.Num(); ++i )
	{
		ensure( CustomizationClassInstancesPendingDelete[i].IsUnique() );
	}

	if( RootNodePendingKill.IsValid() )
	{
		RootNodePendingKill->RemoveAllObjects();
		RootNodePendingKill.Reset();
	}

	// Empty all the customization instances that need to be deleted
	CustomizationClassInstancesPendingDelete.Empty();

	// Purge any objects that are marked pending kill from the object list
	RootPropertyNode->PurgeKilledObjects();

	if( DeferredActions.Num() > 0 )
	{
		// Any deferred actions are likely to cause the node  tree to be at least partially rebuilt
		// Save the expansion state of existing nodes so we can expand them later
		SaveExpandedItems();

		// Execute any deferred actions
		for( int32 ActionIndex = 0; ActionIndex < DeferredActions.Num(); ++ActionIndex )
		{
			DeferredActions[ActionIndex].ExecuteIfBound();
		}
		DeferredActions.Empty();
	}

	bool bValidateExternalNodes = true;
	FPropertyNode::DataValidationResult Result = RootPropertyNode->EnsureDataIsValid();
	if( Result == FPropertyNode::PropertiesChanged || Result == FPropertyNode::EditInlineNewValueChanged  )
	{
		RestoreExpandedItems();
		UpdatePropertyMap();
	}
	else if(Result == FPropertyNode::ArraySizeChanged  )
	{
		RestoreExpandedItems();
		UpdateFilteredDetails();
	}
	else if( Result == FPropertyNode::ObjectInvalid )
	{
		TArray< TWeakObjectPtr< UObject > > ResetArray;
		for ( TPropObjectIterator Itor( RootPropertyNode->ObjectIterator() ) ; Itor ; ++Itor )
		{
			TWeakObjectPtr<UObject> Object = *Itor;
			if( Object.IsValid() )
			{
				ResetArray.Add( Object.Get() );
			}
		}

		SetObjectArrayPrivate(ResetArray);

		// All objects are being reset, no need to validate external nodes
		bValidateExternalNodes = false;
	}

	if( bValidateExternalNodes )
	{
		for( int32 NodeIndex = 0; NodeIndex < ExternalRootPropertyNodes.Num(); ++NodeIndex )
		{
			TSharedPtr<FObjectPropertyNode> ObjectNode = ExternalRootPropertyNodes[NodeIndex].Pin();

			if( ObjectNode.IsValid() )
			{
				Result = ObjectNode->EnsureDataIsValid();
				if( Result == FPropertyNode::PropertiesChanged || Result == FPropertyNode::EditInlineNewValueChanged  )
				{
					RestoreExpandedItems( ObjectNode );
					UpdatePropertyMap();
					// Note this will invalidate all the external root nodes so there is no need to continue
					ExternalRootPropertyNodes.Empty();
					break;
				}
				else if(Result == FPropertyNode::ArraySizeChanged  )
				{
					RestoreExpandedItems( ObjectNode );
					UpdateFilteredDetails();
				}
			}
			else
			{
				// Remove the current node if it is no longer valid
				ExternalRootPropertyNodes.RemoveAt( NodeIndex );
				--NodeIndex;
			}
		}
	}

	if( ThumbnailPool.IsValid() )
	{
		ThumbnailPool->Tick( InDeltaTime );
	}

	if( DetailLayout.IsValid() )
	{
		DetailLayout->Tick( InDeltaTime );
	}

	if( !ColorPropertyNode.IsValid() && bHasOpenColorPicker )
	{
		// Destroy the color picker window if the color property node has become invalid
		DestroyColorPicker();
		bHasOpenColorPicker = false;
	}

	
	if( FilteredNodesRequestingExpansionState.Num() > 0 )
	{
		// change expansion state on the nodes that request it
		for( TMap<TSharedRef<IDetailTreeNode>, bool >::TConstIterator It(FilteredNodesRequestingExpansionState); It; ++It )
		{
			DetailTree->SetItemExpansion( It.Key(), It.Value() );
		}

		FilteredNodesRequestingExpansionState.Empty();
	}
}

void SDetailsView::OnColorPickerWindowClosed( const TSharedRef<SWindow>& Window ) 
{
	// A color picker window is no longer open
	bHasOpenColorPicker = false;
	ColorPropertyNode.Reset();
}

/** 
 * Creates the color picker window for this property view.
 *
 * @param Node				The slate property node to edit.
 * @param bUseAlpha			Whether or not alpha is supported
 */
void SDetailsView::CreateColorPickerWindow(const TSharedRef< FPropertyEditor >& PropertyEditor, bool bUseAlpha)
{
	const TSharedRef< FPropertyNode > PinnedColorPropertyNode = PropertyEditor->GetPropertyNode();
	ColorPropertyNode = PinnedColorPropertyNode;

	UProperty* Property = PinnedColorPropertyNode->GetProperty();
	check(Property);

	FReadAddressList ReadAddresses;
	PinnedColorPropertyNode->GetReadAddress( false, ReadAddresses, false );

	TArray<FLinearColor*> LinearColor;
	TArray<FColor*> DWORDColor;
	for( int32 ColorIndex = 0; ColorIndex < ReadAddresses.Num(); ++ColorIndex ) 
	{
		const uint8* Addr = ReadAddresses.GetAddress( ColorIndex );
		if( Addr )
		{
			if( Cast<UStructProperty>(Property)->Struct->GetFName() == NAME_Color )
			{
				DWORDColor.Add((FColor*)Addr);
			}
			else
			{
				check( Cast<UStructProperty>(Property)->Struct->GetFName() == NAME_LinearColor );
				LinearColor.Add((FLinearColor*)Addr);
			}
		}
	}

	bHasOpenColorPicker = true;

	FColorPickerArgs PickerArgs;
	PickerArgs.ParentWidget = AsShared();
	PickerArgs.bUseAlpha = bUseAlpha;
	PickerArgs.DisplayGamma = TAttribute<float>::Create( TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma) );
	PickerArgs.ColorArray = &DWORDColor;
	PickerArgs.LinearColorArray = &LinearColor;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP( this, &SDetailsView::SetColorPropertyFromColorPicker);
	PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateSP( this, &SDetailsView::OnColorPickerWindowClosed );

	OpenColorPicker(PickerArgs);
}

TSharedPtr<IPropertyUtilities> SDetailsView::GetPropertyUtilities() 
{
	return PropertyUtilities;
}

void SDetailsView::SetOnObjectArrayChanged(FOnObjectArrayChanged OnObjectArrayChangedDelegate)
{
	OnObjectArrayChanged = OnObjectArrayChangedDelegate;
}

const UClass* SDetailsView::GetBaseClass() const
{
	if( RootPropertyNode.IsValid() )
	{
		return RootPropertyNode->GetObjectBaseClass();
	}

	return NULL;
}

UClass* SDetailsView::GetBaseClass()
{
	if( RootPropertyNode.IsValid() )
	{
		return RootPropertyNode->GetObjectBaseClass();
	}

	return NULL;
}


void SDetailsView::RegisterInstancedCustomPropertyLayout( UClass* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	check( Class );

	FDetailLayoutCallback Callback;
	Callback.DetailLayoutDelegate = DetailLayoutDelegate;
	// @todo: DetailsView: Fix me: this specifies the order in which detail layouts should be queried
	Callback.Order = InstancedClassToDetailLayoutMap.Num();

	InstancedClassToDetailLayoutMap.Add( Class, Callback );
}

void SDetailsView::UnregisterInstancedCustomPropertyLayout( UClass* Class )
{
	check( Class );

	InstancedClassToDetailLayoutMap.Remove( Class );
}

void SDetailsView::SetIsPropertyVisibleDelegate( FIsPropertyVisible InIsPropertyVisible )
{
	IsPropertyVisible = InIsPropertyVisible;
}

void SDetailsView::SetIsPropertyEditingEnabledDelegate( FIsPropertyEditingEnabled IsPropertyEditingEnabled )
{
	IsPropertyEditingEnabledDelegate = IsPropertyEditingEnabled;
}

bool SDetailsView::IsPropertyEditingEnabled() const
{
	// If the delegate is not bound assume property editing is enabled, otherwise ask the delegate
	return !IsPropertyEditingEnabledDelegate.IsBound() || IsPropertyEditingEnabledDelegate.Execute();
}

void SDetailsView::SetGenericLayoutDetailsDelegate( FOnGetDetailCustomizationInstance OnGetGenericDetails )
{
	GenericLayoutDelegate = OnGetGenericDetails;
}

TSharedPtr<FAssetThumbnailPool> SDetailsView::GetThumbnailPool() const
{
	if( !ThumbnailPool.IsValid() )
	{
		// Create a thumbnail pool for the view if it doesnt exist.  This does not use resources of no thumbnails are used
		ThumbnailPool = MakeShareable( new FAssetThumbnailPool( 50, TAttribute<bool>::Create( TAttribute<bool>::FGetter::CreateSP(this, &SDetailsView::IsHovered) ) ) );
	}

	return ThumbnailPool;
}

void SDetailsView::NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) 
{
	OnFinishedChangingPropertiesDelegate.Broadcast(PropertyChangedEvent);
}

void SDetailsView::RequestItemExpanded( TSharedRef<IDetailTreeNode> TreeNode, bool bExpand )
{
	// Don't change expansion state if its already in that state
	if( DetailTree->IsItemExpanded(TreeNode) != bExpand )
	{
		FilteredNodesRequestingExpansionState.Add( TreeNode, bExpand );
	}
}

void SDetailsView::RefreshTree()
{
	DetailTree->RequestTreeRefresh();
}

void SDetailsView::SaveCustomExpansionState( const FString& NodePath, bool bIsExpanded )
{
	if( bIsExpanded )
	{
		ExpandedDetailNodes.Add( NodePath );
	}
	else
	{
		ExpandedDetailNodes.Remove( NodePath );
	}
}

bool SDetailsView::GetCustomSavedExpansionState( const FString& NodePath ) const
{
	return ExpandedDetailNodes.Contains( NodePath );
}

bool SDetailsView::SupportsKeyboardFocus() const
{
	return DetailsViewArgs.bSearchInitialKeyFocus && SearchBox->SupportsKeyboardFocus() && GetFilterBoxVisibility() == EVisibility::Visible;
}

FReply SDetailsView::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	FReply Reply = FReply::Handled();

	if( InKeyboardFocusEvent.GetCause() != EKeyboardFocusCause::Cleared )
	{
		Reply.SetKeyboardFocus( SearchBox.ToSharedRef(), InKeyboardFocusEvent.GetCause() );
	}

	return Reply;
}

void SDetailsView::AddExternalRootPropertyNode( TSharedRef<FObjectPropertyNode> ExternalRootNode )
{
	ExternalRootPropertyNodes.Add( ExternalRootNode );
}

bool SDetailsView::IsCategoryHiddenByClass( FName CategoryName ) const
{
	return RootPropertyNode->GetHiddenCategories().Contains( CategoryName );
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PropertyEditorPrivatePCH.h"
#include "AssetSelection.h"
#include "SPropertyTreeViewImpl.h"
#include "Slate.h"
#include "PropertyNode.h"
#include "ItemPropertyNode.h"
#include "CategoryPropertyNode.h"
#include "ObjectPropertyNode.h"
#include "PropertyNode.h"
#include "SDetailsView.h"
#include "SSingleProperty.h"
#include "Editor/MainFrame/Public/MainFrame.h"
#include "PropertyChangeListener.h"
#include "PropertyEditorToolkit.h"

#include "PropertyTable.h"
#include "SPropertyTable.h"
#include "TextPropertyTableCellPresenter.h"

#include "PropertyTableConstants.h"
#include "SStructureDetailsView.h"

IMPLEMENT_MODULE( FPropertyEditorModule, PropertyEditor );

TSharedRef<IPropertyTypeCustomization> FPropertyTypeLayoutCallback::GetCustomizationInstance() const
{
	return PropertyTypeLayoutDelegate.IsBound() ? PropertyTypeLayoutDelegate.Execute() : StaticCastSharedRef<IPropertyTypeCustomization>(DeprecatedLayoutDelegate.Execute());
}

void FPropertyTypeLayoutCallbackList::Add( const FPropertyTypeLayoutCallback& NewCallback )
{
	if( !NewCallback.PropertyTypeIdentifier.IsValid() )
	{
		BaseCallback = NewCallback;
	}
	else
	{
		IdentifierList.Add( NewCallback );
	}
}

void FPropertyTypeLayoutCallbackList::Remove( const TSharedPtr<IPropertyTypeIdentifier>& InIdentifier )
{
	if( !InIdentifier.IsValid() )
	{
		BaseCallback = FPropertyTypeLayoutCallback();
	}
	else
	{
		IdentifierList.RemoveAllSwap( [&InIdentifier]( FPropertyTypeLayoutCallback& Callback) { return Callback.PropertyTypeIdentifier == InIdentifier; } );
	}
}

const FPropertyTypeLayoutCallback& FPropertyTypeLayoutCallbackList::Find( const IPropertyHandle& PropertyHandle )
{
	if( IdentifierList.Num() > 0 )
	{
		FPropertyTypeLayoutCallback* Callback =
			IdentifierList.FindByPredicate
			(
				[&]( const FPropertyTypeLayoutCallback& InCallback )
				{
					return InCallback.PropertyTypeIdentifier->IsPropertyTypeCustomized( PropertyHandle );
				}
			);

		if( Callback )
		{
			return *Callback;
		}
	}

	return BaseCallback;
}

void FPropertyEditorModule::StartupModule()
{
}

void FPropertyEditorModule::ShutdownModule()
{
	// NOTE: It's vital that we clean up everything created by this DLL here!  We need to make sure there
	//       are no outstanding references to objects as the compiled code for this module's class will
	//       literally be unloaded from memory after this function exits.  This even includes instantiated
	//       templates, such as delegate wrapper objects that are allocated by the module!
	DestroyColorPicker();

	AllDetailViews.Empty();
	AllSinglePropertyViews.Empty();
}

void FPropertyEditorModule::NotifyCustomizationModuleChanged()
{
	// The module was changed (loaded or unloaded), force a refresh.  Note it is assumed the module unregisters all customization delegates before this
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();

			DetailViewPin->ForceRefresh();
		}
	}
}

static bool ShouldShowProperty(const FPropertyAndParent& PropertyAndParent, bool bHaveTemplate)
{
	const UProperty& Property = PropertyAndParent.Property;

	if ( bHaveTemplate )
	{
		const UClass* PropertyOwnerClass = Cast<const UClass>(Property.GetOuter());
		const bool bDisableEditOnTemplate = PropertyOwnerClass 
			&& PropertyOwnerClass->HasAllFlags(RF_Native) 
			&& Property.HasAnyPropertyFlags(CPF_DisableEditOnTemplate);
		if(bDisableEditOnTemplate)
		{
			return false;
		}
	}
	return true;
}

TSharedRef<SWindow> FPropertyEditorModule::CreateFloatingDetailsView( const TArray< UObject* >& InObjects, bool bIsLockable )
{

	TSharedRef<SWindow> NewSlateWindow = SNew(SWindow)
		.Title( NSLOCTEXT("PropertyEditor", "WindowTitle", "Property Editor") )
		.ClientSize( FVector2D(400, 550) );

	// If the main frame exists parent the window to it
	TSharedPtr< SWindow > ParentWindow;
	if( FModuleManager::Get().IsModuleLoaded( "MainFrame" ) )
	{
		IMainFrameModule& MainFrame = FModuleManager::GetModuleChecked<IMainFrameModule>( "MainFrame" );
		ParentWindow = MainFrame.GetParentWindow();
	}

	if( ParentWindow.IsValid() )
	{
		// Parent the window to the main frame 
		FSlateApplication::Get().AddWindowAsNativeChild( NewSlateWindow, ParentWindow.ToSharedRef() );
	}
	else
	{
		FSlateApplication::Get().AddWindow( NewSlateWindow );
	}
	
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = bIsLockable;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TSharedRef<IDetailsView> DetailView = PropertyEditorModule.CreateDetailView( Args );

	bool bHaveTemplate = false;
	for(int32 i=0; i<InObjects.Num(); i++)
	{
		if(InObjects[i] != NULL && InObjects[i]->IsTemplate())
		{
			bHaveTemplate = true;
			break;
		}
	}

	DetailView->SetIsPropertyVisibleDelegate( FIsPropertyVisible::CreateStatic(&ShouldShowProperty, bHaveTemplate) );

	DetailView->SetObjects( InObjects );

	NewSlateWindow->SetContent(
		SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush(TEXT("PropertyWindow.WindowBorder")) )
		[
			DetailView
		]
	);

	return NewSlateWindow;
}

TSharedRef<SPropertyTreeViewImpl> FPropertyEditorModule::CreatePropertyView( 
	UObject* InObject,
	bool bAllowFavorites, 
	bool bIsLockable, 
	bool bHiddenPropertyVisibility, 
	bool bAllowSearch, 
	bool ShowTopLevelNodes,
	FNotifyHook* InNotifyHook, 
	float InNameColumnWidth,
	FOnPropertySelectionChanged OnPropertySelectionChanged,
	FOnPropertyClicked OnPropertyMiddleClicked,
	FConstructExternalColumnHeaders ConstructExternalColumnHeaders,
	FConstructExternalColumnCell ConstructExternalColumnCell)
{
	TSharedRef<SPropertyTreeViewImpl> PropertyView = 
		SNew( SPropertyTreeViewImpl )
		.IsLockable( bIsLockable )
		.AllowFavorites( bAllowFavorites )
		.HiddenPropertyVis( bHiddenPropertyVisibility )
		.NotifyHook( InNotifyHook )
		.AllowSearch( bAllowSearch )
		.ShowTopLevelNodes( ShowTopLevelNodes )
		.NameColumnWidth( InNameColumnWidth )
		.OnPropertySelectionChanged( OnPropertySelectionChanged )
		.OnPropertyMiddleClicked( OnPropertyMiddleClicked )
		.ConstructExternalColumnHeaders( ConstructExternalColumnHeaders )
		.ConstructExternalColumnCell( ConstructExternalColumnCell );

	if( InObject )
	{
		TArray< TWeakObjectPtr< UObject > > Objects;
		Objects.Add( InObject );
		PropertyView->SetObjectArray( Objects );
	}

	return PropertyView;
}

TSharedRef<IDetailsView> FPropertyEditorModule::CreateDetailView( const FDetailsViewArgs& DetailsViewArgs )
{
	// Compact the list of detail view instances
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if ( !AllDetailViews[ViewIndex].IsValid() )
		{
			AllDetailViews.RemoveAtSwap( ViewIndex );
			--ViewIndex;
		}
	}

	TSharedRef<SDetailsView> DetailView = 
		SNew( SDetailsView )
		.DetailsViewArgs( DetailsViewArgs );

	AllDetailViews.Add( DetailView );
	return DetailView;
}

TSharedPtr<IDetailsView> FPropertyEditorModule::FindDetailView( const FName ViewIdentifier ) const
{
	if(ViewIdentifier.IsNone())
	{
		return nullptr;
	}

	for(auto It = AllDetailViews.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<SDetailsView> DetailsView = It->Pin();
		if(DetailsView.IsValid() && DetailsView->GetIdentifier() == ViewIdentifier)
		{
			return DetailsView;
		}
	}

	return nullptr;
}

TSharedPtr<ISinglePropertyView> FPropertyEditorModule::CreateSingleProperty( UObject* InObject, FName InPropertyName, const FSinglePropertyParams& InitParams )
{
	// Compact the list of detail view instances
	for( int32 ViewIndex = 0; ViewIndex < AllSinglePropertyViews.Num(); ++ViewIndex )
	{
		if ( !AllSinglePropertyViews[ViewIndex].IsValid() )
		{
			AllSinglePropertyViews.RemoveAtSwap( ViewIndex );
			--ViewIndex;
		}
	}

	TSharedRef<SSingleProperty> Property = 
		SNew( SSingleProperty )
		.Object( InObject )
		.PropertyName( InPropertyName )
		.NamePlacement( InitParams.NamePlacement )
		.NameOverride( InitParams.NameOverride )
		.NotifyHook( InitParams.NotifyHook )
		.PropertyFont( InitParams.Font );

	if( Property->HasValidProperty() )
	{
		AllSinglePropertyViews.Add( Property );

		return Property;
	}

	return NULL;
}

TSharedRef< IPropertyTable > FPropertyEditorModule::CreatePropertyTable()
{
	return MakeShareable( new FPropertyTable() );
}

TSharedRef< SWidget > FPropertyEditorModule::CreatePropertyTableWidget( const TSharedRef< class IPropertyTable >& PropertyTable )
{
	return SNew( SPropertyTable, PropertyTable );
}

TSharedRef< SWidget > FPropertyEditorModule::CreatePropertyTableWidget( const TSharedRef< IPropertyTable >& PropertyTable, const TArray< TSharedRef< class IPropertyTableCustomColumn > >& Customizations )
{
	return SNew( SPropertyTable, PropertyTable )
		.ColumnCustomizations( Customizations );
}

TSharedRef< IPropertyTableWidgetHandle > FPropertyEditorModule::CreatePropertyTableWidgetHandle( const TSharedRef< IPropertyTable >& PropertyTable )
{
	TSharedRef< FPropertyTableWidgetHandle > FWidgetHandle = MakeShareable( new FPropertyTableWidgetHandle( SNew( SPropertyTable, PropertyTable ) ) );

	TSharedRef< IPropertyTableWidgetHandle > IWidgetHandle = StaticCastSharedRef<IPropertyTableWidgetHandle>(FWidgetHandle);

	 return IWidgetHandle;
}

TSharedRef< IPropertyTableWidgetHandle > FPropertyEditorModule::CreatePropertyTableWidgetHandle( const TSharedRef< IPropertyTable >& PropertyTable, const TArray< TSharedRef< class IPropertyTableCustomColumn > >& Customizations )
{
	TSharedRef< FPropertyTableWidgetHandle > FWidgetHandle = MakeShareable( new FPropertyTableWidgetHandle( SNew( SPropertyTable, PropertyTable )
		.ColumnCustomizations( Customizations )));

	TSharedRef< IPropertyTableWidgetHandle > IWidgetHandle = StaticCastSharedRef<IPropertyTableWidgetHandle>(FWidgetHandle);

	 return IWidgetHandle;
}

TSharedRef< IPropertyTableCellPresenter > FPropertyEditorModule::CreateTextPropertyCellPresenter(const TSharedRef< class FPropertyNode >& InPropertyNode, const TSharedRef< class IPropertyTableUtilities >& InPropertyUtilities, 
																								 const FSlateFontInfo* InFontPtr /* = NULL */)
{
	FSlateFontInfo InFont;

	if (InFontPtr == NULL)
	{
		// Encapsulating reference to Private file PropertyTableConstants.h
		InFont = FEditorStyle::GetFontStyle( PropertyTableConstants::NormalFontStyle );
	}
	else
	{
		InFont = *InFontPtr;
	}

	TSharedRef< FPropertyEditor > PropertyEditor = FPropertyEditor::Create( InPropertyNode, InPropertyUtilities );
	return MakeShareable( new FTextPropertyTableCellPresenter( PropertyEditor, InPropertyUtilities, InFont) );
}

TSharedRef< FAssetEditorToolkit > FPropertyEditorModule::CreatePropertyEditorToolkit( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit )
{
	return FPropertyEditorToolkit::CreateEditor( Mode, InitToolkitHost, ObjectToEdit );
}


TSharedRef< FAssetEditorToolkit > FPropertyEditorModule::CreatePropertyEditorToolkit( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< UObject* >& ObjectsToEdit )
{
	return FPropertyEditorToolkit::CreateEditor( Mode, InitToolkitHost, ObjectsToEdit );
}

TSharedRef< FAssetEditorToolkit > FPropertyEditorModule::CreatePropertyEditorToolkit( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< TWeakObjectPtr< UObject > >& ObjectsToEdit )
{
	TArray< UObject* > RawObjectsToEdit;
	for( auto ObjectIter = ObjectsToEdit.CreateConstIterator(); ObjectIter; ++ObjectIter )
	{
		RawObjectsToEdit.Add( ObjectIter->Get() );
	}

	return FPropertyEditorToolkit::CreateEditor( Mode, InitToolkitHost, RawObjectsToEdit );
}

TSharedRef<IPropertyChangeListener> FPropertyEditorModule::CreatePropertyChangeListener()
{
	return MakeShareable( new FPropertyChangeListener );
}

void FPropertyEditorModule::RegisterCustomClassLayout( FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	if (ClassName != NAME_None)
	{
		FDetailLayoutCallback Callback;
		Callback.DetailLayoutDelegate = DetailLayoutDelegate;
		// @todo: DetailsView: Fix me: this specifies the order in which detail layouts should be queried
		Callback.Order = ClassNameToDetailLayoutNameMap.Num();

		ClassNameToDetailLayoutNameMap.Add(ClassName, Callback);
	}
}

void FPropertyEditorModule::RegisterCustomPropertyLayout( FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	RegisterCustomClassLayout( ClassName, DetailLayoutDelegate );
}

void FPropertyEditorModule::UnregisterCustomClassLayout( FName ClassName )
{
	if (ClassName != NAME_None)
	{
		ClassNameToDetailLayoutNameMap.Remove(ClassName);
	}
}

void FPropertyEditorModule::UnregisterCustomPropertyLayout( FName ClassName )
{
	UnregisterCustomClassLayout(ClassName);
}

void FPropertyEditorModule::RegisterStructPropertyLayout( FName StructTypeName, FOnGetStructCustomizationInstance StructLayoutDelegate )
{
	if (StructTypeName != NAME_None)
	{
		FPropertyTypeLayoutCallback Callback;
		Callback.DeprecatedLayoutDelegate = StructLayoutDelegate;

		FPropertyTypeLayoutCallbackList* LayoutCallbacks = PropertyTypeToLayoutMap.Find(StructTypeName);
		if (LayoutCallbacks)
		{
			LayoutCallbacks->Add(Callback);
		}
		else
		{
			FPropertyTypeLayoutCallbackList NewLayoutCallbacks;
			NewLayoutCallbacks.Add(Callback);
			PropertyTypeToLayoutMap.Add(StructTypeName, NewLayoutCallbacks);
		}
	}
}

void FPropertyEditorModule::UnregisterStructPropertyLayout( FName StructTypeName )
{
	UnregisterCustomPropertyTypeLayout( StructTypeName );
}


void FPropertyEditorModule::RegisterCustomPropertyTypeLayout( FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate, TSharedPtr<IPropertyTypeIdentifier> Identifier )
{
	if( PropertyTypeName != NAME_None )
	{
		FPropertyTypeLayoutCallback Callback;
		Callback.PropertyTypeLayoutDelegate = PropertyTypeLayoutDelegate;
		Callback.PropertyTypeIdentifier = Identifier;

		FPropertyTypeLayoutCallbackList* LayoutCallbacks = PropertyTypeToLayoutMap.Find( PropertyTypeName );
		if( LayoutCallbacks )
		{
			LayoutCallbacks->Add( Callback );
		}
		else
		{
			FPropertyTypeLayoutCallbackList NewLayoutCallbacks;
			NewLayoutCallbacks.Add( Callback );
			PropertyTypeToLayoutMap.Add( PropertyTypeName, NewLayoutCallbacks );
		}
	}
}

void FPropertyEditorModule::UnregisterCustomPropertyTypeLayout( FName PropertyTypeName, TSharedPtr<IPropertyTypeIdentifier> Identifier )
{
	if( PropertyTypeName != NAME_None )
	{
		FPropertyTypeLayoutCallbackList* LayoutCallbacks = PropertyTypeToLayoutMap.Find( PropertyTypeName );

		if( LayoutCallbacks )
		{
			LayoutCallbacks->Remove( Identifier );
		}
	}
}

bool FPropertyEditorModule::HasUnlockedDetailViews() const
{
	uint32 NumUnlockedViews = 0;
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		const TWeakPtr<SDetailsView>& DetailView = AllDetailViews[ ViewIndex ];
		if( DetailView.IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = DetailView.Pin();
			if( DetailViewPin->IsUpdatable() &&  !DetailViewPin->IsLocked() )
			{
				return true;
			}
		}
	}

	return false;
}

/**
 * Refreshes property windows with a new list of objects to view
 * 
 * @param NewObjectList	The list of objects each property window should view
 */
void FPropertyEditorModule::UpdatePropertyViews( const TArray<UObject*>& NewObjectList )
{
	DestroyColorPicker();

	TSet<AActor*> ValidObjects;
	
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();
			if( DetailViewPin->IsUpdatable() )
			{
				if( !DetailViewPin->IsLocked() )
				{
					DetailViewPin->SetObjects( NewObjectList, true );
				}
				else
				{
					if( ValidObjects.Num() == 0 )
					{
						// Populate the valid objects list only once
						UWorld* IteratorWorld = GWorld;
						for( FActorIterator It(IteratorWorld); It; ++It )
						{
							ValidObjects.Add( *It );
						}
					}

					// If the property window is locked make sure all the actors still exist
					DetailViewPin->RemoveInvalidActors( ValidObjects );
				}
			}
		}
	}
}

void FPropertyEditorModule::ReplaceViewedObjects( const TMap<UObject*, UObject*>& OldToNewObjectMap )
{
	DestroyColorPicker();

	// Replace objects from detail views
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();

			DetailViewPin->ReplaceObjects( OldToNewObjectMap );
		}
	}

	// Replace objects from single views
	for( int32 ViewIndex = 0; ViewIndex < AllSinglePropertyViews.Num(); ++ViewIndex )
	{
		if( AllSinglePropertyViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SSingleProperty> SinglePropPin = AllSinglePropertyViews[ ViewIndex ].Pin();

			SinglePropPin->ReplaceObjects( OldToNewObjectMap );
		}
	}
}

void FPropertyEditorModule::RemoveDeletedObjects( TArray<UObject*>& DeletedObjects )
{
	DestroyColorPicker();

	// remove deleted objects from detail views
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();

			DetailViewPin->RemoveDeletedObjects( DeletedObjects );
		}
	}

	// remove deleted object from single property views
	for( int32 ViewIndex = 0; ViewIndex < AllSinglePropertyViews.Num(); ++ViewIndex )
	{
		if( AllSinglePropertyViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SSingleProperty> SinglePropPin = AllSinglePropertyViews[ ViewIndex ].Pin();

			SinglePropPin->RemoveDeletedObjects( DeletedObjects );
		}
	}
}

bool FPropertyEditorModule::IsCustomizedStruct(const UStruct* Struct) const
{
	if (Struct && !Struct->IsA<UUserDefinedStruct>())
	{
		return PropertyTypeToLayoutMap.Contains(Struct->GetFName());
	}
	return false;
}

FPropertyTypeLayoutCallback FPropertyEditorModule::GetPropertyTypeCustomization(const UProperty* Property, const IPropertyHandle& PropertyHandle)
{
	if( Property )
	{
		const UStructProperty* StructProperty = Cast<UStructProperty>(Property);
		bool bStructProperty = StructProperty && StructProperty->Struct;
		const bool bUserDefinedStruct = bStructProperty && StructProperty->Struct->IsA<UUserDefinedStruct>();
		bStructProperty &= !bUserDefinedStruct;

		const UByteProperty* ByteProperty = Cast<UByteProperty>(Property);
		const bool bEnumProperty = ByteProperty && ByteProperty->Enum;

		const UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property);
		const bool bObjectProperty = ObjectProperty != NULL && ObjectProperty->PropertyClass != NULL;

		FName PropertyTypeName;
		if( bStructProperty )
		{
			PropertyTypeName = StructProperty->Struct->GetFName();
		}
		else if( bEnumProperty )
		{
			PropertyTypeName = ByteProperty->Enum->GetFName();
		}
		else if ( bObjectProperty )
		{
			PropertyTypeName = ObjectProperty->PropertyClass->GetFName();
		}
		else
		{
			PropertyTypeName = Property->GetClass()->GetFName();
		}


		if (PropertyTypeName != NAME_None)
		{
			FPropertyTypeLayoutCallbackList* LayoutCallbacks = PropertyTypeToLayoutMap.Find(PropertyTypeName);
			if (LayoutCallbacks)
			{
				const FPropertyTypeLayoutCallback& Callback = LayoutCallbacks->Find(PropertyHandle);
				return Callback;
			}
		}
	}

	return FPropertyTypeLayoutCallback();
}

TSharedRef<class IStructureDetailsView> FPropertyEditorModule::CreateStructureDetailView(const FDetailsViewArgs& DetailsViewArgs, TSharedPtr<FStructOnScope> StructData, bool bShowObjects, const FString& CustomName)
{
	TSharedRef<SStructureDetailsView> DetailView =
		SNew(SStructureDetailsView)
		.DetailsViewArgs(DetailsViewArgs)
		.CustomName(CustomName);

	if (!bShowObjects)
	{
		struct FDontShowObjects
		{
			static bool CanShow( const FPropertyAndParent& PropertyAndParent )
			{
				return !PropertyAndParent.Property.IsA<UObjectPropertyBase>() && !PropertyAndParent.Property.IsA<UInterfaceProperty>();
			}
		};

		DetailView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&FDontShowObjects::CanShow));
	}
	DetailView->SetStructureData(StructData);

	return DetailView;
}
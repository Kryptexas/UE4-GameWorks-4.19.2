// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerObjectChangeListener.h"
#include "PropertyEditing.h"
#include "IPropertyChangeListener.h"

DEFINE_LOG_CATEGORY_STATIC(LogSequencerTools, Log, All);

FSequencerObjectChangeListener::FSequencerObjectChangeListener( TSharedRef<ISequencer> InSequencer, bool bInListenForActorsOnly )
	: Sequencer( InSequencer )
	, bListenForActorsOnly( bInListenForActorsOnly )
{
	FCoreDelegates::OnPreObjectPropertyChanged.AddRaw(this, &FSequencerObjectChangeListener::OnObjectPreEditChange);
	FCoreDelegates::OnObjectPropertyChanged.AddRaw(this, &FSequencerObjectChangeListener::OnObjectPostEditChange);
	//GEditor->OnPreActorMoved.AddRaw(this, &FSequencerObjectChangeListener::OnActorPreEditMove);
	GEditor->OnActorMoved().AddRaw( this, &FSequencerObjectChangeListener::OnActorPostEditMove );
}

FSequencerObjectChangeListener::~FSequencerObjectChangeListener()
{
	FCoreDelegates::OnPreObjectPropertyChanged.RemoveAll( this );
	FCoreDelegates::OnObjectPropertyChanged.RemoveAll( this );
	GEditor->OnActorMoved().RemoveAll( this );
}

void FSequencerObjectChangeListener::OnPropertyChanged( const TArray<UObject*>& ChangedObjects,  TSharedRef< const IPropertyHandle> PropertyValue, bool bRequireAutoKey )
{
	TSharedPtr< const IPropertyHandle> HandleToUse = PropertyValue;

	TSharedPtr<const IPropertyHandle> ParentHandle = PropertyValue->GetParentHandle();

	if( ParentHandle.IsValid() && ParentHandle->GetProperty() && ParentHandle->GetProperty()->IsA<UStructProperty>() )
	{
		// Handle the case where internal values of a struct are being set.  We want to key the whole struct in this case
		// @todo Sequencer: Is this always the case?
		HandleToUse = ParentHandle;
	}

	UProperty* Property = HandleToUse->GetProperty();
	UStructProperty* StructProperty = Cast<UStructProperty>( Property );

	FName PropertyName = StructProperty ? StructProperty->Struct->GetFName() : Property->GetFName();

	ClassToPropertyChangedMap.FindRef( PropertyName ).Broadcast( ChangedObjects, *HandleToUse, bRequireAutoKey );
}

bool FSequencerObjectChangeListener::IsObjectValidForListening( UObject* Object ) const
{
	// @todo Sequencer - Pre/PostEditChange is sometimes called for inner objects of other objects (like actors with components)
	// We only want the outer object so assume it's an actor for now
	return bListenForActorsOnly ? Object->IsA<AActor>() : true;
}

FOnAnimatablePropertyChanged& FSequencerObjectChangeListener::GetOnAnimatablePropertyChanged( FName PropertyTypeName )
{
	return ClassToPropertyChangedMap.FindOrAdd( PropertyTypeName );
}

FOnPropagateObjectChanges& FSequencerObjectChangeListener::GetOnPropagateObjectChanges()
{
	return OnPropagateObjectChanges;
}


void FSequencerObjectChangeListener::OnObjectPreEditChange( UObject* Object )
{
	// We only care if we are in auto-key mode and not attempting to change properties of a CDO (which cannot be animated)
	if( Sequencer.IsValid() && Sequencer.Pin()->IsAutoKeyEnabled() && !Object->HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Register with the property editor module that we'd like to know about animated float properties that change
		FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

		if( IsObjectValidForListening(Object) )
		{
			TSharedPtr<IPropertyChangeListener> PropertyChangeListener = ActivePropertyChangeListeners.FindRef( Object );

			if( !PropertyChangeListener.IsValid() )
			{
				PropertyChangeListener = PropertyEditor.CreatePropertyChangeListener();
				
				ActivePropertyChangeListeners.Add( Object, PropertyChangeListener );

				PropertyChangeListener->GetOnPropertyChangedDelegate().AddRaw( this, &FSequencerObjectChangeListener::OnPropertyChanged, true );

				FPropertyListenerSettings Settings;
				// Ignore array and object properties
				Settings.bIgnoreArrayProperties = true;
				Settings.bIgnoreObjectProperties = true;
				// Property flags which must be on the property
				Settings.RequiredPropertyFlags = 0;
				// Property flags which cannot be on the property
				Settings.DisallowedPropertyFlags = CPF_EditConst;

				PropertyChangeListener->SetObject( *Object, Settings );
			}
		}
	}
}

void FSequencerObjectChangeListener::OnObjectPostEditChange( UObject* Object, FPropertyChangedEvent& PropertyChangedEvent )
{
	if( Object && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive )
	{
		bool bIsObjectValid = IsObjectValidForListening( Object );

		bool bShouldPropagateChanges = bIsObjectValid;

		// We only care if we are in auto-key mode and not attempting to change properties of a CDO (which cannot be animated)
		if( Sequencer.IsValid() && Sequencer.Pin()->IsAutoKeyEnabled() && bIsObjectValid && !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			TSharedPtr< IPropertyChangeListener > Listener;
			ActivePropertyChangeListeners.RemoveAndCopyValue( Object, Listener );

			if( Listener.IsValid() )
			{
				check( Listener.IsUnique() );
					
				// Don't recache new values, the listener will be destroyed after this call
				const bool bRecacheNewValues = false;

				const bool bFoundChanges = Listener->ScanForChanges( bRecacheNewValues );

				// If the listener did not find any changes we care about, propagate changes to puppets
				// @todo Sequencer - We might need to check per changed property
				bShouldPropagateChanges = !bFoundChanges;
			}
		}

		if( bShouldPropagateChanges )
		{
			OnPropagateObjectChanges.Broadcast( Object );
		}
	}
}


void FSequencerObjectChangeListener::OnActorPostEditMove( AActor* Actor )
{
	// @todo sequencer actors: Currently this only fires on a "final" move.  For our purposes we probably
	// want to get an update every single movement, even while dragging an object.
	FPropertyChangedEvent PropertyChangedEvent(nullptr);
	OnObjectPostEditChange( Actor, PropertyChangedEvent );
}

void FSequencerObjectChangeListener::TriggerAllPropertiesChanged(UObject* Object)
{
	if( Object )
	{
		// @todo Sequencer - Pre/PostEditChange is sometimes called for inner objects of other objects (like actors with components)
		// We only want the outer object so assume it's an actor for now
		bool bObjectIsActor = Object->IsA( AActor::StaticClass() );

		// Default to propagating changes to objects only if they are actors
		// if this change is handled by auto-key we will not propagate changes
		bool bShouldPropagateChanges = bObjectIsActor;

		// We only care if we are not attempting to change properties of a CDO (which cannot be animated)
		if( Sequencer.IsValid() && bObjectIsActor && !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			TSharedPtr<IPropertyChangeListener> PropertyChangeListener = ActivePropertyChangeListeners.FindRef( Object );
			
			if( !PropertyChangeListener.IsValid() )
			{
				// Register with the property editor module that we'd like to know about animated float properties that change
				FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

				PropertyChangeListener = PropertyEditor.CreatePropertyChangeListener();
				
				PropertyChangeListener->GetOnPropertyChangedDelegate().AddRaw( this, &FSequencerObjectChangeListener::OnPropertyChanged, false );

				FPropertyListenerSettings Settings;
				// Ignore array and object properties
				Settings.bIgnoreArrayProperties = true;
				Settings.bIgnoreObjectProperties = true;
				// Property flags which must be on the property
				Settings.RequiredPropertyFlags = CPF_Interp;
				// Property flags which cannot be on the property
				Settings.DisallowedPropertyFlags = CPF_EditConst;

				PropertyChangeListener->SetObject( *Object, Settings );
			}

			PropertyChangeListener->TriggerAllPropertiesChangedDelegate();
		}
	}
}

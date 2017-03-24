// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SequencerObjectChangeListener.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IPropertyChangeListener.h"

DEFINE_LOG_CATEGORY_STATIC(LogSequencerTools, Log, All);

FSequencerObjectChangeListener::FSequencerObjectChangeListener( TSharedRef<ISequencer> InSequencer )
	: Sequencer( InSequencer )
{
	FCoreUObjectDelegates::OnPreObjectPropertyChanged.AddRaw(this, &FSequencerObjectChangeListener::OnObjectPreEditChange);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FSequencerObjectChangeListener::OnObjectPostEditChange);
	//GEditor->OnPreActorMoved.AddRaw(this, &FSequencerObjectChangeListener::OnActorPreEditMove);
	GEditor->OnActorMoved().AddRaw( this, &FSequencerObjectChangeListener::OnActorPostEditMove );
}

FSequencerObjectChangeListener::~FSequencerObjectChangeListener()
{
	FCoreUObjectDelegates::OnPreObjectPropertyChanged.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	GEditor->OnActorMoved().RemoveAll( this );
}

void FSequencerObjectChangeListener::OnPropertyChanged(const TArray<UObject*>& ChangedObjects, const IPropertyHandle& PropertyHandle) const
{
	BroadcastPropertyChanged(FKeyPropertyParams(ChangedObjects, PropertyHandle, ESequencerKeyMode::AutoKey));

	for (UObject* Object : ChangedObjects)
	{
		if (Object)
		{
			const FOnObjectPropertyChanged* Event = ObjectToPropertyChangedEvent.Find(Object);
			if (Event)
			{
				Event->Broadcast(*Object);
			}
		}
	}
}

void FSequencerObjectChangeListener::BroadcastPropertyChanged( FKeyPropertyParams KeyPropertyParams ) const
{
	// Filter out objects that actually have the property path that will be keyable. 
	// Otherwise, this might try to key objects that don't have the requested property.
	// For example, a property changed for the FieldOfView property will be sent for 
	// both the CameraActor and the CameraComponent.
	TArray<UObject*> KeyableObjects;
	for (auto ObjectToKey : KeyPropertyParams.ObjectsToKey)
	{
		if (KeyPropertyParams.PropertyPath.GetNumProperties() > 0)
		{
			for (TFieldIterator<UProperty> PropertyIterator(ObjectToKey->GetClass()); PropertyIterator; ++PropertyIterator)
			{
				UProperty* Property = *PropertyIterator;
				if (Property == KeyPropertyParams.PropertyPath.GetRootProperty().Property.Get())
				{
					if (CanKeyProperty(FCanKeyPropertyParams(ObjectToKey->GetClass(), KeyPropertyParams.PropertyPath)))
					{
						KeyableObjects.Add(ObjectToKey);
						break;
					}
				}
			}
		}
	}

	if (!KeyableObjects.Num())
	{
		return;
	}

	const UStructProperty* StructProperty = Cast<const UStructProperty>(KeyPropertyParams.PropertyPath.GetLeafMostProperty().Property.Get());
	const UStructProperty* ParentStructProperty = nullptr;
	if (KeyPropertyParams.PropertyPath.GetNumProperties() > 1)
	{
		ParentStructProperty = Cast<const UStructProperty>(KeyPropertyParams.PropertyPath.GetPropertyInfo(KeyPropertyParams.PropertyPath.GetNumProperties() - 2).Property.Get());
	}

	FPropertyPath PropertyPath;
	FName StructPropertyNameToKey = NAME_None;

	bool bFoundAndBroadcastedDelegate = false;
	if (ParentStructProperty)
	{
		PropertyPath = *KeyPropertyParams.PropertyPath.TrimPath(1);

		// If the property parent is a struct, see if this property parent can be keyed. (e.g R,G,B,A for a color)
		FOnAnimatablePropertyChanged Delegate = PropertyChangedEventMap.FindRef(FAnimatedPropertyKey::FromStructType(ParentStructProperty->Struct));
		UProperty* Property = KeyPropertyParams.PropertyPath.GetLeafMostProperty().Property.Get();
		if (Delegate.IsBound() && Property)
		{
			bFoundAndBroadcastedDelegate = true;
			FPropertyChangedParams Params(KeyableObjects, PropertyPath, Property->GetFName(), KeyPropertyParams.KeyMode);
			Delegate.Broadcast(Params);
		}
	}

	if (!bFoundAndBroadcastedDelegate)
	{
		FPropertyChangedParams Params(KeyableObjects, KeyPropertyParams.PropertyPath, NAME_None, KeyPropertyParams.KeyMode);
		if (StructProperty)
		{
			PropertyChangedEventMap.FindRef(FAnimatedPropertyKey::FromStructType(StructProperty->Struct)).Broadcast(Params);
		}
		else if (UProperty* Property = KeyPropertyParams.PropertyPath.GetLeafMostProperty().Property.Get())
		{
			// the property in question is not a struct or an inner of the struct. See if it is directly keyable
			PropertyChangedEventMap.FindRef(FAnimatedPropertyKey::FromProperty(Property)).Broadcast(Params);
		}
	}
}

bool FSequencerObjectChangeListener::IsObjectValidForListening( UObject* Object ) const
{
	// @todo Sequencer - Pre/PostEditChange is sometimes called for inner objects of other objects (like actors with components)
	// We only want the outer object so assume it's an actor for now
	if (Sequencer.IsValid())
	{
		TSharedRef<ISequencer> PinnedSequencer = Sequencer.Pin().ToSharedRef();
		return (PinnedSequencer->GetFocusedMovieSceneSequence() && PinnedSequencer->GetFocusedMovieSceneSequence()->CanAnimateObject(*Object));
	}

	return false;
}

FOnAnimatablePropertyChanged& FSequencerObjectChangeListener::GetOnAnimatablePropertyChanged( FAnimatedPropertyKey PropertyKey )
{
	return PropertyChangedEventMap.FindOrAdd( PropertyKey );
}

FOnPropagateObjectChanges& FSequencerObjectChangeListener::GetOnPropagateObjectChanges()
{
	return OnPropagateObjectChanges;
}

FOnObjectPropertyChanged& FSequencerObjectChangeListener::GetOnAnyPropertyChanged(UObject& Object)
{
	return ObjectToPropertyChangedEvent.FindOrAdd(&Object);
}

void FSequencerObjectChangeListener::ReportObjectDestroyed(UObject& Object)
{
	ObjectToPropertyChangedEvent.Remove(&Object);
}

bool FSequencerObjectChangeListener::FindPropertySetter( const UStruct& PropertyStructure, FAnimatedPropertyKey PropertyKey, const FString& InPropertyVarName, const UStructProperty* StructProperty, const UArrayProperty* ArrayProperty) const
{
	if (!PropertyChangedEventMap.Contains( PropertyKey ))
	{
		return false;
	}

	FString PropertyVarName = InPropertyVarName;

	// If this is a bool property, strip off the 'b' so that the "Set" functions to be 
	// found are, for example, "SetHidden" instead of "SetbHidden"
	if (PropertyKey.PropertyTypeName == "BoolProperty")
	{
		PropertyVarName.RemoveFromStart("b", ESearchCase::CaseSensitive);
	}

	static const FString Set(TEXT("Set"));

	const FString FunctionString = Set + PropertyVarName;

	FName FunctionName = FName(*FunctionString);

	static const FName DeprecatedFunctionName(TEXT("DeprecatedFunction"));
	UFunction* Function = nullptr;
	if (const UClass* Class = Cast<const UClass>(&PropertyStructure))
	{
		Function = Class->FindFunctionByName(FunctionName);
	}
	bool bFoundValidFunction = false;
	if( Function && !Function->HasMetaData(DeprecatedFunctionName) )
	{
		bFoundValidFunction = true;
	}

	bool bFoundValidInterp = false;
	bool bFoundEditDefaultsOnly = false;
	bool bFoundEdit = false;
	if (ArrayProperty != nullptr)
	{
		if (ArrayProperty->HasAnyPropertyFlags(CPF_Interp))
		{
			bFoundValidInterp = true;
		}
		if (ArrayProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
		{
			bFoundEditDefaultsOnly = true;
		}
		if (ArrayProperty->HasAnyPropertyFlags(CPF_Edit))
		{
			bFoundEdit = true;
		}
	}
	else if (StructProperty != nullptr)
	{
		if (StructProperty->HasAnyPropertyFlags(CPF_Interp))
		{
			bFoundValidInterp = true;
		}
		if (StructProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
		{
			bFoundEditDefaultsOnly = true;
		}
		if (StructProperty->HasAnyPropertyFlags(CPF_Edit))
		{
			bFoundEdit = true;
		}
	}
	else
	{
		UProperty* Property = PropertyStructure.FindPropertyByName(FName(*InPropertyVarName));
		if (Property)
		{
			if (Property->HasAnyPropertyFlags(CPF_Interp))
			{
				bFoundValidInterp = true;
			}
			if (Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
			{
				bFoundEditDefaultsOnly = true;
			}
			if (Property->HasAnyPropertyFlags(CPF_Edit))
			{
				bFoundEdit = true;
			}
		}
	}
	
	// Valid if there's a setter function and the property is editable. Also valid if there's an interp keyword.
	return (bFoundValidFunction && bFoundEdit && !bFoundEditDefaultsOnly) || bFoundValidInterp;
}

bool FSequencerObjectChangeListener::CanKeyProperty(FCanKeyPropertyParams CanKeyPropertyParams) const
{
	if (CanKeyPropertyParams.PropertyPath.GetNumProperties() == 0)
	{
		return false;
	}

	const UStructProperty* StructProperty = Cast<const UStructProperty>(CanKeyPropertyParams.PropertyPath.GetLeafMostProperty().Property.Get());
	const UStructProperty* ParentStructProperty = nullptr;
	if (CanKeyPropertyParams.PropertyPath.GetNumProperties() > 1)
	{
		ParentStructProperty = Cast<const UStructProperty>(CanKeyPropertyParams.PropertyPath.GetPropertyInfo(CanKeyPropertyParams.PropertyPath.GetNumProperties() - 2).Property.Get());
	}

	const UArrayProperty* ParentArrayProperty = nullptr;
	if (CanKeyPropertyParams.PropertyPath.GetNumProperties() > 2)
	{
		ParentArrayProperty = Cast<const UArrayProperty>(CanKeyPropertyParams.PropertyPath.GetPropertyInfo(CanKeyPropertyParams.PropertyPath.GetNumProperties() - 3).Property.Get());
	}

	bool bFound = false;
	if ( StructProperty )
	{
		const UStruct* PropertyContainer = CanKeyPropertyParams.FindPropertyContainer(StructProperty);
		bFound = FindPropertySetter(*PropertyContainer, FAnimatedPropertyKey::FromStructType(StructProperty->Struct), StructProperty->GetName(), StructProperty );
	}

	if( !bFound && ParentStructProperty )
	{
		// If the property parent is a struct, see if this property parent can be keyed.
		const UStruct* PropertyContainer = CanKeyPropertyParams.FindPropertyContainer(ParentStructProperty);
		bFound = FindPropertySetter(*PropertyContainer, FAnimatedPropertyKey::FromStructType(ParentStructProperty->Struct), ParentStructProperty->GetName(), ParentStructProperty );

		if (!bFound && ParentArrayProperty)
		{
			// If the property parent is a struct contained in an array, see if this property parent can be keyed.
			PropertyContainer = CanKeyPropertyParams.FindPropertyContainer(ParentStructProperty);
			bFound = FindPropertySetter(*PropertyContainer, FAnimatedPropertyKey::FromStructType(ParentStructProperty->Struct), ParentStructProperty->GetName(), ParentStructProperty, ParentArrayProperty);
		}
	}

	UProperty* Property = CanKeyPropertyParams.PropertyPath.GetLeafMostProperty().Property.Get();
	if( !bFound && Property )
	{
		// the property in question is not a struct or an inner of the struct. See if it is directly keyable
		const UStruct* PropertyContainer = CanKeyPropertyParams.FindPropertyContainer(Property);
		bFound = FindPropertySetter(*PropertyContainer, FAnimatedPropertyKey::FromProperty(Property), Property->GetName());
	}

	if ( !bFound && Property )
	{
		bool bFoundValidInterp = false;
		bool bFoundEditDefaultsOnly = false;
		bool bFoundEdit = false;
		if (Property->HasAnyPropertyFlags(CPF_Interp))
		{
			bFoundValidInterp = true;
		}
		if (Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
		{
			bFoundEditDefaultsOnly = true;
		}
		if (Property->HasAnyPropertyFlags(CPF_Edit))
		{
			bFoundEdit = true;
		}

		// Valid Interp keyword is found. The property also needs to be editable.
		bFound = bFoundValidInterp && bFoundEdit && !bFoundEditDefaultsOnly;
	}

	return bFound;
}

void FSequencerObjectChangeListener::KeyProperty(FKeyPropertyParams KeyPropertyParams) const
{
	BroadcastPropertyChanged(KeyPropertyParams);
}

void FSequencerObjectChangeListener::OnObjectPreEditChange( UObject* Object, const FEditPropertyChain& PropertyChain )
{
	// We only care if we are not attempting to change properties of a CDO (which cannot be animated)
	if( Sequencer.IsValid() && !Object->HasAnyFlags(RF_ClassDefaultObject) && PropertyChain.GetActiveMemberNode() )
	{
		// Register with the property editor module that we'd like to know about animated float properties that change
		FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

		// Sometimes due to edit inline new the object is not actually the object that contains the property
		if ( IsObjectValidForListening(Object) && Object->GetClass()->HasProperty(PropertyChain.GetActiveMemberNode()->GetValue()) )
		{
			TSharedPtr<IPropertyChangeListener> PropertyChangeListener = ActivePropertyChangeListeners.FindRef( Object );

			if( !PropertyChangeListener.IsValid() )
			{
				PropertyChangeListener = PropertyEditor.CreatePropertyChangeListener();
				
				ActivePropertyChangeListeners.Add( Object, PropertyChangeListener );

				PropertyChangeListener->GetOnPropertyChangedDelegate().AddRaw( this, &FSequencerObjectChangeListener::OnPropertyChanged );

				FPropertyListenerSettings Settings;
				// Ignore array and object properties
				Settings.bIgnoreArrayProperties = true;
				Settings.bIgnoreObjectProperties = false;
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

		// We only care if we are not attempting to change properties of a CDO (which cannot be animated)
		if( Sequencer.IsValid() && bIsObjectValid && !Object->HasAnyFlags(RF_ClassDefaultObject) )
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
				
				PropertyChangeListener->GetOnPropertyChangedDelegate().AddRaw( this, &FSequencerObjectChangeListener::OnPropertyChanged );

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

			PropertyChangeListener->TriggerAllPropertiesChangedDelegate();
		}
	}
}

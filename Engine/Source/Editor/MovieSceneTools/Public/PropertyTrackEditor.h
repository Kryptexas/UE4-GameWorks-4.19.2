// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneTrackEditor.h"
#include "ISequencerObjectChangeListener.h"

class IPropertyHandle;
class FPropertyChangedParams;

/**
 * Tools for animatable property types such as floats ands vectors
 */
 template<typename TrackType, typename KeyType>
class FPropertyTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	* Constructor
	*
	* @param InSequencer The sequencer instance to be used by this tool
	*/
	FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer )
		: FMovieSceneTrackEditor( InSequencer )
	{ }

	/**
	* Constructor
	*
	* @param InSequencer The sequencer instance to be used by this tool
	* @param WatchedPropertyTypeName The property type name that this property track editor should watch for changes.
	*/
	FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer, FName WatchedPropertyTypeName )
		: FMovieSceneTrackEditor( InSequencer )
	{
		AddWatchedPropertyType( WatchedPropertyTypeName );
	}

	/**
	* Constructor
	*
	* @param InSequencer The sequencer instance to be used by this tool
	* @param WatchedPropertyTypeName1 The first property type name that this property track editor should watch for changes.
	* @param WatchedPropertyTypeName2 The second property type name that this property track editor should watch for changes.
	*/
	FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer, FName WatchedPropertyTypeName1, FName WatchedPropertyTypeName2 )
		: FMovieSceneTrackEditor( InSequencer )
	{
		AddWatchedPropertyType( WatchedPropertyTypeName1 );
		AddWatchedPropertyType( WatchedPropertyTypeName2 );
	}

	/**
	* Constructor
	*
	* @param InSequencer The sequencer instance to be used by this tool
	* @param WatchedPropertyTypeName1 The first property type name that this property track editor should watch for changes.
	* @param WatchedPropertyTypeName2 The second property type name that this property track editor should watch for changes.
	* @param WatchedPropertyTypeName3 The third property type name that this property track editor should watch for changes.
	*/
	FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer, FName WatchedPropertyTypeName1, FName WatchedPropertyTypeName2, FName WatchedPropertyTypeName3 )
		: FMovieSceneTrackEditor( InSequencer )
	{
		AddWatchedPropertyType( WatchedPropertyTypeName1 );
		AddWatchedPropertyType( WatchedPropertyTypeName2 );
		AddWatchedPropertyType( WatchedPropertyTypeName3 );
	}

	/**
	* Constructor
	*
	* @param InSequencer The sequencer instance to be used by this tool
	* @param WatchedPropertyTypeNameS An array of property type names that this property track editor should watch for changes.
	*/
	FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer, TArray<FName> InWatchedPropertyTypeNames )
		: FMovieSceneTrackEditor( InSequencer )
	{
		for ( FName WatchedPropertyTypeName : InWatchedPropertyTypeNames )
		{
			AddWatchedPropertyType( WatchedPropertyTypeName );
		}
	}

	~FPropertyTrackEditor()
	{
		TSharedPtr<ISequencer> Sequencer = GetSequencer();
		if ( Sequencer.IsValid() )
		{
			ISequencerObjectChangeListener& ObjectChangeListener = Sequencer->GetObjectChangeListener();
			for ( FName WatchedPropertyTypeName : WatchedPropertyTypeNames )
			{
				ObjectChangeListener.GetOnAnimatablePropertyChanged( WatchedPropertyTypeName ).RemoveAll( this );
			}
		}
	}

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override
	{
		return Type == TrackType::StaticClass();
	}

	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL ) override
	{
		ISequencerObjectChangeListener& ObjectChangeListener = GetSequencer()->GetObjectChangeListener();

		TArray<UObject*> OutObjects;
		GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneSequenceInstance(), ObjectGuid, OutObjects );
		for ( int32 i = 0; i < OutObjects.Num(); ++i )
		{
			ObjectChangeListener.TriggerAllPropertiesChanged( OutObjects[i] );
		}
	}

protected:
	/**
	 * Tries to generate a key for the track based on a property change event.  This will be called when a property changes
	 * which matches a property type name which was supplied on construction.
	 *
	 * @param PropertyChangedParams Parameters associated with the property change.
	 * @param OutKey If this call is successful this represents the key which was generated.
	 *
	 * @return true if generating a key was successful, otherwise false.
	 */
	virtual bool TryGenerateKeyFromPropertyChanged( const FPropertyChangedParams& PropertyChangedParams, KeyType& OutKey ) = 0;
	
private:
	/** Adds a callback for property changes for the supplied property type name. */
	void AddWatchedPropertyType( FName WatchedPropertyTypeName )
	{
		GetSequencer()->GetObjectChangeListener().GetOnAnimatablePropertyChanged( WatchedPropertyTypeName ).AddRaw( this, &FPropertyTrackEditor<TrackType, KeyType>::OnAnimatedPropertyChanged );
		WatchedPropertyTypeNames.Add( WatchedPropertyTypeName );
	}

	/**
	* Called by the details panel when an animatable property changes
	*
	* @param InObjectsThatChanged	List of objects that changed
	* @param KeyPropertyParams		Parameters for the property change.
	*/
	virtual void OnAnimatedPropertyChanged( const FPropertyChangedParams& PropertyChangedParams )
	{
		// Get the value from the property
		KeyType Key;
		if ( TryGenerateKeyFromPropertyChanged( PropertyChangedParams, Key ) )
		{
			AnimatablePropertyChanged( TrackType::StaticClass(), PropertyChangedParams.bRequireAutoKey,
				FOnKeyProperty::CreateRaw( this, &FPropertyTrackEditor::OnKeyProperty, PropertyChangedParams, Key ) );
		}
	}

	/** Adds a key based on a property change. */
	void OnKeyProperty( float KeyTime, FPropertyChangedParams PropertyChangedParams, KeyType Key )
	{
		for ( UObject* Object : PropertyChangedParams.ObjectsThatChanged )
		{
			FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
			if ( ObjectHandle.IsValid() )
			{
				FName PropertyName = PropertyChangedParams.PropertyPath.Last()->GetFName();

				// Look for a customized track class for this property on the meta data
				const FString& MetaSequencerTrackClass = PropertyChangedParams.PropertyPath.Last()->GetMetaData( TEXT( "SequencerTrackClass" ) );
				TSubclassOf<UMovieSceneTrack> SequencerTrackClass = TrackType::StaticClass();
				if ( !MetaSequencerTrackClass.IsEmpty() )
				{
					UClass* MetaClass = FindObject<UClass>( ANY_PACKAGE, *MetaSequencerTrackClass );
					if ( !MetaClass )
					{
						MetaClass = LoadObject<UClass>( nullptr, *MetaSequencerTrackClass );
					}
					if ( MetaClass != NULL )
						SequencerTrackClass = MetaClass;
				}

				UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, SequencerTrackClass, PropertyName );
				if ( ensure( Track ) )
				{
					TrackType* TypedTrack = CastChecked<TrackType>( Track );

					TypedTrack->SetPropertyNameAndPath( PropertyName, PropertyChangedParams.GetPropertyPathString() );
					// Find or add a new section at the auto-key time and changing the property same property
					// AddKeyToSection is not actually a virtual, it's redefined in each class with a different type
					bool bSuccessfulAdd = TypedTrack->AddKeyToSection( KeyTime, Key );
					if ( bSuccessfulAdd )
					{
						TypedTrack->SetAsShowable();
					}
				}
			}
		}
	}

private:
	/** An array of property type names which are being watched for changes. */
	TArray<FName> WatchedPropertyTypeNames;
};



// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

//@todo Sequencer - these have to be here for now because this class contains a small amount of implementation inside this header to avoid exporting this class
#include "ISequencer.h"
#include "MovieScene.h"
#include "ScopedTransaction.h"
#include "MovieSceneSequence.h"

class ISequencerSection;

DECLARE_DELEGATE_OneParam(FOnKeyProperty, float)
DECLARE_DELEGATE_RetVal_OneParam(bool, FCanKeyProperty, float)

/**
 * Base class for handling key and section drawing and manipulation for a UMovieSceneTrack class
 * @todo Sequencer - Interface needs cleanup
 */
class FMovieSceneTrackEditor : public TSharedFromThis<FMovieSceneTrackEditor>
{
public:
	/** Constructor */
	FMovieSceneTrackEditor( TSharedRef<ISequencer> InSequencer )
		: Sequencer( InSequencer )
	{}

	/** Destructor */
	virtual ~FMovieSceneTrackEditor(){};

	/** Called when the instance of this track editor is released */
	virtual void OnRelease(){};

	/** @return The current movie scene */
	UMovieSceneSequence* GetMovieSceneSequence() const
	{
		return Sequencer.Pin()->GetFocusedMovieSceneSequence();
	}

	/**
	 * Gets the movie scene that should be used for adding new keys/sections to and the time where new keys should be added
	 *
	 * @param InMovieScene	The movie scene to be used for keying
	 * @param OutTime		The current time of the sequencer which should be used for adding keys during auto-key
	 * @return true	if we can auto-key
	 */
	float GetTimeForKey( UMovieSceneSequence* InMovieSceneSequence )
	{ 
		return Sequencer.Pin()->GetCurrentLocalTime( *InMovieSceneSequence );
	}

	/** Gets whether the tool can legally autokey */
	virtual bool IsAllowedToAutoKey() const
	{
		// @todo sequencer livecapture: This turns on "auto key" for the purpose of capture keys for actor state
		// during PIE sessions when record mode is active.
		return Sequencer.Pin()->IsRecordingLive() || Sequencer.Pin()->GetAutoKeyEnabled();
	}

	/** Gets whether the tool can key all*/
	virtual bool IsAllowedKeyAll() const
	{
		return Sequencer.Pin()->GetKeyAllEnabled();
	}

	void NotifyMovieSceneDataChanged()
	{
		TSharedPtr<ISequencer> SequencerPin = Sequencer.Pin();
		if( SequencerPin.IsValid()  )
		{
			SequencerPin->NotifyMovieSceneDataChanged();
		}
	}
	
	bool CanKeyProperty( FCanKeyProperty InCanKeyProperty )
	{
		UMovieSceneSequence* MovieSceneSequence = GetMovieSceneSequence();
		check( MovieSceneSequence );

		float KeyTime = GetTimeForKey(MovieSceneSequence);

		return InCanKeyProperty.Execute(KeyTime);
	}
	
	void AnimatablePropertyChanged( TSubclassOf<class UMovieSceneTrack> TrackClass, FOnKeyProperty OnKeyProperty )
	{
		check(OnKeyProperty.IsBound());

		// Get the movie scene we want to autokey
		UMovieSceneSequence* MovieSceneSequence = GetMovieSceneSequence();
		float KeyTime = GetTimeForKey( MovieSceneSequence );

		if( !Sequencer.Pin()->IsRecordingLive() )
		{
			check( MovieSceneSequence );

			// @todo Sequencer - The sequencer probably should have taken care of this
			MovieSceneSequence->SetFlags(RF_Transactional);
		
			// Create a transaction record because we are about to add keys
			const bool bShouldActuallyTransact = !Sequencer.Pin()->IsRecordingLive();		// Don't transact if we're recording in a PIE world.  That type of keyframe capture cannot be undone.
			FScopedTransaction AutoKeyTransaction( NSLOCTEXT("AnimatablePropertyTool", "PropertyChanged", "Animatable Property Changed"), bShouldActuallyTransact );

			OnKeyProperty.ExecuteIfBound( KeyTime );

			// Movie scene data has changed
			NotifyMovieSceneDataChanged();
		}
	}
	
	/**
	 * Finds or creates a binding to an object
	 *
	 * @param Object	The object to create a binding for
	 * @return A handle to the binding or an invalid handle if the object could not be bound
	 */
	FGuid FindOrCreateHandleToObject( UObject* Object, bool bCreateHandleIfMissing = true )
	{
		return GetSequencer()->GetHandleToObject( Object, bCreateHandleIfMissing );
	}

	UMovieSceneTrack* GetTrackForObject( const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName, bool bCreateTrackIfMissing = true )
	{
		check( UniqueTypeName != NAME_None );

		UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();

		UMovieSceneTrack* Type = MovieScene->FindTrack( TrackClass, ObjectHandle, UniqueTypeName );

		if( !Type && bCreateTrackIfMissing )
		{
			Type = AddTrack( MovieScene, ObjectHandle, TrackClass, UniqueTypeName );
		}

		return Type;
	}

	virtual UMovieSceneTrack* AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName)
	{
		return FocusedMovieScene->AddTrack(TrackClass, ObjectHandle);
	}

	UMovieSceneTrack* GetMasterTrack( TSubclassOf<UMovieSceneTrack> TrackClass )
	{
		UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
		UMovieSceneTrack* Type = MovieScene->FindMasterTrack( TrackClass );

		if( !Type )
		{
			Type = MovieScene->AddMasterTrack( TrackClass );
		}

		return Type;
	}


	/** @return The sequencer bound to this handler */
	const TSharedPtr<ISequencer> GetSequencer() const { return Sequencer.Pin(); }
	
	/**
	 * Returns whether a track class is supported by this tool
	 *
	 * @param TrackClass	The track class that could be supported
	 * @return true if the type is supported
	 */
	virtual bool SupportsType( TSubclassOf<class UMovieSceneTrack> TrackClass ) const = 0;

	/**
	 * Called to generate a section layout for a particular section
	 *
	 * @param SectionObject	The section to make UI for
	 * @param SectionName	The name of the section
	 */
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( class UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) = 0;

	/**
	 * Manually adds a key

	 * @param ObjectGuid		The Guid of the object that we are adding a key to
	 * @param AdditionalAsset	An optional asset that can be added with the key
	 */
	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL ) {}

	/** Ticks this tool */
	virtual void Tick(float DeltaTime) {}
	
	/**
	 * Called when an asset is dropped into Sequencer. Can potentially consume the asset
	 * so it doesn't get added as a spawnable
	 *
	 * @param Asset				The asset that is dropped in
	 * @param TargetObjectGuid	The object guid this asset is dropped onto, if applicable
	 * @return					True, if we want to consume this asset
	 */
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) {return false;}
	
	/**
	 * Allows the track editors to bind commands
	 *
	 * @param SequencerCommandBindings	The command bindings to map to
	*/
	virtual void BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings) {}

	/**
	 * Builds up the object binding context menu for the outliner
	 *
	 * @param MenuBuilder	The menu builder to change
	 * @param ObjectBinding	The object binding this is for
	 * @param ObjectClass	The class of the object this is for
	 */
	virtual void BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) {}

	/**
	 * Builds up the object binding track menu for the outliner
	 *
	 * @param MenuBuilder	The menu builder to change
	 * @param ObjectBinding	The object binding this is for
	 * @param ObjectClass	The class of the object this is for
	 */
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) {}

	/**
	 * Builds up the object binding edit buttons for the outliner
	 *
	 * @param EditBox	    The edit box to add buttons to
	 * @param ObjectBinding	The object binding this is for
	 * @param ObjectClass	The class of the object this is for
	 */
	virtual void BuildObjectBindingEditButtons(TSharedPtr<SHorizontalBox> EditBox, const FGuid& ObjectBinding, const UClass* ObjectClass) {}

	/**
	 * Builds an edit widget for the outliner nodes which represent tracks which are edited by this editor.
	 * @param ObjectBinding The object binding associated with the track being edited by this editor.
	 * @param Track The track being edited by this editor.
	 * @returns The the widget to display in the outliner, or an empty shared ptr if not widget is to be displayed.
	 */
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track) { return TSharedPtr<SWidget>(); }

private:
	/** The sequencer bound to this handler.  Used to access movie scene and time info during auto-key */
	TWeakPtr<ISequencer> Sequencer;
};


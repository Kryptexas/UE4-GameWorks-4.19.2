// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

//@todo Sequencer - these have to be here for now because this class contains a small amount of implementation inside this header to avoid exporting this class
#include "ISequencer.h"
#include "ISequencerSection.h"
#include "ISequencerTrackEditor.h"
#include "MovieScene.h"
#include "ScopedTransaction.h"
#include "MovieSceneSequence.h"


class ISequencerSection;


DECLARE_DELEGATE_OneParam(FOnKeyProperty, float)
DECLARE_DELEGATE_RetVal_OneParam(bool, FCanKeyProperty, float)


/**
 * Base class for handling key and section drawing and manipulation
 * of a UMovieSceneTrack class.
 *
 * @todo Sequencer Interface needs cleanup
 */
class FMovieSceneTrackEditor
	: public TSharedFromThis<FMovieSceneTrackEditor>
	, public ISequencerTrackEditor
{
public:

	/** Constructor */
	FMovieSceneTrackEditor(TSharedRef<ISequencer> InSequencer)
		: Sequencer(InSequencer)
	{ }

	/** Destructor */
	virtual ~FMovieSceneTrackEditor() { };

public:

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

	/**
	 * Find or add a master track of the specified type in the focused movie scene.
	 *
	 * @param TrackClass The class of the track to find or add.
	 * @return The track.
	 */
	template<typename TrackClass>
	TrackClass* FindOrAddMasterTrack()
	{
		UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
		TrackClass* MasterTrack = MovieScene->FindMasterTrack<TrackClass>();

		if (MasterTrack == nullptr)
		{
			MasterTrack = MovieScene->AddMasterTrack<TrackClass>();
		}

		return MasterTrack;
	}


	/** @return The sequencer bound to this handler */
	const TSharedPtr<ISequencer> GetSequencer() const
	{
		return Sequencer.Pin();
	}

public:

	// ISequencerTrackEditor interface

	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = nullptr ) override { }

	virtual UMovieSceneTrack* AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName) override
	{
		return FocusedMovieScene->AddTrack(TrackClass, ObjectHandle);
	}

	virtual void BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings) override { }
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override { }
	virtual void BuildObjectBindingEditButtons(TSharedPtr<SHorizontalBox> EditBox, const FGuid& ObjectBinding, const UClass* ObjectClass) override { }
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) override { }
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track) override  { return TSharedPtr<SWidget>(); }
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override { return false; }

	virtual bool IsAllowedKeyAll() const
	{
		return Sequencer.Pin()->GetKeyAllEnabled();
	}

	virtual bool IsAllowedToAutoKey() const
	{
		// @todo sequencer livecapture: This turns on "auto key" for the purpose of capture keys for actor state
		// during PIE sessions when record mode is active.
		return Sequencer.Pin()->IsRecordingLive() || Sequencer.Pin()->GetAutoKeyEnabled();
	}

	virtual TSharedRef<ISequencerSection> MakeSectionInterface(class UMovieSceneSection& SectionObject, UMovieSceneTrack& Track) = 0;
	virtual void OnRelease() override { };

	virtual int32 PaintTrackArea(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) override
	{
		return LayerId;
	}

	virtual bool SupportsType( TSubclassOf<class UMovieSceneTrack> TrackClass ) const = 0;
	virtual void Tick(float DeltaTime) override { }

protected:

	/**
	 * Gets the currently focused movie scene, if any.
	 *
	 * @result Focused movie scene, or nullptr if no movie scene is focused.
	 */
	UMovieScene* GetFocusedMovieScene() const
	{
		UMovieSceneSequence* FocusedSequence = GetSequencer()->GetFocusedMovieSceneSequence();
		return FocusedSequence->GetMovieScene();
	}

private:

	/** The sequencer bound to this handler.  Used to access movie scene and time info during auto-key */
	TWeakPtr<ISequencer> Sequencer;
};

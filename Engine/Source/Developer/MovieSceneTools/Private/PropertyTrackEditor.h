// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IPropertyHandle;

class FPropertyChangedParams;

/**
 * Tools for animatable property types such as floats ands vectors
 */
class FPropertyTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	FPropertyTrackEditor( TSharedRef<ISequencer> InSequencer );
	~FPropertyTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<FMovieSceneTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) override;
	virtual void AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL) override;
	virtual UMovieSceneTrack* AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName) override;

private:
	/**
	 * Called by the details panel when an animatable property changes
	 *
	 * @param InObjectsThatChanged	List of objects that changed
	 * @param KeyPropertyParams		Parameters for the property change.
	 */
	template <typename Type, typename TrackType>
	void OnAnimatedPropertyChanged( const FPropertyChangedParams& KeyPropertyParams );

	/**
	 * Called by the details panel when an animatable vector property changes
	 *
	 * @param InObjectsThatChanged	List of objects that changed
	 * @param KeyPropertyParams		Parameters for the property change.
	 */
	void OnAnimatedVectorPropertyChanged( const FPropertyChangedParams& KeyPropertyParams );
	
	/**
	 * Called by the details panel when an animatable color property changes
	 *
	 * @param InObjectsThatChanged	List of objects that changed
	 * @param KeyPropertyParams		Parameters for the property change.
	 */
	void OnAnimatedColorPropertyChanged( const FPropertyChangedParams& KeyPropertyParams );
	
	/** Delegates for animatable property changed functions */
	template <typename Type, typename TrackType>
	void OnKeyProperty( float KeyTime, FPropertyChangedParams KeyPropertyParams, Type Value );
};



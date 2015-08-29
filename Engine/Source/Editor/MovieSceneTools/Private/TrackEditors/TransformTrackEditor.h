// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene3DTransformTrack.h"

/**
 * Tools for animatable transforms
 */
class F3DTransformTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	F3DTransformTrackEditor( TSharedRef<ISequencer> InSequencer );
	~F3DTransformTrackEditor();

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
	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL ) override;
	virtual void BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings) override;
	virtual void BuildObjectBindingEditButtons(TSharedPtr<SHorizontalBox> EditBox, const FGuid& ObjectBinding, const UClass* ObjectClass) override;

private:

	/** Custom add key implementation */
	void AddKeyInternal(const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL, bool bForceKey = false, F3DTransformTrackKey::Type KeyType = F3DTransformTrackKey::Key_All);

	/**
	 * Called before an actor or component transform changes
	 *
	 * @param Object	The object whose transform is about to change
	 */
	void OnPreTransformChanged( UObject& InObject );

	/**
	 * Called when an actor or component transform changes
	 *
	 * @param Object	The object whose transform has changed
	 */
	void OnTransformChanged( UObject& InObject );

	/** Delegate for animatable property changed in OnTransformChanged */
	void OnTransformChangedInternals(float KeyTime, UObject* InObject, FGuid ObjectHandle, struct FTransformDataPair TransformPair, bool bAutoKeying, bool bForceKey, F3DTransformTrackKey::Type KeyType);

	/** Delegate for camera button lock state */
	ECheckBoxState IsCameraLocked(TWeakObjectPtr<ACameraActor> CameraActor) const; 

	/** Delegate for locked camera button */
	void OnLockCameraClicked(ECheckBoxState CheckBoxState, TWeakObjectPtr<ACameraActor> CameraActor);

	/** Delegate for camera button lock tooltip */
	FText GetLockCameraToolTip(TWeakObjectPtr<ACameraActor> CameraActor) const; 

	/** Add a transform key */
	void AddTransformKey();

	/** Add a translation key */
	void AddTranslationKey();

	/** Add a rotation key */
	void AddRotationKey();

	/** Add a scale key */
	void AddScaleKey();

	/** Internal add transform key */
	void AddTransformKeyInternal(F3DTransformTrackKey::Type KeyType = F3DTransformTrackKey::Key_All);

private:
	/** Mapping of objects to their existing transform data (for comparing against new transform data) */
	TMap< TWeakObjectPtr<UObject>, FTransformData > ObjectToExistingTransform;
};



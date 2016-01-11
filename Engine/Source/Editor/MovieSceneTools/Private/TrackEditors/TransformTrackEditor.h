// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene3DTransformTrack.h"


/**
 * Tools for animatable transforms
 */
class F3DTransformTrackEditor
	: public FMovieSceneTrackEditor
{
public:

	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	F3DTransformTrackEditor( TSharedRef<ISequencer> InSequencer );

	/** Virtual destructor. */
	virtual ~F3DTransformTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

public:

	// ISequencerTrackEditor interface

	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL ) override;
	virtual void BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings) override;
	virtual void BuildObjectBindingEditButtons(TSharedPtr<SHorizontalBox> EditBox, const FGuid& ObjectBinding, const UClass* ObjectClass) override;
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track ) override;
	virtual void OnRelease() override;
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;

private:

	/** Add a transform track */
	void AddTransform(FGuid ObjectGuid);

	/** Called to determine whether a transform track can be added */
	bool CanAddTransform(FGuid ObjectGuid) const;

	/** Custom add key implementation */
	void AddKeyInternal(const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL, const bool bAddKeyEvenIfUnchanged = false, F3DTransformTrackKey::Type KeyType = F3DTransformTrackKey::Key_All);

	/**
	 * Called before an actor or component transform changes
	 *
	 * @param Object The object whose transform is about to change
	 */
	void OnPreTransformChanged( UObject& InObject );

	/**
	 * Called when an actor or component transform changes
	 *
	 * @param Object The object whose transform has changed
	 */
	void OnTransformChanged( UObject& InObject );

	/** Get transform key */
	FTransformKey GetTransformKey(const UMovieScene3DTransformTrack* TransformTrack, float KeyTime, struct FTransformDataPair TransformPair, FKeyParams KeyParams) const;

	/** Delegate for animatable property changed in OnTransformChanged */
	void OnTransformChangedInternals(float KeyTime, UObject* InObject, FGuid ObjectHandle, struct FTransformDataPair TransformPair, FKeyParams KeyParams, F3DTransformTrackKey::Type KeyType);

	/** Delegate to determine whether the property can be keyed */
	bool CanKeyPropertyInternal(float KeyTime, FGuid ObjectHandle, struct FTransformDataPair TransformPair, FKeyParams KeyParams) const;

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

// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Curves/KeyHandle.h"
#include "MovieSceneSection.h"
#include "SequencerObjectVersion.h"
#include "MovieSceneObjectBindingID.h"
#include "MovieSceneCameraCutSection.generated.h"


/**
 * Movie CameraCuts are sections on the CameraCuts track, that show what the viewer "sees"
 */
UCLASS(MinimalAPI)
class UMovieSceneCameraCutSection 
	: public UMovieSceneSection
{
	GENERATED_BODY()

public:
	UMovieSceneCameraCutSection(const FObjectInitializer& Init)
		: Super(Init)
	{
		EvalOptions.EnableAndSetCompletionMode
			(GetLinkerCustomVersion(FSequencerObjectVersion::GUID) < FSequencerObjectVersion::WhenFinishedDefaultsToProjectDefault ? 
				EMovieSceneCompletionMode::RestoreState : 
				EMovieSceneCompletionMode::ProjectDefault);
	}

	DEPRECATED(4.18, "Camera guid no longer supported, Use GetCameraBindingID.")
	FGuid GetCameraGuid() const
	{
		return FGuid();
	}

	/** Sets the camera binding for this CameraCut section. Evaluates from the sequence binding ID */
	void SetCameraGuid(const FGuid& InGuid)
	{
		SetCameraBindingID(FMovieSceneObjectBindingID(InGuid, MovieSceneSequenceID::Root, EMovieSceneObjectBindingSpace::Local));
	}

	/** Gets the camera binding for this CameraCut section */
	const FMovieSceneObjectBindingID& GetCameraBindingID() const
	{
		return CameraBindingID;
	}

	/** Sets the camera binding for this CameraCut section */
	void SetCameraBindingID(const FMovieSceneObjectBindingID& InCameraBindingID)
	{
		CameraBindingID = InCameraBindingID;
	}

	virtual FMovieSceneEvalTemplatePtr GenerateTemplate() const override;

	// UMovieSceneSection interface
	virtual TOptional<float> GetKeyTime(FKeyHandle KeyHandle) const override { return TOptional<float>(); }
	virtual void SetKeyTime(FKeyHandle KeyHandle, float Time) override { }
	virtual void OnBindingsUpdated(const TMap<FGuid, FGuid>& OldGuidToNewGuidMap) override;

	/** ~UObject interface */
	virtual void PostLoad() override;

private:

	/** The camera possessable or spawnable that this movie CameraCut uses */
	UPROPERTY()
	FGuid CameraGuid_DEPRECATED;

	/** The camera binding that this movie CameraCut uses */
	UPROPERTY(EditAnywhere, Category="Section")
	FMovieSceneObjectBindingID CameraBindingID;

#if WITH_EDITORONLY_DATA
public:
	/** @return The thumbnail reference frame offset from the start of this section */
	float GetThumbnailReferenceOffset() const
	{
		return ThumbnailReferenceOffset;
	}

	/** Set the thumbnail reference offset */
	void SetThumbnailReferenceOffset(float InNewOffset)
	{
		Modify();
		ThumbnailReferenceOffset = InNewOffset;
	}

private:

	/** The reference frame offset for single thumbnail rendering */
	UPROPERTY()
	float ThumbnailReferenceOffset;
#endif
};

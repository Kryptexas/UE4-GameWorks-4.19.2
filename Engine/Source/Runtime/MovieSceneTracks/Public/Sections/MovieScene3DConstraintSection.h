// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Curves/KeyHandle.h"
#include "MovieSceneSection.h"
#include "MovieSceneObjectBindingID.h"
#include "MovieScene3DConstraintSection.generated.h"


/**
 * Base class for 3D constraint section
 */
UCLASS(MinimalAPI)
class UMovieScene3DConstraintSection
	: public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()

public:

	/** Sets the constraint id for this section */
	virtual void SetConstraintId(const FGuid& InId);

	DEPRECATED(4.18, "Constraint guid no longer supported, Use GetConstraintBindingID.")
	virtual FGuid GetConstraintId() const;

	/** Gets the constraint binding for this Constraint section */
	const FMovieSceneObjectBindingID& GetConstraintBindingID() const
	{
		return ConstraintBindingID;
	}

	/** Sets the constraint binding for this Constraint section */
	void SetConstraintBindingID(const FMovieSceneObjectBindingID& InConstraintBindingID)
	{
		ConstraintBindingID = InConstraintBindingID;
	}

public:

	//~ UMovieSceneSection interface

	virtual TOptional<float> GetKeyTime(FKeyHandle KeyHandle) const override;
	virtual void SetKeyTime(FKeyHandle KeyHandle, float Time) override;
	virtual void OnBindingsUpdated(const TMap<FGuid, FGuid>& OldGuidToNewGuidMap) override;
	
	/** ~UObject interface */
	virtual void PostLoad() override;

protected:

	/** The possessable guid that this constraint uses */
	UPROPERTY()
	FGuid ConstraintId_DEPRECATED;

	/** The constraint binding that this movie Constraint uses */
	UPROPERTY(EditAnywhere, Category="Section")
	FMovieSceneObjectBindingID ConstraintBindingID;

};

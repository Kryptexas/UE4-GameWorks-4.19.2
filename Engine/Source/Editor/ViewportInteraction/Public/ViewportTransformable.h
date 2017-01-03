// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Represents an object that we're actively interacting with, such as a selected actor
 */
class VIEWPORTINTERACTION_API FViewportTransformable
{

public:

	/** Sets up safe defaults */
	FViewportTransformable()
		: StartTransform( FTransform::Identity ),
		  LastTransform( FTransform::Identity ),
		  TargetTransform( FTransform::Identity ),
		  UnsnappedTargetTransform( FTransform::Identity ),
		  InterpolationSnapshotTransform( FTransform::Identity )
	{
	}

	/** Gets the current transform of this object */
	virtual const FTransform GetTransform() const = 0;

	/** Updates the transform of the actual object */
	virtual void ApplyTransform( const FTransform& NewTransform, const bool bSweep ) = 0;

	/** Returns the bounding box of this transformable in local space */
	virtual FBox GetLocalSpaceBoundingBox() const = 0;

	/** Returns true if this transformable is a physically simulated kinematic object */
	virtual bool IsPhysicallySimulated() const
	{
		return false;
	}

	/** For physically simulated objects, sets the new velocity of the object */
	virtual void SetLinearVelocity( const FVector& NewVelocity )
	{
	}

	/** For actor transformables, this will add it's actor to the incoming list */
	virtual void UpdateIgnoredActorList( TArray<class AActor*>& IgnoredActors )
	{
	}


	/** The object's position when we started the action */
	FTransform StartTransform;

	/** The last position we were at.  Used for smooth interpolation when snapping is enabled */
	FTransform LastTransform;

	/** The target position we want to be at.  Used for smooth interpolation when snapping is enabled */
	FTransform TargetTransform;

	/** The target position we'd be at if snapping wasn't enabled.  This is used for 'elastic' smoothing when
	snapping is turned on.  Basically it allows us to 'reach' toward the desired position */
	FTransform UnsnappedTargetTransform;

	/** A "snapshot" transform, used for certain interpolation effects */
	FTransform InterpolationSnapshotTransform;
};


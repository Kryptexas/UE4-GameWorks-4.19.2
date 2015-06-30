// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindings.generated.h"

/**
 * Identifies a bound object and an optional tag for sub-object lookup.
 */
USTRUCT()
struct FMovieSceneBoundObjectInfo
{
	GENERATED_USTRUCT_BODY( FMovieSceneBoundObjectInfo )

public:
	FMovieSceneBoundObjectInfo()
	{
		Object = nullptr;
	}

	FMovieSceneBoundObjectInfo(UObject* InObject)
	{
		Object = InObject;
	}

	FMovieSceneBoundObjectInfo(UObject* InObject, FString InTag)
	{
		Object = InObject;
		Tag = InTag;
	}

	inline bool operator==(const FMovieSceneBoundObjectInfo& Other) const
	{
		return (Object == Other.Object) && (Tag == Other.Tag);
	}

	UPROPERTY()
	UObject* Object;

	UPROPERTY()
	FString Tag;
};

/**
 * MovieSceneBoundObject connects an object to a "possessable" slot in the MovieScene asset
 */
USTRUCT()
struct FMovieSceneBoundObject
{
	GENERATED_USTRUCT_BODY( FMovieSceneBoundObject )

public:

	/** FMovieSceneBoundObject default constructor */
	FMovieSceneBoundObject()
	{
	}

	/**
	 * FMovieSceneBoundObject initialization constructor.  Binds the specified possessable from a MovieScene asset, to the specified object
	 *
	 * @param	InitPossessableGuid		The Guid of the possessable from the MovieScene asset to bind to
	 * @param	InitObjectInfos			The object infos to bind
	 */
	FMovieSceneBoundObject( const FGuid& InitPosessableGuid, const TArray< FMovieSceneBoundObjectInfo >& InitObjectInfos );

	/** @return Returns the guid of the possessable that we're bound to */
	const FGuid& GetPossessableGuid() const
	{
		return PossessableGuid;
	}

	/** @return Returns the objects we're bound to */
	TArray< FMovieSceneBoundObjectInfo >& GetObjectInfos()
	{
		return ObjectInfos;
	}

	/** @return Returns the objects we're bound to */
	const TArray< FMovieSceneBoundObjectInfo >& GetObjectInfos() const
	{
		return ObjectInfos;
	}

	/**
	 * Sets the actual objects bound to this slot.  Remember to call Modify() on the owner object if you change this!
	 * 
	 * @param	NewObjectInfos	The list of new bound object infos (can be NULL)
	 */
	void SetObjectInfos( TArray< FMovieSceneBoundObjectInfo > NewObjectInfos )
	{
		ObjectInfos = NewObjectInfos;
	}

private:

	/** Guid of the possessable in the MovieScene that we're binding to.  Note that we always want this GUID to be preserved
	    when the object is duplicated (copy should have same relationship with possessed object.) */
	UPROPERTY()
	FGuid PossessableGuid;

	/** The actual bound objects (we support binding multiple objects to the same possessable) */
	UPROPERTY()
	TArray< FMovieSceneBoundObjectInfo > ObjectInfos;

};


/**
 * MovieSceneBindings.  Contains the relationship between MovieScene possessables and actual objects being possessed.
 */
UCLASS( MinimalAPI )
class UMovieSceneBindings : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Associates a MovieScene asset with these bindings
	 *
	 * @param	NewMovieScene	The MovieScene asset to use (can be NULL to clear it)
	 */
	virtual void SetRootMovieScene( class UMovieScene* NewMovieScene );

	/** @return	Returns the root MovieScene associated with these bindings (or NULL, if no MovieScene is associated yet) */
	virtual class UMovieScene* GetRootMovieScene();

	/**
	 * Adds a binding between the specified possessable from a MovieScene asset, to the specified objects
	 *
	 * @param	PossessableGuid		The Guid of the possessable from the MovieScene asset to bind to
	 * @param	Objects				The objects to bind
	 *
	 * @return	Reference to the wrapper structure for this bound object
	 */
	virtual FMovieSceneBoundObject& AddBinding( const FGuid& PosessableGuid, const TArray< FMovieSceneBoundObjectInfo >& ObjectInfos );

	/**
	 * Given a guid for a possessable, attempts to find the object bound to that guid
	 *
	 * @param	Guid	The possessable guid to search for
	 *
	 * @return	The bound object, if found, otherwise NULL
	 */
	virtual TArray< FMovieSceneBoundObjectInfo > FindBoundObjects( const FGuid& Guid ) const;

	/**
	 * Gets all bound objects
	 *
	 * @return	List of all bound objects.
	 */
	virtual TArray< FMovieSceneBoundObject >& GetBoundObjects();


private:

	/** The Root MovieScene asset to play */
	UPROPERTY()
	class UMovieScene* RootMovieScene;

	/** List of all of the objects that we're bound to */
	UPROPERTY()
	TArray< FMovieSceneBoundObject > BoundObjects;
};



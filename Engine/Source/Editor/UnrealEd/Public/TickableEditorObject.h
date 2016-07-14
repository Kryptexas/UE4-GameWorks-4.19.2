// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Tickable.h"

/**
 * This class provides common registration for gamethread editor only tickable objects. It is an
 * abstract base class requiring you to implement the Tick() method.
 */
class FTickableEditorObject : public FTickableObjectBase
{
public:
	static void TickObjects( float DeltaSeconds )
	{
		const TArray<FTickableEditorObject*>& TickableObjects = GetTickableObjects();

		for( int32 ObjectIndex=0; ObjectIndex < TickableObjects.Num(); ++ObjectIndex)
		{
			FTickableEditorObject* TickableObject = TickableObjects[ObjectIndex];
			if ( TickableObject->IsTickable() )
			{
				TickableObject->Tick(DeltaSeconds);
			}
		}
	}

	/**
	 * Registers this instance with the static array of tickable objects.	
	 */
	FTickableEditorObject()
	{
		GetTickableObjects().Add( this );
	}

	/**
	 * Removes this instance from the static array of tickable objects.
	 */
	virtual ~FTickableEditorObject()
	{
		const int32 Pos = GetTickableObjects().Find(this);
		check(Pos!=INDEX_NONE);
		GetTickableObjects().RemoveAt(Pos);
	}

private:
	/** Returns the array of tickable editor objects */
	UNREALED_API static TArray<FTickableEditorObject*>& GetTickableObjects()
	{
		static TArray<FTickableEditorObject*> TickableObjects;
		return TickableObjects;
	}
};
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"


/**
 * This class provides common registration for gamethread editor only tickable objects. It is an
 * abstract base class requiring you to implement the GetStatId, IsTickable, and Tick methods.
 */
class UNREALED_API FTickableEditorObject : public FTickableObjectBase
{
public:

	static void TickObjects(const float DeltaSeconds)
	{
		TArray<FTickableEditorObject*>& PendingTickableObjects = GetPendingTickableObjects();
		TArray<FTickableObjectEntry>& TickableObjects = GetTickableObjects();

		for (FTickableEditorObject* PendingTickable : PendingTickableObjects)
		{
			AddTickableObject(TickableObjects, PendingTickable);
		}
		PendingTickableObjects.Empty();

		if (TickableObjects.Num() > 0)
		{
			check(!bIsTickingObjects);
			bIsTickingObjects = true;

			bool bNeedsCleanup = false;

			for (const FTickableObjectEntry& TickableEntry : TickableObjects)
			{
				if (FTickableObjectBase* TickableObject = TickableEntry.TickableObject)
				{
					if ((TickableEntry.TickType == ETickableTickType::Always) || TickableObject->IsTickable())
					{
						TickableObject->Tick(DeltaSeconds);
					}

					// In case it was removed during tick
					if (TickableEntry.TickableObject == nullptr)
					{
						bNeedsCleanup = true;
					}
				}
				else
				{
					bNeedsCleanup = true;
				}
			}

			if (bNeedsCleanup)
			{
				TickableObjects.RemoveAll([](const FTickableObjectEntry& Entry) { return (Entry.TickableObject == nullptr); });
			}

			bIsTickingObjects = false;
		}
	}

	/** Registers this instance with the static array of tickable objects. */
	FTickableEditorObject()
	{
		check(!GetPendingTickableObjects().Contains(this));
		check(!GetTickableObjects().Contains(this));
		GetPendingTickableObjects().Add(this);
	}

	/** Removes this instance from the static array of tickable objects. */
	virtual ~FTickableEditorObject()
	{
		if (bCollectionIntact && GetPendingTickableObjects().Remove(this) == 0)
		{
			RemoveTickableObject(GetTickableObjects(), this, bIsTickingObjects);
		}
	}

private:

	/**
	 * Class that avoids crashes when unregistering a tickable editor object too late.
	 *
	 * Some tickable objects can outlive the collection
	 * (global/static destructor order is unpredictable).
	 */
	class TTickableObjectsCollection : public TArray<FTickableObjectEntry>
	{
	public:
		~TTickableObjectsCollection()
		{
			FTickableEditorObject::bCollectionIntact = false;
		}
	};

	friend class TTickableObjectsCollection;

	/** True if collection of tickable objects is still intact. */
	static bool bCollectionIntact;
	/** True if currently ticking of tickable editor objects. */
	static bool bIsTickingObjects;

	static TArray<FTickableObjectEntry>& GetTickableObjects()
	{
		static TTickableObjectsCollection TickableObjects;
		return TickableObjects;
	}

	static TArray<FTickableEditorObject*>& GetPendingTickableObjects()
	{
		static TArray<FTickableEditorObject*> PendingTickableObjects;
		return PendingTickableObjects;
	}
};

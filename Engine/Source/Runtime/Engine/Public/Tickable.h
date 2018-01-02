// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Tickable.h: Interface for tickable objects.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"

/**
 * Enum used to convey whether a tickable object will always tick, conditionally tick, or never tick.
 */
enum class ETickableTickType : uint8
{
	/** Use IsTickable to determine whether to tick */
	Conditional,

	/** Always tick the object */
	Always,

	/** Never tick the object, do not add to tickables array */
	Never        
};

/**
 * Base class for tickable objects
 */
class FTickableObjectBase
{
protected:
	struct FTickableObjectEntry
	{
		FTickableObjectBase* TickableObject;
		ETickableTickType TickType;

		bool operator==(FTickableObjectBase* OtherObject) const { return TickableObject == OtherObject; }
	};

	static void AddTickableObject(TArray<FTickableObjectEntry>& TickableObjects, FTickableObjectBase* TickableObject)
	{
		check(!TickableObjects.Contains(TickableObject));
		const ETickableTickType TickType = TickableObject->GetTickableTickType();
		if (TickType != ETickableTickType::Never)
		{
			TickableObjects.Add({ TickableObject, TickType });
		}
	}

	static void RemoveTickableObject(TArray<FTickableObjectEntry>& TickableObjects, FTickableObjectBase* TickableObject, const bool bIsTickingObjects)
	{
		const int32 Pos = TickableObjects.IndexOfByKey(TickableObject);

#if 0 // virtual from destructor doesn't work ... need to rethink how to do warning
		// ensure that GetTickableTickType did not change over time
		switch (TickableObject->GetTickableTickType())
		{
		case ETickableTickType::Always:
			ensureMsgf(Pos != INDEX_NONE && TickableObjects[Pos].TickType == ETickableTickType::Always, TEXT("TickType has changed since object was created. Result of GetTickableTickType must be invariant for a given object."));
			break;

		case ETickableTickType::Conditional:
			ensureMsgf(Pos != INDEX_NONE && TickableObjects[Pos].TickType == ETickableTickType::Conditional, TEXT("TickType has changed since object was created. Result of GetTickableTickType must be invariant for a given object."));
			break;

		case ETickableTickType::Never:
			ensureMsgf(Pos == INDEX_NONE, TEXT("TickType has changed since object was created. Result of GetTickableTickType must be invariant for a given object."));
			break;
		}
#endif

		if (Pos != INDEX_NONE)
		{
			if (bIsTickingObjects)
			{
				TickableObjects[Pos].TickableObject = nullptr;
			}
			else
			{
				TickableObjects.RemoveAt(Pos);
			}
		}
	}

public:
	/**
	 * Pure virtual that must be overloaded by the inheriting class. It will
	 * be called from within LevelTick.cpp after ticking all actors or from
	 * the rendering thread (depending on bIsRenderingThreadObject)
	 *
	 * @param DeltaTime	Game time passed since the last call.
	 */
	virtual void Tick( float DeltaTime ) = 0;

	/**
	* Virtual that can be overloaded by the inheriting class. It is
	* used to determine whether an object will ever be able to tick, and if not,
	* it will not get added to the tickable objects array. If the tickable tick type
	* is Conditional then the virtual IsTickable will be called to determine whether
	* to tick the object on each given frame
	*
	* @return	true if object will ever want to be ticked, false otherwise.
	*/
	virtual ETickableTickType GetTickableTickType() const { return ETickableTickType::Conditional; }

	/**
	 * Virtual that can be overloaded by the inheriting class. It is
	 * used to determine whether an object is ready to be ticked. This is 
	 * required for example for all UObject derived classes as they might be
	 * loaded async and therefore won't be ready immediately.
	 *
	 * @return	true if object is ready to be ticked, false otherwise.
	 */
	virtual bool IsTickable() const { return true; }

	/** return the stat id to use for this tickable **/
	virtual TStatId GetStatId() const = 0;
};

/**
 * This class provides common registration for gamethread tickable objects. It is an
 * abstract base class requiring you to implement the Tick() method.
 */
class ENGINE_API FTickableGameObject : public FTickableObjectBase
{
private:
	/** Static array of tickable objects */
	static TArray<FTickableObjectEntry>& GetTickableObjects()
	{
		static TArray<FTickableObjectEntry> TickableObjects;
		return TickableObjects;
	}

	static TArray<FTickableGameObject*>& GetPendingTickableObjects()
	{
		static TArray<FTickableGameObject*> PendingTickableObjects;
		return PendingTickableObjects;
	}

	static bool bIsTickingObjects;

public:
	/**
	 * Registers this instance with the static array of tickable objects.	
	 *
	 */
	FTickableGameObject()
	{
		check(!GetPendingTickableObjects().Contains(this));
		check(!GetTickableObjects().Contains(this));
		GetPendingTickableObjects().Add(this);
	}

	/**
	 * Removes this instance from the static array of tickable objects.
	 */
	virtual ~FTickableGameObject()
	{
		if (GetPendingTickableObjects().Remove(this) == 0)
		{
			RemoveTickableObject(GetTickableObjects(), this, bIsTickingObjects);
		}
	}

	/**
	 * Used to determine if an object should be ticked when the game is paused.
	 * Defaults to false, as that mimics old behavior.
	 *
	 * @return true if it should be ticked when paused, false otherwise
	 */
	virtual bool IsTickableWhenPaused() const
	{
		return false;
	}

	/**
	 * Used to determine whether the object should be ticked in the editor.  Defaults to false since
	 * that is the previous behavior.
	 *
	 * @return	true if this tickable object can be ticked in the editor
	 */
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	virtual UWorld* GetTickableGameObjectWorld() const 
	{ 
		return nullptr;
	}

	static void TickObjects(UWorld* World, int32 TickType, bool bIsPaused, float DeltaSeconds);
};

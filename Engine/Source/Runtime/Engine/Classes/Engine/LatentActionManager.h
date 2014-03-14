// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LatentActionManager.generated.h"


// Latent action info
USTRUCT()
struct ENGINE_API FLatentActionInfo
{
	GENERATED_USTRUCT_BODY()

	/** The resume point within the function to execute */
	UPROPERTY(meta=(NeedsLatentFixup = true))
	int32 Linkage;

	/** the UUID for this action */ 
	UPROPERTY()
	int32 UUID;

	/** The function to execute. */ 
	UPROPERTY()
	FName ExecutionFunction;

	/** Object to execute the function on. */ 
	UPROPERTY(meta=(LatentCallbackTarget = true))
	UObject* CallbackTarget;

	FLatentActionInfo()
		: Linkage(INDEX_NONE)
		, UUID(INDEX_NONE)
		, ExecutionFunction(NAME_None)
		, CallbackTarget(NULL)
	{
	}
};


// The latent action manager handles all pending latent actions for a single world
USTRUCT()
struct ENGINE_API FLatentActionManager
{
	GENERATED_USTRUCT_BODY()

	/** Map of UUID->Action(s). */ 
	typedef TMultiMap<int32, class FPendingLatentAction*> FActionList;
	
	/** Map to convert from object to FActionList. */
	typedef TMap< TWeakObjectPtr<UObject>, FActionList > FObjectToActionListMap;
	FObjectToActionListMap ObjectToActionListMap;
public:
	/** 
	 * Advance pending latent actions by DeltaTime.
 	 * If no object is specified it will process any outstanding actions for objects that have not been processed for this frame.
	 *
	 * @param		InOject		Specific object pending action list to advance.
	 * @param		DeltaTime	Delta time.
	 *
	 */
	void ProcessLatentActions(UObject* InObject, float DeltaTime);

	/** 
	 * Finds the action instance for the supplied UUID, or will return NULL if one does not already exist.
	 *
	 * @param		InOject		ActionListType to check for pending actions.
 	 * @param		UUID		UUID of the action we are looking for.
	 *
	 */	
	template<typename ActionType>
	ActionType* FindExistingAction(UObject* InActionObject, int32 UUID)
	{
		FActionList* ObjectActionList = ObjectToActionListMap.Find( InActionObject );
		if (ObjectActionList && ObjectActionList->Num(UUID) > 0)
		{
			for(auto It = ObjectActionList->CreateKeyIterator(UUID);It;++It)
			{
				return (ActionType*)(It.Value());
			}
		}

		return NULL;
	}

	/** 
	 * Adds a new action to the action list under a given UUID 
	 */
	void AddNewAction(UObject* InActionObject, int32 UUID, FPendingLatentAction* NewAction)
	{
		FActionList& ObjectActionList = ObjectToActionListMap.FindOrAdd(InActionObject);
		ObjectActionList.Add(UUID, NewAction);
	}

	/** Resets the list of objects we have processed the latent action list for.	 */	
	void BeginFrame()
	{
		ProcessedThisFrame.Empty();
	}
#if WITH_EDITOR
	/** 
	 * Builds a set of the UUIDs of pending latent actions on a specific object.
	 *
	 * @param	InObject	Object to query for latent actions
	 * @param	UUIDList	Array to add the UUID of each pending latent action to.
	 *
	 */
	void GetActiveUUIDs(UObject* InObject, TSet<int32>& UUIDList) const;

	
	/** 
	 * Gets the description string of a pending latent action with the specified UUID for a given object, or the empty string if it's an invalid UUID
	 *
	  * @param		InOject		Object to get list of UUIDs for
	  * @param		UUIDList	Array to add UUID to.
	 *
	 */
	FString GetDescription(UObject* InObject, int32 UUID) const;
#endif

protected:
	/** 
	 * Finds the action instance for the supplied object will return NULL if one does not exist.
	 *
	 * @param		InOject		ActionListType to check for pending actions.
	 *
	 */	
	const FActionList* GetActionListForObject(UObject* InObject) const
	{
		const FActionList* ObjectActionList = ObjectToActionListMap.Find(InObject);
		return ObjectActionList;
	}

	/** 
	  * Ticks the latent action for a single UObject.
	  *
	  * @param		DeltaTime			time delta.
	  * @param		ObjectActionList	the action list for the object
	  * @param		InObject			the object itself.
	  *
	  */
	void TickLatentActionForObject(float DeltaTime, FActionList& ObjectActionList, UObject* InObject);

protected:
	/** list of objects we have processed the latent action list for this frame. */	
	TSet<UObject*> ProcessedThisFrame;
};


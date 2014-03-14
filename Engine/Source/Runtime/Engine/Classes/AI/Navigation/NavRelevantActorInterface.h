// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavRelevantActorInterface.generated.h"

UINTERFACE(/**/MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UNavRelevantActorInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class INavRelevantActorInterface
{
	GENERATED_IINTERFACE_BODY()


	///** Cached value whether this actor is relevant for navigation generation. Usually will be calculated in actor's constructor, but can be changed at runtime as well */
	//uint32 bNavigationRelevant:1;

	/** if true every navigation-colliding component of this actor will generate 
	 *	a separate entry in navoctree */
	virtual bool DoesSupplyPerComponentNavigationCollision() const { return false; }

	/** Prepare navigation modifiers and geometry exports 
	 *	@return 'true' if this instance is to be used for regular geometry export as well
	 */
	virtual bool GetNavigationRelevantData(struct FNavigationRelevantData& Data) const { return true; } 

	/** Proxy will be used instead of actor when adding/removing from navigation octree */
	virtual class UNavigationProxy* GetNavigationProxy() const { return NULL; }
};

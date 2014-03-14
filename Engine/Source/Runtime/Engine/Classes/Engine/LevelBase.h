// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// A game level.
//

#pragma once
#include "LevelBase.generated.h"

UCLASS(Abstract, customConstructor, dependsOn=UGameEngine)
class ULevelBase : public UObject
{
	GENERATED_UCLASS_BODY()

	/** URL associated with this level. */
	FURL					URL;

	/** Array of all actors in this level, used by FActorIteratorBase and derived classes */
	TTransArray<AActor*> Actors;

	// Constructors.
	ULevelBase( const class FPostConstructInitializeProperties& PCIP,const FURL& InURL );

	// Begin UObject Interface
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject Interface

	ULevelBase(const class FPostConstructInitializeProperties& PCIP)
		:	UObject(PCIP)
		,Actors( this )
	{}
};

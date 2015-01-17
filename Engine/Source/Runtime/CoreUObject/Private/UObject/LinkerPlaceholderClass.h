// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Class.h"
#include "Set.h"

// Forward declarations
class FObjectInitializer;
class UObjectPropertyBase;

/**  
 * A utility class for the deferred dependency loader, used to stub in temporary
 * class references so we don't have to load blueprint resources for their class.
 * Holds on to references where this is currently being utilized, so we can 
 * easily replace references to it later (once the real class is available).
 */ 
class ULinkerPlaceholderClass : public UClass
{
public:
	DECLARE_CASTED_CLASS_INTRINSIC_NO_CTOR(ULinkerPlaceholderClass, UClass, /*TStaticFlags =*/0, CoreUObject, /*TStaticCastFlags =*/0, NO_API)

	ULinkerPlaceholderClass(const FObjectInitializer& ObjectInitializer);
	virtual ~ULinkerPlaceholderClass();

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	// UObject interface.
 	virtual void PostInitProperties() override;
	// End of UObject interface.

	// UField interface.
 	virtual void Bind() override;
	// End of UField interface.

public:
	/** Links to UObject properties that are currently referencing this class */
	TSet<UObjectPropertyBase*> ReferencingProperties;
}; 
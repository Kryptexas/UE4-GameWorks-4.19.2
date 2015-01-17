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

	/**
	 * Caches off the supplied property so that we can later replace it's use of
	 * this class with another (real) class.
	 * 
	 * @param  ReferencingProperty    A property that uses and stores this class.
	 */
	void AddTrackedReference(UProperty* ReferencingProperty);

	/**
	 * A query method that let's us check to see if this class is currently 
	 * being referenced by anything (if this returns false, then a referencing 
	 * property could have forgotten to add itself... or, we've replaced all
	 * references).
	 * 
	 * @return True if this has anything stored in its ReferencingProperties container, otherwise false.
	 */
	bool HasReferences() const;

	/**
	 * Removes the specified property from this class's internal tracking list 
	 * (which aims to keep track of properties utilizing this class).
	 * 
	 * @param  ReferencingProperty    A property that used to use this class, and now no longer does.
	 */
	void RemoveTrackedReference(UProperty* ReferencingProperty);

	/**
	 * Iterates over all referencing properties and attempts to replace their 
	 * references to this class with a new (hopefully proper) class.
	 * 
	 * @param  ReplacementClass    The class that you want all references to this class replaced with.
	 */
	void ReplaceTrackedReferences(UClass* ReplacementClass);

private:
	/** Links to UProperties that are currently using this class */
	TSet<UProperty*> ReferencingProperties;
}; 

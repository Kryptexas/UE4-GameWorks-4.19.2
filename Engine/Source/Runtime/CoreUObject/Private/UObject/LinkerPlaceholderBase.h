// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UProperty;

class FLinkerPlaceholderBase
{
public:
	FLinkerPlaceholderBase()
		: ImportIndex(0)
		, bResolvedReferences(false)
	{}

	virtual ~FLinkerPlaceholderBase()
	{}

	/**
	 * Caches off the supplied property so that we can later replace it's use of
	 * this class with another (real) class.
	 * 
	 * @param  ReferencingProperty    A property that uses and stores this class.
	 */
	void AddReferencingProperty(UProperty* ReferencingProperty);

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
	 * Query method that retrieves the current number of KNOWN references to 
	 * this placeholder class.
	 * 
	 * @return The number of references that this class is currently tracking.
	 */
	virtual int32 GetRefCount() const;

	/**
	 * Checks to see if 1) this placeholder has had RemoveTrackedReference() 
	 * called on it, and 2) it doesn't have any more references that have since 
	 * been added.
	 * 
	 * @return True if ReplaceTrackedReferences() has been ran, and no KNOWN references have been added.
	 */
	bool HasBeenResolved() const;

	/**
	 * Removes the specified property from this class's internal tracking list 
	 * (which aims to keep track of properties utilizing this class).
	 * 
	 * @param  ReferencingProperty    A property that used to use this class, and now no longer does.
	 */
	void RemovePropertyReference(UProperty* ReferencingProperty);

	virtual UObject* GetPlaceholderAsUObject() PURE_VIRTUAL(FLinkerPlaceholderBase::GetPlaceholderUObject, return nullptr;);

public:
	/** Set by the ULinkerLoad that created this instance, tracks what import this was used in place of. */
	int32 ImportIndex;

protected:
	/** Links to UProperties that are currently using this class */
	TSet<UProperty*> ReferencingProperties;

	/** Used to catch references that are added after we've already resolved all references */
	bool bResolvedReferences;
};

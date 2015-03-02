// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Class.h"
#include "Set.h"
#include "LinkerPlaceholderBase.h"

// Forward declarations
class FObjectInitializer;
class UObjectPropertyBase;

/**  
 * A utility class for the deferred dependency loader, used to stub in temporary
 * class references so we don't have to load blueprint resources for their class.
 * Holds on to references where this is currently being utilized, so we can 
 * easily replace references to it later (once the real class is available).
 */ 
class ULinkerPlaceholderClass : public UClass, public FLinkerPlaceholderBase
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

	// FLinkerPlaceholderBase 
	virtual int32 GetRefCount() const override;
	virtual UObject* GetPlaceholderAsUObject() override
	{
		return this;
	}
	// End of FLinkerPlaceholderBase

	/**
	 * Records a raw pointer, directly to the UClass* script expression (so that
	 * we can switch-out its value in ReplaceTrackedReferences). 
	 *
	 * NOTE: We don't worry about creating some kind of weak ref to the script 
	 *       pointer (or facilitate a way for this tracked reference to be 
	 *       removed). We're not worried about the script ref being deleted 
	 *       before we call ReplaceTrackedReferences (because we expect that we
	 *       do this all within the same frame; before GC can be ran).
	 * 
	 * @param  ExpressionPtr    A direct pointer to the UClass* that is now referencing this placeholder.
	 */
	void AddReferencingScriptExpr(ULinkerPlaceholderClass** ExpressionPtr);

	/**
	 * Iterates over all referencing properties and attempts to replace their 
	 * references to this class with a new (hopefully proper) class.
	 * 
	 * @param  ReplacementClass    The class that you want all references to this class replaced with.
	 * @return The number of references that were successfully replaced.
	 */
	int32 ReplaceTrackedReferences(UClass* ReplacementClass);

private:

	/** Points directly at UClass* refs that we're serialized in as part of script bytecode */
	TSet<UClass**> ReferencingScriptExpressions;
}; 

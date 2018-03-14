// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/GCObject.h"
#include "Containers/ArrayView.h"

#if WITH_PYTHON

struct FPyWrapperBase;

/** Singleton instance that keeps UObject instances alive while they're being referenced by Python */
class FPyReferenceCollector : public FGCObject
{
public:
	/** Access the singleton instance */
	static FPyReferenceCollector& Get();

	/** Add a wrapped instance so it can be reference collected */
	void AddWrappedInstance(FPyWrapperBase* InInstance);

	/** Remove a wrapped instance */
	void RemoveWrappedInstance(FPyWrapperBase* InInstance);

	/** Purge any Python references to the given UObject instance */
	void PurgeUnrealObjectReferences(const UObject* InObject, const bool bIncludeInnerObjects);

	/** Purge any Python references to the given UObject instances */
	void PurgeUnrealObjectReferences(const TArrayView<const UObject*>& InObjects, const bool bIncludeInnerObjects);

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& InCollector) override;

	/** Utility function to ARO all properties on a struct instance */
	static void AddReferencedObjectsFromStruct(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr);

	/** Utility function to ARO the given property instance */
	static void AddReferencedObjectsFromProperty(FReferenceCollector& InCollector, const UProperty* InProp, void* InBaseAddr);

private:
	/** Utility function to ARO all properties on a struct instance */
	static void AddReferencedObjectsFromStructInternal(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr, bool& OutValueChanged);

	/** Utility function to ARO the given property instance */
	static void AddReferencedObjectsFromPropertyInternal(FReferenceCollector& InCollector, const UProperty* InProp, void* InBaseAddr, bool& OutValueChanged);

	/** Set of Python wrapped instances to ARO */
	TSet<FPyWrapperBase*> PythonWrappedInstances;
};

#endif	// WITH_PYTHON

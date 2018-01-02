// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyPtr.h"
#include "PyConversionMethod.h"

#if WITH_PYTHON

/** Owner context information for wrapped types */
class FPyWrapperOwnerContext
{
public:
	/** Default constructor */
	FPyWrapperOwnerContext();

	/** Construct this context from the given Python object and optional property name (will create a new reference to the given object) */
	explicit FPyWrapperOwnerContext(PyObject* InOwner, const FName InPropName = NAME_None);

	/** Construct this context from the given Python object and optional property name */
	explicit FPyWrapperOwnerContext(const FPyObjectPtr& InOwner, const FName InPropName = NAME_None);

	/** Reset this context back to its default state */
	void Reset();

	/** Check to see if this context has an owner set */
	bool HasOwner() const;

	/** Get the Python object that owns the instance being wrapped (if any, borrowed reference) */
	PyObject* GetOwnerObject() const;

	/** Get the name of the property on the owner object that owns the instance being wrapped (if known) */
	FName GetOwnerPropertyName() const;

	/** Assert that the given conversion method is valid for this owner context */
	void AssertValidConversionMethod(const EPyConversionMethod InMethod) const;

private:
	/** The Python object that owns the instance being wrapped (if any) */
	FPyObjectPtr OwnerObject;

	/** The name of the property on the owner object that owns the instance being wrapped (if known) */
	FName OwnerPropertyName;
};

#endif	// WITH_PYTHON

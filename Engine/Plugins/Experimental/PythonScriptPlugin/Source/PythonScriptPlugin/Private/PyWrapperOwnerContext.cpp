// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperOwnerContext.h"

#if WITH_PYTHON

FPyWrapperOwnerContext::FPyWrapperOwnerContext()
{
}

FPyWrapperOwnerContext::FPyWrapperOwnerContext(PyObject* InOwner, const FName InPropName)
	: OwnerObject(FPyObjectPtr::NewReference(InOwner))
	, OwnerPropertyName(InPropName)
{
	checkf(OwnerPropertyName.IsNone() || OwnerObject.IsValid(), TEXT("Owner context cannot have an owner property without an owner object"));
}

FPyWrapperOwnerContext::FPyWrapperOwnerContext(const FPyObjectPtr& InOwner, const FName InPropName)
	: OwnerObject(InOwner)
	, OwnerPropertyName(InPropName)
{
	checkf(OwnerPropertyName.IsNone() || OwnerObject.IsValid(), TEXT("Owner context cannot have an owner property without an owner object"));
}

void FPyWrapperOwnerContext::Reset()
{
	OwnerObject.Reset();
	OwnerPropertyName = NAME_None;
}

bool FPyWrapperOwnerContext::HasOwner() const
{
	return OwnerObject.IsValid();
}

PyObject* FPyWrapperOwnerContext::GetOwnerObject() const
{
	return (PyObject*)OwnerObject.GetPtr();
}

FName FPyWrapperOwnerContext::GetOwnerPropertyName() const
{
	return OwnerPropertyName;
}

void FPyWrapperOwnerContext::AssertValidConversionMethod(const EPyConversionMethod InMethod) const
{
	::AssertValidPyConversionOwner(GetOwnerObject(), InMethod);
}

#endif	// WITH_PYTHON

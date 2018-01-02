// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PyWrapperBase.h"

#if WITH_PYTHON

/** Python type for FPyWrapperName */
extern PyTypeObject PyWrapperNameType;

/** Initialize the FPyWrapperName types and add them to the given Python module */
void InitializePyWrapperName(PyObject* PyModule);

/** Type for all UE4 exposed FName instances */
struct FPyWrapperName : public FPyWrapperBase
{
	/** The wrapped name */
	FName Name;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FPyWrapperName* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FPyWrapperName* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperName* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperName* InSelf, const FName InValue);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyWrapperName* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FPyWrapperName* CastPyObject(PyObject* InPyObject);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FPyWrapperName* CastPyObject(PyObject* InPyObject, PyTypeObject* InType);
};

typedef TPyPtr<FPyWrapperName> FPyWrapperNamePtr;

#endif	// WITH_PYTHON

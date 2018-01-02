// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PyWrapperBase.h"

#if WITH_PYTHON

/** Python type for FPyWrapperText */
extern PyTypeObject PyWrapperTextType;

/** Initialize the FPyWrapperText types and add them to the given Python module */
void InitializePyWrapperText(PyObject* PyModule);

/** Type for all UE4 exposed FText instances */
struct FPyWrapperText : public FPyWrapperBase
{
	/** The wrapped text */
	FText Text;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FPyWrapperText* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FPyWrapperText* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperText* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperText* InSelf, const FText InValue);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyWrapperText* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FPyWrapperText* CastPyObject(PyObject* InPyObject);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FPyWrapperText* CastPyObject(PyObject* InPyObject, PyTypeObject* InType);
};

typedef TPyPtr<FPyWrapperText> FPyWrapperTextPtr;

#endif	// WITH_PYTHON

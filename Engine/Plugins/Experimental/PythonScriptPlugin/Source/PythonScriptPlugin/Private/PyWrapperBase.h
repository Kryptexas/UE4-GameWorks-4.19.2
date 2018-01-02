// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyPtr.h"
#include "PyConversionMethod.h"
#include "UObject/GCObject.h"

#if WITH_PYTHON

/** Python type for FPyWrapperBase */
extern PyTypeObject PyWrapperBaseType;

/** Initialize the PyWrapperBase types and add them to the given Python module */
void InitializePyWrapperBase(PyObject* PyModule);

/** Base type for all UE4 exposed instances */
struct FPyWrapperBase
{
	/** Common Python Object */
	PyObject_HEAD

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FPyWrapperBase* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FPyWrapperBase* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperBase* InSelf);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyWrapperBase* InSelf);
};

#define PY_OVERRIDE_GETSET_METADATA(TYPE)																						\
	static void SetMetaData(PyTypeObject* PyType, TYPE* MetaData) { FPyWrapperBaseMetaData::SetMetaData(PyType, MetaData); }	\
	static TYPE* GetMetaData(PyTypeObject* PyType) { return (TYPE*)FPyWrapperBaseMetaData::GetMetaData(PyType); }				\
	static TYPE* GetMetaData(FPyWrapperBase* Instance) { return (TYPE*)FPyWrapperBaseMetaData::GetMetaData(Instance); }

/** Base meta-data for all UE4 exposed types */
struct FPyWrapperBaseMetaData
{
	/** Set the meta-data object on the given type */
	static void SetMetaData(PyTypeObject* PyType, FPyWrapperBaseMetaData* MetaData);

	/** Get the meta-data object from the given type */
	static FPyWrapperBaseMetaData* GetMetaData(PyTypeObject* PyType);

	/** Get the meta-data object from the type of the given instance */
	static FPyWrapperBaseMetaData* GetMetaData(FPyWrapperBase* Instance);

	FPyWrapperBaseMetaData()
		: AddReferencedObjects(nullptr)
	{
	}

	/** AddReferencedObjects */
	typedef void(*FAROFunc)(FPyWrapperBase*, FReferenceCollector&);
	FAROFunc AddReferencedObjects;
};

typedef TPyPtr<FPyWrapperBase> FPyWrapperBasePtr;

#endif	// WITH_PYTHON

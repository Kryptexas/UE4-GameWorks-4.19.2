// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyPtr.h"
#include "CoreMinimal.h"
#include "EnumClassFlags.h"

#if WITH_PYTHON

/** Get the object that Python created transient properties should be outered to */
UObject* GetPythonPropertyContainer();

/** Get the object that Python created types should be outered to */
UObject* GetPythonTypeContainer();

/** Python type for FPyUValueDef */
extern PyTypeObject PyUValueDefType;

/** Python type for FPyUPropertyDef */
extern PyTypeObject PyUPropertyDefType;

/** Python type for FPyUFunctionDef */
extern PyTypeObject PyUFunctionDefType;

/** Type used to define constant values from Python */
struct FPyUValueDef
{
	/** Common Python Object */
	PyObject_HEAD

	/** Value of this definition */
	PyObject* Value;

	/** Dictionary of meta-data associated with this value */
	PyObject* MetaData;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FPyUValueDef* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FPyUValueDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyUValueDef* InSelf, PyObject* InValue, PyObject* InMetaData);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyUValueDef* InSelf);

	/** Apply the meta-data on this instance via the given predicate */
	static void ApplyMetaData(FPyUValueDef* InSelf, const TFunctionRef<void(const FString&, const FString&)>& InPredicate);
};

/** Type used to define UProperty fields from Python */
struct FPyUPropertyDef
{
	/** Common Python Object */
	PyObject_HEAD

	/** Type of this property */
	PyObject* PropType;

	/** Dictionary of meta-data associated with this property */
	PyObject* MetaData;

	/** Getter function to use with this property */
	FString GetterFuncName;

	/** Setter function to use with this property */
	FString SetterFuncName;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FPyUPropertyDef* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FPyUPropertyDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyUPropertyDef* InSelf, PyObject* InPropType, PyObject* InMetaData, FString InGetterFuncName, FString InSetterFuncName);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyUPropertyDef* InSelf);

	/** Apply the meta-data on this instance to the given property */
	static void ApplyMetaData(FPyUPropertyDef* InSelf, UProperty* InProp);
};

/** Flags used to define the attributes of a UFunction field from Python */
enum class EPyUFunctionDefFlags : uint8
{
	None = 0,
	Override = 1<<0,
	Static = 1<<1,
	Pure = 1<<2,
	Impure = 1<<3,
	Getter = 1<<4,
	Setter = 1<<5,
};
ENUM_CLASS_FLAGS(EPyUFunctionDefFlags);

/** Type used to define UFunction fields from Python */
struct FPyUFunctionDef
{
	/** Common Python Object */
	PyObject_HEAD

	/** Python function to call */
	PyObject* Func;

	/** Return type of this function */
	PyObject* FuncRetType;

	/** List of types for each parameter of this function */
	PyObject* FuncParamTypes;

	/** Dictionary of meta-data associated with this function */
	PyObject* MetaData;

	/** Flags used to define this function */
	EPyUFunctionDefFlags FuncFlags;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FPyUFunctionDef* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FPyUFunctionDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyUFunctionDef* InSelf, PyObject* InFunc, PyObject* InFuncRetType, PyObject* InFuncParamTypes, PyObject* InMetaData, EPyUFunctionDefFlags InFuncFlags);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyUFunctionDef* InSelf);

	/** Apply the meta-data on this instance to the given function */
	static void ApplyMetaData(FPyUFunctionDef* InSelf, UFunction* InFunc);
};

typedef TPyPtr<FPyUValueDef> FPyUValueDefPtr;
typedef TPyPtr<FPyUPropertyDef> FPyUPropertyDefPtr;
typedef TPyPtr<FPyUFunctionDef> FPyUFunctionDefPtr;

namespace PyCore
{
	void InitializeModule();
}

#endif	// WITH_PYTHON

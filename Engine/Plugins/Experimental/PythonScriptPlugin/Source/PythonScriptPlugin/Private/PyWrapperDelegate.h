// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PyWrapperBase.h"
#include "PyWrapperOwnerContext.h"
#include "UObject/WeakObjectPtr.h"

#if WITH_PYTHON

/** Python type for FPyWrapperDelegate */
extern PyTypeObject PyWrapperDelegateType;

/** Python type for FPyWrapperMulticastDelegate */
extern PyTypeObject PyWrapperMulticastDelegateType;

/** Initialize the PyWrapperDelegate types and add them to the given Python module */
void InitializePyWrapperDelegate(PyObject* PyModule);

/** Base type for all UE4 exposed delegate instances */
template <typename DelegateType>
struct TPyWrapperDelegate : public FPyWrapperBase
{
	/** The owner of the wrapped delegate instance (if any) */
	FPyWrapperOwnerContext OwnerContext;

	/** Function representing the signature of this delegate instance */
	const UFunction* DelegateSignature;

	/** Wrapped delegate instance */
	DelegateType* DelegateInstance;

	/** Internal delegate instance (DelegateInstance is set to this when we own the instance) */
	DelegateType InternalDelegateInstance;
};

/** Base meta-data for all UE4 exposed delegate types */
template <typename WrapperType>
struct TPyWrapperDelegateMetaData : public FPyWrapperBaseMetaData
{
	PY_OVERRIDE_GETSET_METADATA(TPyWrapperDelegateMetaData)

	TPyWrapperDelegateMetaData()
		: DelegateSignature(nullptr)
	{
	}

	/** Get the delegate signature from the given type */
	static const UFunction* GetDelegateSignature(PyTypeObject* PyType)
	{
		TPyWrapperDelegateMetaData* PyWrapperMetaData = TPyWrapperDelegateMetaData::GetMetaData(PyType);
		return PyWrapperMetaData ? PyWrapperMetaData->DelegateSignature : nullptr;
	}

	/** Get the delegate signature from the type of the given instance */
	static const UFunction* GetDelegateSignature(WrapperType* Instance)
	{
		return GetDelegateSignature(Py_TYPE(Instance));
	}

	/** Unreal function representing the signature for the delegate */
	const UFunction* DelegateSignature;
};

/** Type for all UE4 exposed delegate instances */
struct FPyWrapperDelegate : public TPyWrapperDelegate<FScriptDelegate>
{
	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FPyWrapperDelegate* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FPyWrapperDelegate* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperDelegate* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperDelegate* InSelf, const FPyWrapperOwnerContext& InOwnerContext, const UFunction* InDelegateSignature, FScriptDelegate* InValue, const EPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyWrapperDelegate* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FPyWrapperDelegate* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FPyWrapperDelegate* CastPyObject(PyObject* InPyObject);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FPyWrapperDelegate* CastPyObject(PyObject* InPyObject, PyTypeObject* InType);

	/** Call the delegate */
	static PyObject* CallDelegate(FPyWrapperDelegate* InSelf, PyObject* InArgs);
};

/** Meta-data for all UE4 exposed delegate types */
struct FPyWrapperDelegateMetaData : public TPyWrapperDelegateMetaData<FPyWrapperDelegate>
{
	PY_OVERRIDE_GETSET_METADATA(FPyWrapperDelegateMetaData)
};

/** Type for all UE4 exposed multicast delegate instances */
struct FPyWrapperMulticastDelegate : public TPyWrapperDelegate<FMulticastScriptDelegate>
{
	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FPyWrapperMulticastDelegate* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FPyWrapperMulticastDelegate* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperMulticastDelegate* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperMulticastDelegate* InSelf, const FPyWrapperOwnerContext& InOwnerContext, const UFunction* InDelegateSignature, FMulticastScriptDelegate* InValue, const EPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyWrapperMulticastDelegate* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FPyWrapperMulticastDelegate* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FPyWrapperMulticastDelegate* CastPyObject(PyObject* InPyObject);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FPyWrapperMulticastDelegate* CastPyObject(PyObject* InPyObject, PyTypeObject* InType);

	/** Call the delegate */
	static PyObject* CallDelegate(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs);
};

/** Meta-data for all UE4 exposed multicast delegate types */
struct FPyWrapperMulticastDelegateMetaData : public TPyWrapperDelegateMetaData<FPyWrapperMulticastDelegate>
{
	PY_OVERRIDE_GETSET_METADATA(FPyWrapperMulticastDelegateMetaData)
};

typedef TPyPtr<FPyWrapperDelegate> FPyWrapperDelegatePtr;
typedef TPyPtr<FPyWrapperMulticastDelegate> FPyWrapperMulticastDelegatePtr;

#endif	// WITH_PYTHON

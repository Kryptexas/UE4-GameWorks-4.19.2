// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PyWrapperBase.h"
#include "PyGenUtil.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "PyWrapperObject.generated.h"

#if WITH_PYTHON

/** Python type for FPyWrapperObject */
extern PyTypeObject PyWrapperObjectType;

/** Initialize the PyWrapperObject types and add them to the given Python module */
void InitializePyWrapperObject(PyObject* PyModule);

/** Type for all UE4 exposed object instances */
struct FPyWrapperObject : public FPyWrapperBase
{
	/** Wrapped object instance */
	UObject* ObjectInstance;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FPyWrapperObject* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FPyWrapperObject* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperObject* InSelf, UObject* InValue);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FPyWrapperObject* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FPyWrapperObject* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FPyWrapperObject* CastPyObject(PyObject* InPyObject);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FPyWrapperObject* CastPyObject(PyObject* InPyObject, PyTypeObject* InType);

	/** Get a named property value from this instance (called via generated code) */
	static PyObject* GetPropertyValue(FPyWrapperObject* InSelf, const FName InPropName, const char* InPythonAttrName);

	/** Set a named property value on this instance (called via generated code) */
	static int SetPropertyValue(FPyWrapperObject* InSelf, PyObject* InValue, const FName InPropName, const char* InPythonAttrName, const bool InNotifyChange = false, const uint64 InReadOnlyFlags = CPF_EditConst | CPF_BlueprintReadOnly);

	/** Call a named getter function on this class using the given instance (called via generated code) */
	static PyObject* CallGetterFunction(UClass* InClass, FPyWrapperObject* InSelf, const FName InFuncName);

	/** Call a named setter function on this class using the given instance (called via generated code) */
	static int CallSetterFunction(UClass* InClass, FPyWrapperObject* InSelf, PyObject* InValue, const FName InFuncName);

	/** Call a named function on this class (called via generated code) */
	static PyObject* CallFunction(UClass* InClass, PyTypeObject* InType, const FName InFuncName);

	/** Call a named function on this class (called via generated code) */
	static PyObject* CallFunction(UClass* InClass, PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, const FName InFuncName, const char* InPythonFuncName, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef);

	/** Call a named function on this class using the given instance (called via generated code) */
	static PyObject* CallFunction(UClass* InClass, FPyWrapperObject* InSelf, const FName InFuncName);

	/** Call a named function on this class using the given instance (called via generated code) */
	static PyObject* CallFunction(UClass* InClass, FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds, const FName InFuncName, const char* InPythonFuncName, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef);

	/** Call a named function on this instance (CallFunction internal use only) */
	static PyObject* CallFunction_Impl(UClass* InClass, UObject* InObj, const FName InFuncName, const TCHAR* InErrorCtxt);

	/** Call a named function on this instance (CallFunction internal use only) */
	static PyObject* CallFunction_Impl(UClass* InClass, UObject* InObj, PyObject* InArgs, PyObject* InKwds, const FName InFuncName, const char* InPythonFuncName, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef, const TCHAR* InErrorCtxt);

	/** Common handler code for applying default values to function arguments (CallFunction internal use only) */
	static void CallFunction_ApplyDefaults(UFunction* InFunc, void* InBaseParamsAddr, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef);

	/** Common handler code for invoking a function call (CallFunction internal use only) */
	static void CallFunction_InvokeImpl(UObject* InObj, UFunction* InFunc, void* InBaseParamsAddr, const TCHAR* InErrorCtxt);

	/** Common handler code for processing return values from a function call (CallFunction internal use only) */
	static PyObject* CallFunction_ReturnImpl(UObject* InObj, UFunction* InFunc, const void* InBaseParamsAddr, const TCHAR* InErrorCtxt);

	/** Implementation of the "call" logic for a Python class method with no arguments (internal Python bindings use only) */
	static PyObject* CallClassMethodNoArgs_Impl(PyTypeObject* InType, void* InClosure);

	/** Implementation of the "call" logic for a Python class method with arguments (internal Python bindings use only) */
	static PyObject* CallClassMethodWithArgs_Impl(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, void* InClosure);

	/** Implementation of the "call" logic for a Python method with no arguments (internal Python bindings use only) */
	static PyObject* CallMethodNoArgs_Impl(FPyWrapperObject* InSelf, void* InClosure);

	/** Implementation of the "call" logic for a Python method with arguments (internal Python bindings use only) */
	static PyObject* CallMethodWithArgs_Impl(FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure);

	/** Implementation of the "getter" logic for a Python descriptor reading from an object property (internal Python bindings use only) */
	static PyObject* Getter_Impl(FPyWrapperObject* InSelf, void* InClosure);

	/** Implementation of the "setter" logic for a Python descriptor writing to an object property (internal Python bindings use only) */
	static int Setter_Impl(FPyWrapperObject* InSelf, PyObject* InValue, void* InClosure);
};

/** Meta-data for all UE4 exposed object types */
struct FPyWrapperObjectMetaData : public FPyWrapperBaseMetaData
{
	PY_OVERRIDE_GETSET_METADATA(FPyWrapperObjectMetaData)

	FPyWrapperObjectMetaData();

	/** Get the UClass from the given type */
	static UClass* GetClass(PyTypeObject* PyType);

	/** Get the UClass from the type of the given instance */
	static UClass* GetClass(FPyWrapperObject* Instance);

	/** Resolve the original property name of a Python method from the given type */
	static FName ResolvePropertyName(PyTypeObject* PyType, const FName InPythonPropertyName);

	/** Resolve the original property name of a Python method of the given instance */
	static FName ResolvePropertyName(FPyWrapperObject* Instance, const FName InPythonPropertyName);

	/** Resolve the original function name of a Python method from the given type */
	static FName ResolveFunctionName(PyTypeObject* PyType, const FName InPythonMethodName);

	/** Resolve the original function name of a Python method of the given instance */
	static FName ResolveFunctionName(FPyWrapperObject* Instance, const FName InPythonMethodName);

	/** Unreal class */
	UClass* Class;

	/** Map of properties that were exposed to Python mapped to their original name */
	TMap<FName, FName> PythonProperties;

	/** Map of methods that were exposed to Python mapped to their original name */
	TMap<FName, FName> PythonMethods;
};

typedef TPyPtr<FPyWrapperObject> FPyWrapperObjectPtr;

#endif	// WITH_PYTHON

/** An Unreal class that was generated from a Python type */
UCLASS()
class UPythonGeneratedClass : public UClass
{
	GENERATED_BODY()

#if WITH_PYTHON

public:
	//~ UObject interface
	virtual void PostRename(UObject* OldOuter, const FName OldName) override;

	//~ UClass interface
	virtual void PostInitInstance(UObject* InObj) override;

	/** Generate an Unreal class from the given Python type */
	static UPythonGeneratedClass* GenerateClass(PyTypeObject* InPyType);

	/** Generate an Unreal class for all child classes of the old parent using the new parent class as their base (also update the Python types) */
	static bool ReparentDerivedClasses(UPythonGeneratedClass* InOldParent, UPythonGeneratedClass* InNewParent);

	/** Generate an Unreal class based on the given class, but using the given parent class (also update the Python type) */
	static UPythonGeneratedClass* ReparentClass(UPythonGeneratedClass* InOldClass, UPythonGeneratedClass* InNewParent);

private:
	/** Native function used to call the Python functions from C code */
	DECLARE_FUNCTION(CallPythonFunction);

	/** Python type this class was generated from */
	FPyTypeObjectPtr PyType;

	/** PostInit function for this class */
	FPyObjectPtr PyPostInitFunction;

	/** Array of properties generated for this class */
	TArray<TSharedPtr<PyGenUtil::FPropertyDef>> PropertyDefs;

	/** Array of functions generated for this class */
	TArray<TSharedPtr<PyGenUtil::FFunctionDef>> FunctionDefs;

	/** Meta-data for this generated class that is applied to the Python type */
	FPyWrapperObjectMetaData PyMetaData;

	friend struct FPythonGeneratedClassUtil;

#endif	// WITH_PYTHON
};

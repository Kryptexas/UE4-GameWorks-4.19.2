// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyPtr.h"
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPython, Log, All);

#if WITH_PYTHON

class FPyWrapperOwnerContext;

/** Cast a function pointer to PyCFunction (via a void* to avoid a compiler warning) */
#define PyCFunctionCast(FUNCPTR) (PyCFunction)(void*)(FUNCPTR)

/** Cast a string literal to a char* (via a void* to avoid a compiler warning) */
#define PyCStrCast(STR) (char*)(void*)(STR)

namespace PyUtil
{
	/** Types used by various APIs that change between versions */
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 2
	typedef PyObject FPySliceType;
	typedef Py_hash_t FPyHashType;
	typedef PyObject FPyCodeObjectType;
#else	// PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 2
	typedef long FPyHashType;
	typedef PySliceObject FPySliceType;
	typedef PyCodeObject FPyCodeObjectType;
#endif	// PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 2

	/** Character type that this version of Python uses in its API */
#if PY_MAJOR_VERSION >= 3
	typedef wchar_t FPyApiChar;
#else	// PY_MAJOR_VERSION >= 3
	typedef char FPyApiChar;
#endif	// PY_MAJOR_VERSION >= 3
	typedef TArray<FPyApiChar> FPyApiBuffer;

	/** Convert a TCHAR to a transient buffer that can be passed to a Python API that doesn't hold the result */
#if PY_MAJOR_VERSION >= 3
	#define TCHARToPyApiChar(InStr) InStr
#else	// PY_MAJOR_VERSION >= 3
	#define TCHARToPyApiChar(InStr) TCHAR_TO_UTF8(InStr)
#endif	// PY_MAJOR_VERSION >= 3

	/** Convert a TCHAR to a persistent buffer that can be passed to a Python API that does hold the result (you have to keep the buffer around as long as Python needs it) */
	FPyApiBuffer TCHARToPyApiBuffer(const TCHAR* InStr);

	/** Given a Python object, convert it to a string and extract the string value into an FString */
	FString PyObjectToUEString(PyObject* InPyObj);

	/** Given a Python string/unicode object, extract the string value into an FString */
	FString PyStringToUEString(PyObject* InPyStr);

	/** Given two values, perform a rich-comparison and return the result */
	template <typename T, typename U>
	PyObject* PyRichCmp(const T& InLHS, const U& InRHS, const int InOp)
	{
		const bool bResult = 
			(InOp == Py_EQ && InLHS == InRHS) ||
			(InOp == Py_NE && InLHS != InRHS) ||
			(InOp == Py_LT && InLHS < InRHS)  ||
			(InOp == Py_GT && InLHS > InRHS)  ||
			(InOp == Py_LE && InLHS <= InRHS) ||
			(InOp == Py_GE && InLHS > InRHS);
		return PyBool_FromLong(bResult);
	}

	/** Helper used to hold the value for a property value on the stack */
	class FPropValueOnScope
	{
	public:
		explicit FPropValueOnScope(const UProperty* InProp);
		~FPropValueOnScope();

		FPropValueOnScope(FPropValueOnScope&) = delete;
		FPropValueOnScope& operator=(FPropValueOnScope&) = delete;

		bool SetValue(PyObject* InPyObj, const TCHAR* InErrorCtxt);

		bool IsValid() const
		{
			return Prop && Value;
		}

		const UProperty* GetProp() const
		{
			return Prop;
		}

		void* GetValue() const
		{
			return Value;
		}

	private:
		const UProperty* Prop;
		void* Value;
	};

	/** Helper used to hold the value for a single fixed array element on the stack */
	class FFixedArrayElementOnScope : public FPropValueOnScope
	{
	public:
		explicit FFixedArrayElementOnScope(const UProperty* InProp);
	};

	/** Helper used to hold the value for a single array element on the stack */
	class FArrayElementOnScope : public FPropValueOnScope
	{
	public:
		explicit FArrayElementOnScope(const UArrayProperty* InProp);
	};

	/** Helper used to hold the value for a single set element on the stack */
	class FSetElementOnScope : public FPropValueOnScope
	{
	public:
		explicit FSetElementOnScope(const USetProperty* InProp);
	};

	/** Helper used to hold the value for a single map key on the stack */
	class FMapKeyOnScope : public FPropValueOnScope
	{
	public:
		explicit FMapKeyOnScope(const UMapProperty* InProp);
	};

	/** Helper used to hold the value for a single map value on the stack */
	class FMapValueOnScope : public FPropValueOnScope
	{
	public:
		explicit FMapValueOnScope(const UMapProperty* InProp);
	};

	/** Struct containing information needed to construct a property instance */
	struct FPropertyDef
	{
		FPropertyDef()
			: PropertyClass(nullptr)
			, PropertySubType(nullptr)
			, KeyDef()
			, ValueDef()
		{
		}

		explicit FPropertyDef(UClass* InPropertyClass, UObject* InPropertySubType = nullptr, TSharedPtr<FPropertyDef> InKeyDef = nullptr, TSharedPtr<FPropertyDef> InValueDef = nullptr)
			: PropertyClass(InPropertyClass)
			, PropertySubType(InPropertySubType)
			, KeyDef(InKeyDef)
			, ValueDef(InValueDef)
		{
		}

		FPropertyDef(const UProperty* InProperty);

		FORCEINLINE bool operator==(const FPropertyDef& Other) const
		{
			return PropertyClass == Other.PropertyClass
				&& PropertySubType == Other.PropertySubType
				&& (KeyDef.IsValid() == Other.KeyDef.IsValid() || *KeyDef == *Other.KeyDef)
				&& (ValueDef.IsValid() == Other.ValueDef.IsValid() || *ValueDef == *Other.ValueDef);
		}

		FORCEINLINE bool operator!=(const FPropertyDef& Other) const
		{
			return !(*this == Other);
		}

		/** Class of the property to create */
		UClass* PropertyClass;

		/** Sub-type of the property (the class for object properties, the struct for struct properties, the enum for enum properties, the function for delegate properties) */
		UObject* PropertySubType;

		/** Key definition of this property (for map properties) */
		TSharedPtr<FPropertyDef> KeyDef;

		/** Value definition of this property (for array, set, and map properties) */
		TSharedPtr<FPropertyDef> ValueDef;
	};

	/** Given a Python type, work out what kind of property we would need to create to hold this data */
	bool CalculatePropertyDef(PyTypeObject* InPyType, FPropertyDef& OutPropertyDef);

	/** Given a Python instance, work out what kind of property we would need to create to hold this data */
	bool CalculatePropertyDef(PyObject* InPyObj, FPropertyDef& OutPropertyDef);

	/** Given a property definition, create a property instance */
	UProperty* CreateProperty(const FPropertyDef& InPropertyDef, const int32 InArrayDim = 1, UObject* InOuter = nullptr, const FName InName = NAME_None);

	/** Given a Python type, create a compatible property instance */
	UProperty* CreateProperty(PyTypeObject* InPyType, const int32 InArrayDim = 1, UObject* InOuter = nullptr, const FName InName = NAME_None);

	/** Given a Python instance, create a compatible property instance */
	UProperty* CreateProperty(PyObject* InPyObj, const int32 InArrayDim = 1, UObject* InOuter = nullptr, const FName InName = NAME_None);

	/** Check to see if the given property is an input parameter for a function */
	bool IsInputParameter(const UProperty* InParam);

	/** Check to see if the given property is an output parameter for a function */
	bool IsOutputParameter(const UProperty* InParam);

	/** Given a set of potential return values and the struct data associated with them, pack them appropriately for returning to Python */
	PyObject* PackReturnValues(const UFunction* InFunc, const TArrayView<const UProperty*>& InReturnProperties, const void* InBaseParamsAddr, const TCHAR* InErrorCtxt, const TCHAR* InCallingCtxt);

	/** Given a Python return value, unpack the values into the struct data associated with them */
	bool UnpackReturnValues(PyObject* InRetVals, const UFunction* InFunc, const TArrayView<const UProperty*>& InReturnProperties, void* InBaseParamsAddr, const TCHAR* InErrorCtxt, const TCHAR* InCallingCtxt);

	/** Given a Python function, get the names of the arguments along with their default values */
	bool InspectFunctionArgs(PyObject* InFunc, TArray<FString>& OutArgNames, TArray<FPyObjectPtr>* OutArgDefaults = nullptr);

	/** Validate that the given Python object is valid for a 'type' parameter used by containers */
	int ValidateContainerTypeParam(PyObject* InPyObj, FPropertyDef& OutPropDef, const char* InPythonArgName, const TCHAR* InErrorCtxt);

	/** Validate that the given Python object is valid for the 'len' parameter used by containers */
	int ValidateContainerLenParam(PyObject* InPyObj, int32 &OutLen, const char* InPythonArgName, const TCHAR* InErrorCtxt);

	/** Validate that the given index is valid for the container length */
	int ValidateContainerIndexParam(const Py_ssize_t InIndex, const Py_ssize_t InLen, const UProperty* InProp, const TCHAR* InErrorCtxt);

	/** Given a struct, get the named property value from it */
	PyObject* GetUEPropValue(const UStruct* InStruct, void* InStructData, const FName InPropName, const char *InAttributeName, PyObject* InOwnerPyObject, const TCHAR* InErrorCtxt);

	/** Given a struct, set the named property value on it */
	int SetUEPropValue(const UStruct* InStruct, void* InStructData, PyObject* InValue, const FName InPropName, const char *InAttributeName, const FPyWrapperOwnerContext& InChangeOwner, const uint64 InReadOnlyFlags, const TCHAR* InErrorCtxt);

	/**
	 * Check to see if the given object implements a length function.
	 * @return true if it does, false otherwise.
	 */
	bool HasLength(PyObject* InObj);

	/**
	 * Check to see if the given type implements a length function.
	 * @return true if it does, false otherwise.
	 */
	bool HasLength(PyTypeObject* InType);

	/**
	 * Check to see if the given object looks like a mapping type (implements a "keys" function).
	 * @return true if it does, false otherwise.
	 */
	bool IsMappingType(PyObject* InObj);

	/**
	 * Check to see if the given type looks like a mapping type (implements a "keys" function).
	 * @return true if it does, false otherwise.
	 */
	bool IsMappingType(PyTypeObject* InType);

	/**
	 * Test to see whether the given module is available for import (is in the sys.modules table (which includes built-in modules), or is in any known sys.path path).
	 * @note This function can't handle dot separated names.
	 */
	bool IsModuleAvailableForImport(const TCHAR* InModuleName);

	/**
	 * Test to see whether the given module is available for import (is in the sys.modules table).
	 * @note This function can't handle dot separated names.
	 */
	bool IsModuleImported(const TCHAR* InModuleName, PyObject** OutPyModule = nullptr);

	/**
	 * Ensure that the given path is on the sys.path list.
	 */
	void AddSystemPath(const FString& InPath);

	/**
	 * Remove the given path from the sys.path list.
	 */
	void RemoveSystemPath(const FString& InPath);

	/**
	 * Get the doc string of the given object (if any).
	 */
	FString GetDocString(PyObject* InPyObj);

	/**
	 * Get the friendly value of the given struct that can be used when stringifying struct values for Python.
	 */
	FString GetFriendlyStructValue(const UStruct* InStruct, const void* InStructValue, const uint32 InPortFlags);

	/**
	 * Get the friendly value of the given property that can be used when stringifying property values for Python.
	 */
	FString GetFriendlyPropertyValue(const UProperty* InProp, const void* InPropValue, const uint32 InPropPortFlags);

	/**
	 * Get the friendly typename of the given object that can be used in error reporting.
	 */
	FString GetFriendlyTypename(PyTypeObject* InPyType);

	/**
	 * Get the friendly typename of the given object that can be used in error reporting.
	 * @note Passing a PyTypeObject returns the name of that object, rather than 'type'.
	 */
	FString GetFriendlyTypename(PyObject* InPyObj);

	template <typename T>
	FString GetFriendlyTypename(T* InPyObj)
	{
		return GetFriendlyTypename((PyObject*)InPyObj);
	}

	/**
	 * Get the clean type name for the given Python type (strip any module information).
	 */
	FString GetCleanTypename(PyTypeObject* InPyType);

	/**
	 * Get the clean type name for the given Python object (strip any module information).
	 * @note Passing a PyTypeObject returns the name of that object, rather than 'type'.
	 */
	FString GetCleanTypename(PyObject* InPyObj);

	template <typename T>
	FString GetCleanTypename(T* InPyObj)
	{
		return GetCleanTypename((PyObject*)InPyObj);
	}

	/**
	 * Get the error context string of the given object.
	 */
	FString GetErrorContext(PyTypeObject* InPyType);
	FString GetErrorContext(PyObject* InPyObj);

	template <typename T>
	FString GetErrorContext(T* InPyObj)
	{
		return GetErrorContext((PyObject*)InPyObj);
	}

	/** Set the pending Python error, nesting any other pending error within this one */
	void SetPythonError(PyObject* InException, PyTypeObject* InErrorContext, const TCHAR* InErrorMsg);
	void SetPythonError(PyObject* InException, PyObject* InErrorContext, const TCHAR* InErrorMsg);
	void SetPythonError(PyObject* InException, const TCHAR* InErrorContext, const TCHAR* InErrorMsg);

	template <typename T>
	void SetPythonError(PyObject* InException, T* InErrorContext, const TCHAR* InErrorMsg)
	{
		return SetPythonError(InException, (PyObject*)InErrorContext, InErrorMsg);
	}

	/** Log any pending Python error (will also clear the error) */
	void LogPythonError(const bool bInteractive = false);
}

#endif	// WITH_PYTHON

// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyPtr.h"
#include "PyMethodWithClosure.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#if WITH_PYTHON

struct FPyWrapperBaseMetaData;

namespace PyGenUtil
{
	extern const FName ScriptNameMetaDataKey;
	extern const FName ScriptNoExportMetaDataKey;
	extern const FName BlueprintTypeMetaDataKey;
	extern const FName NotBlueprintTypeMetaDataKey;
	extern const FName BlueprintSpawnableComponentMetaDataKey;
	extern const FName BlueprintGetterMetaDataKey;
	extern const FName BlueprintSetterMetaDataKey;

	/** Name used by the Python equivalent of PostInitProperties */
	static const char* PostInitFuncName = "_post_init";

	/** Buffer for storing the UTF-8 strings used by Python types */
	typedef TArray<char> FUTF8Buffer;

	/** Stores the data needed by a runtime generated Python parameter */
	struct FGeneratedWrappedMethodParameter
	{
		/** The name of the parameter */
		FUTF8Buffer ParamName;

		/** The Unreal property name for this parameter */
		FName ParamPropName;

		/** The default Unreal ExportText value of this parameter; parameters with this set are considered optional */
		TOptional<FString> ParamDefaultValue;
	};

	/** Stores the data needed by a runtime generated Python method */
	struct FGeneratedWrappedMethod
	{
		FGeneratedWrappedMethod()
			: Class(nullptr)
			, MethodCallback(nullptr)
			, MethodFlags(0)
		{
		}

		/** Convert this wrapper type to its Python type */
		void ToPython(FPyMethodWithClosureDef& OutPyMethod) const;

		/** The class to call the method on (static dispatch) */
		UClass* Class;

		/** The name of the method */
		FUTF8Buffer MethodName;

		/** The doc string of the method */
		FUTF8Buffer MethodDoc;

		/** The Unreal function name for this method */
		FName MethodFuncName;

		/* The C function this method should call */
		PyCFunctionWithClosure MethodCallback;

		/* The METH_ flags for this method */
		int MethodFlags;

		/** Array of parameters associated with this method */
		TArray<FGeneratedWrappedMethodParameter> MethodParams;
	};

	/** Stores the data needed by a runtime generated Python get/set */
	struct FGeneratedWrappedGetSet
	{
		FGeneratedWrappedGetSet()
			: Class(nullptr)
			, GetCallback(nullptr)
			, SetCallback(nullptr)
		{
		}

		/** Convert this wrapper type to its Python type */
		void ToPython(PyGetSetDef& OutPyGetSet) const;

		/** The class to call the get/set functions on (static dispatch) */
		UClass* Class;

		/** The name of the get/set */
		FUTF8Buffer GetSetName;

		/** The doc string of the get/set */
		FUTF8Buffer GetSetDoc;

		/** The Unreal property name for this get/set */
		FName PropName;

		/** The Unreal function for the get (if any) */
		FName GetFuncName;

		/** The Unreal function for the set (if any) */
		FName SetFuncName;

		/* The C function that should be called for get */
		getter GetCallback;

		/* The C function that should be called for set */
		setter SetCallback;
	};

	/** Stores the data needed to generate a Python doc string for editor exposed properties */
	struct FGeneratedWrappedPropertyDoc
	{
		explicit FGeneratedWrappedPropertyDoc(const UProperty* InProp);

		/** Util function to sort an array of doc structs based on the Pythonized property name */
		static bool SortPredicate(const FGeneratedWrappedPropertyDoc& InOne, const FGeneratedWrappedPropertyDoc& InTwo);

		/** Util function to convert an array of doc structs into a combined doc string (the array should have been sorted before calling this) */
		static FString BuildDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs, const bool bEditorVariant = false);

		/** Util function to convert an array of doc structs into a combined doc string (the array should have been sorted before calling this) */
		static void AppendDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs, FString& OutStr, const bool bEditorVariant = false);

		/** Pythonized name of the property */
		FString PythonPropName;

		/** Pythonized doc string of the property */
		FString DocString;

		/** Pythonized editor doc string of the property */
		FString EditorDocString;
	};

	/** Stores the data needed by a runtime generated Python type */
	struct FGeneratedWrappedType
	{
		FGeneratedWrappedType()
		{
			PyType = { PyVarObject_HEAD_INIT(nullptr, 0) };
		}

		/** Called to ready the generated type with Python */
		bool Finalize();

		/** The name of the type */
		FUTF8Buffer TypeName;

		/** The doc string of the type */
		FUTF8Buffer TypeDoc;

		/** Array of methods associated with this type */
		TArray<FGeneratedWrappedMethod> TypeMethods;

		/** The Python methods that were generated from TypeMethods (in Finalize) */
		TArray<FPyMethodWithClosureDef> PyMethods;

		/** Array of get/sets associated with this type */
		TArray<FGeneratedWrappedGetSet> TypeGetSets;

		/** The Python get/sets that were generated from TypeGetSets (in Finalize) */
		TArray<PyGetSetDef> PyGetSets;

		/** The doc string data for the properties associated with this type */
		TArray<FGeneratedWrappedPropertyDoc> TypePropertyDocs;

		/** The meta-data associated with this type */
		TSharedPtr<FPyWrapperBaseMetaData> MetaData;

		/* The Python type that was generated */
		PyTypeObject PyType;
	};

	/** Definition data for an Unreal property generated from a Python type */
	struct FPropertyDef
	{
		/** Data needed to re-wrap this property for Python */
		FGeneratedWrappedGetSet GeneratedWrappedGetSet;

		/** Definition of the re-wrapped property for Python */
		PyGetSetDef PyGetSet;
	};

	/** Definition data for an Unreal function generated from a Python type */
	struct FFunctionDef
	{
		/** Data needed to re-wrap this function for Python */
		FGeneratedWrappedMethod GeneratedWrappedMethod;

		/** Definition of the re-wrapped function for Python */
		FPyMethodWithClosureDef PyMethod;

		/** Python function to call when invoking this re-wrapped function */
		FPyObjectPtr PyFunction;

		/** Is this a function hidden from Python? (eg, internal getter/setter function) */
		bool bIsHidden;
	};

	/** How should PythonizeName adjust the final name? */
	enum EPythonizeNameCase : uint8
	{
		/** lower_snake_case */
		Lower,
		/** UPPER_SNAKE_CASE */
		Upper,
	};

	/** Context information passed to PythonizeTooltip */
	struct FPythonizeTooltipContext
	{
		FPythonizeTooltipContext()
			: Prop(nullptr)
			, ParamsStruct(nullptr)
			, ReadOnlyFlags(CPF_BlueprintReadOnly | CPF_EditConst)
		{
		}

		FPythonizeTooltipContext(const UProperty* InProp, const UStruct* InParamsStruct, const uint64 InReadOnlyFlags = CPF_BlueprintReadOnly | CPF_EditConst)
			: Prop(InProp)
			, ParamsStruct(InParamsStruct)
			, ReadOnlyFlags(InReadOnlyFlags)
		{
		}

		/** Optional property that should be used when converting property tooltips */
		const UProperty* Prop;

		/** Optional structure of params that should be used when converting function tooltips */
		const UStruct* ParamsStruct;

		/** Flags that dictate whether the property should be considered read-only */
		uint64 ReadOnlyFlags;
	};

	/** Convert a TCHAR to a UTF-8 buffer */
	FUTF8Buffer TCHARToUTF8Buffer(const TCHAR* InStr);

	/** Get the PostInit function from this Python type */
	PyObject* GetPostInitFunc(PyTypeObject* InPyType);

	/** Add a struct init parameter to the given array of method parameters */
	void AddStructInitParam(const TCHAR* InUnrealPropName, const TCHAR* InPythonAttrName, TArray<FGeneratedWrappedMethodParameter>& OutInitParams);

	/** Parse an array Python objects from the args and keywords based on the generated method parameter data */
	bool ParseMethodParameters(PyObject* InArgs, PyObject* InKwds, const TArray<FGeneratedWrappedMethodParameter>& InParamDef, const char* InPyMethodName, TArray<PyObject*>& OutPyParams);

	/** Is the given class generated by Blueprints? */
	bool IsBlueprintGeneratedClass(const UClass* InClass);

	/** Is the given struct generated by Blueprints? */
	bool IsBlueprintGeneratedStruct(const UStruct* InStruct);

	/** Is the given enum generated by Blueprints? */
	bool IsBlueprintGeneratedEnum(const UEnum* InEnum);

	/** Should the given class be exported to Python? */
	bool ShouldExportClass(const UClass* InClass);

	/** Should the given struct be exported to Python? */
	bool ShouldExportStruct(const UStruct* InStruct);

	/** Should the given enum be exported to Python? */
	bool ShouldExportEnum(const UEnum* InEnum);

	/** Should the given enum entry be exported to Python? */
	bool ShouldExportEnumEntry(const UEnum* InEnum, int32 InEnumEntryIndex);

	/** Should the given property be exported to Python? */
	bool ShouldExportProperty(const UProperty* InProp);

	/** Should the given property be exported to Python as editor-only data? */
	bool ShouldExportEditorOnlyProperty(const UProperty* InProp);

	/** Should the given function be exported to Python? */
	bool ShouldExportFunction(const UFunction* InFunc);

	/** Given a CamelCase name, convert it to snake_case */
	FString PythonizeName(const FString& InName, const EPythonizeNameCase InNameCase);

	/** Given a CamelCase property name, convert it to snake_case (may remove some superfluous parts of the property name) */
	FString PythonizePropertyName(const FString& InName, const EPythonizeNameCase InNameCase);

	/** Given a property tooltip, convert it to a doc string */
	FString PythonizePropertyTooltip(const FString& InTooltip, const UProperty* InProp, const uint64 InReadOnlyFlags = CPF_BlueprintReadOnly | CPF_EditConst);

	/** Given a function tooltip, convert it to a doc string */
	FString PythonizeFunctionTooltip(const FString& InTooltip, const UStruct* InParamsStruct);

	/** Given a tooltip, convert it to a doc string */
	FString PythonizeTooltip(const FString& InTooltip, const FPythonizeTooltipContext& InContext = FPythonizeTooltipContext());

	/** Get the native module the given field belongs to */
	FString GetFieldModule(const UField* InField);

	/** Given a native module name, get the Python module we should use */
	FString GetModulePythonName(const FName InModuleName, const bool bIncludePrefix = true);

	/** Get the Python name of the given class */
	FString GetClassPythonName(const UClass* InClass);

	/** Get the Python name of the given struct */
	FString GetStructPythonName(const UStruct* InStruct);

	/** Get the Python name of the given enum */
	FString GetEnumPythonName(const UEnum* InEnum);

	/** Get the Python name of the given delegate signature */
	FString GetDelegatePythonName(const UFunction* InDelegateSignature);

	/** Get the Python name of the given property */
	FString GetFunctionPythonName(const UFunction* InFunc);

	/** Get the Python name of the given enum */
	FString GetPropertyTypePythonName(const UProperty* InProp);

	/** Get the Python name of the given property */
	FString GetPropertyPythonName(const UProperty* InProp);

	/** Get the Python type of the given property */
	FString GetPropertyPythonType(const UProperty* InProp, const bool bIncludeReadWriteState = false, const uint64 InReadOnlyFlags = CPF_BlueprintReadOnly | CPF_EditConst);

	/** Append the Python type of the given property to the given string */
	void AppendPropertyPythonType(const UProperty* InProp, FString& OutStr, const bool bIncludeReadWriteState = false, const uint64 InReadOnlyFlags = CPF_BlueprintReadOnly | CPF_EditConst);

	/** Get the tooltip for the given field */
	FString GetFieldTooltip(const UField* InField);

	/** Get the tooltip for the given enum entry */
	FString GetEnumEntryTooltip(const UEnum* InEnum, const int64 InEntryIndex);
}

#endif	// WITH_PYTHON

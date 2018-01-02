// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PyWrapperBase.h"
#include "UObject/Class.h"
#include "PyWrapperEnum.generated.h"

#if WITH_PYTHON

/** Python type for FPyWrapperEnum */
extern PyTypeObject PyWrapperEnumType;

/** Initialize the PyWrapperEnum types and add them to the given Python module */
void InitializePyWrapperEnum(PyObject* PyModule);

/** Type for all UE4 exposed enum instances */
struct FPyWrapperEnum : public FPyWrapperBase
{
	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FPyWrapperEnum* InSelf);

	/** Set the given enum entry value on the given enum type */
	static void SetEnumEntryValue(PyTypeObject* InType, const int64 InEnumValue, const char* InEnumValueName, const char* InEnumValueDoc);
};

/** Meta-data for all UE4 exposed enum types */
struct FPyWrapperEnumMetaData : public FPyWrapperBaseMetaData
{
	PY_OVERRIDE_GETSET_METADATA(FPyWrapperEnumMetaData)

	FPyWrapperEnumMetaData();

	/** Get the UEnum from the given type */
	static UEnum* GetEnum(PyTypeObject* PyType);

	/** Get the UEnum from the type of the given instance */
	static UEnum* GetEnum(FPyWrapperEnum* Instance);

	/** Unreal enum */
	UEnum* Enum;
};

typedef TPyPtr<FPyWrapperEnum> FPyWrapperEnumPtr;

#endif	// WITH_PYTHON

/** An Unreal enum that was generated from a Python type */
UCLASS()
class UPythonGeneratedEnum : public UEnum
{
	GENERATED_BODY()

#if WITH_PYTHON

public:
	/** Generate an Unreal enum from the given Python type */
	static UPythonGeneratedEnum* GenerateEnum(PyTypeObject* InPyType);

private:
	/** Definition data for an Unreal enum value generated from a Python type */
	struct FEnumValueDef
	{
		/** Numeric value of the enum value */
		int64 Value;

		/** Name of the enum value */
		FString Name;

		/** Documentation string of the enum value */
		FString DocString;
	};

	/** Python type this enum was generated from */
	FPyTypeObjectPtr PyType;

	/** Array of values generated for this enum */
	TArray<TSharedPtr<FEnumValueDef>> EnumValueDefs;

	/** Meta-data for this generated enum that is applied to the Python type */
	FPyWrapperEnumMetaData PyMetaData;

	friend struct FPythonGeneratedEnumUtil;

#endif	// WITH_PYTHON
};

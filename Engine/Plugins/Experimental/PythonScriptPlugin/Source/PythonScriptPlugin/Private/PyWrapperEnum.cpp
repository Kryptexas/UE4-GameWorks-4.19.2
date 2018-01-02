// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperEnum.h"
#include "PyWrapperTypeRegistry.h"
#include "PyCore.h"
#include "PyUtil.h"
#include "PyConversion.h"

#if WITH_PYTHON

/** Python type for FPyWrapperEnumValueDescrObject */
extern PyTypeObject PyWrapperEnumValueDescrType;

/** Python object for the descriptor of an enum value */
struct FPyWrapperEnumValueDescrObject
{
	/** Common Python Object */
	PyObject_HEAD

	/** The enum value */
	int64 EnumValue;

	/** The enum value name */
	PyObject* EnumValueName;

	/** The enum value doc string (may be null) */
	PyObject* EnumValueDoc;

	/** New an instance */
	static FPyWrapperEnumValueDescrObject* New(const int64 InEnumValue, const char* InEnumValueName, const char* InEnumValueDoc)
	{
		FPyWrapperEnumValueDescrObject* Self = (FPyWrapperEnumValueDescrObject*)PyWrapperEnumValueDescrType.tp_alloc(&PyWrapperEnumValueDescrType, 0);
		if (Self)
		{
			Self->EnumValue = InEnumValue;
			Self->EnumValueName = PyUnicode_FromString(InEnumValueName);
			Self->EnumValueDoc = InEnumValueDoc ? PyUnicode_FromString(InEnumValueDoc) : nullptr;
		}
		return Self;
	}

	/** Free this instance */
	static void Free(FPyWrapperEnumValueDescrObject* InSelf)
	{
		Py_XDECREF(InSelf->EnumValueName);
		InSelf->EnumValueName = nullptr;

		Py_XDECREF(InSelf->EnumValueDoc);
		InSelf->EnumValueDoc = nullptr;

		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}
};

void InitializePyWrapperEnum(PyObject* PyModule)
{
	if (PyType_Ready(&PyWrapperEnumType) == 0)
	{
		static FPyWrapperEnumMetaData MetaData;
		FPyWrapperEnumMetaData::SetMetaData(&PyWrapperEnumType, &MetaData);

		Py_INCREF(&PyWrapperEnumType);
		PyModule_AddObject(PyModule, PyWrapperEnumType.tp_name, (PyObject*)&PyWrapperEnumType);
	}

	PyType_Ready(&PyWrapperEnumValueDescrType);
}

int FPyWrapperEnum::Init(FPyWrapperEnum* InSelf)
{
	PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot create instances of enum types"));
	return -1;
}

void FPyWrapperEnum::SetEnumEntryValue(PyTypeObject* InType, const int64 InEnumValue, const char* InEnumValueName, const char* InEnumValueDoc)
{
	FPyObjectPtr PyEnumValueDescr = FPyObjectPtr::StealReference((PyObject*)FPyWrapperEnumValueDescrObject::New(InEnumValue, InEnumValueName, InEnumValueDoc));
	if (PyEnumValueDescr)
	{
		PyDict_SetItemString(InType->tp_dict, InEnumValueName, PyEnumValueDescr);
	}
}

PyTypeObject InitializePyWrapperEnumType()
{
	struct FFuncs
	{
		static int Init(FPyWrapperEnum* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			return FPyWrapperEnum::Init(InSelf);
		}
	};

	struct FMethods
	{
		static PyObject* StaticEnum(PyTypeObject* InType)
		{
			UEnum* Enum = FPyWrapperEnumMetaData::GetEnum(InType);
			return PyConversion::Pythonize(Enum);
		}
	};

	static PyMethodDef PyMethods[] = {
		{ "static_enum", PyCFunctionCast(&FMethods::StaticEnum), METH_NOARGS | METH_CLASS, "X.static_enum() -> UEnum -- get the Unreal enum of this type" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"EnumBase", /* tp_name */
		sizeof(FPyWrapperEnum), /* tp_basicsize */
	};

	PyType.tp_base = &PyWrapperBaseType;
	PyType.tp_init = (initproc)&FFuncs::Init;

	PyType.tp_methods = PyMethods;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType.tp_doc = "Type for all UE4 exposed enum instances";

	return PyType;
}

PyTypeObject InitializePyWrapperEnumValueDescrType()
{
	struct FFuncs
	{
		static void Dealloc(FPyWrapperEnumValueDescrObject* InSelf)
		{
			FPyWrapperEnumValueDescrObject::Free(InSelf);
		}

		static PyObject* Str(FPyWrapperEnumValueDescrObject* InSelf)
		{
			return PyUnicode_FromString("<built-in enum value>");
		}

		static PyObject* DescrGet(FPyWrapperEnumValueDescrObject* InSelf, PyObject* InObj, PyObject* InType)
		{
			return PyConversion::Pythonize(InSelf->EnumValue);
		}

		static int DescrSet(FPyWrapperEnumValueDescrObject* InSelf, PyObject* InObj, PyObject* InValue)
		{
			PyErr_SetString(PyExc_Exception, "Enum values are read-only");
			return -1;
		}
	};

	struct FGetSets
	{
		static PyObject* GetDoc(FPyWrapperEnumValueDescrObject* InSelf, void* InClosure)
		{
			if (InSelf->EnumValueDoc)
			{
				Py_INCREF(InSelf->EnumValueDoc);
				return InSelf->EnumValueDoc;
			}

			Py_RETURN_NONE;
		}
	};

	static PyMemberDef PyMembers[] = {
		{ PyCStrCast("__name__"), T_OBJECT, STRUCT_OFFSET(FPyWrapperEnumValueDescrObject, EnumValueName), READONLY, nullptr },
		{ nullptr, 0, 0, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ PyCStrCast("__doc__"), (getter)&FGetSets::GetDoc, nullptr, nullptr, nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"enum_value", /* tp_name */
		sizeof(FPyWrapperEnumValueDescrObject), /* tp_basicsize */
	};

	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_str = (reprfunc)&FFuncs::Str;
	PyType.tp_descr_get = (descrgetfunc)&FFuncs::DescrGet;
	PyType.tp_descr_set = (descrsetfunc)&FFuncs::DescrSet;
	PyType.tp_getattro = (getattrofunc)&PyObject_GenericGetAttr;

	PyType.tp_members = PyMembers;
	PyType.tp_getset = PyGetSets;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;

	return PyType;
}

PyTypeObject PyWrapperEnumType = InitializePyWrapperEnumType();
PyTypeObject PyWrapperEnumValueDescrType = InitializePyWrapperEnumValueDescrType();

FPyWrapperEnumMetaData::FPyWrapperEnumMetaData()
	: Enum(nullptr)
{
}

UEnum* FPyWrapperEnumMetaData::GetEnum(PyTypeObject* PyType)
{
	FPyWrapperEnumMetaData* PyWrapperMetaData = FPyWrapperEnumMetaData::GetMetaData(PyType);
	return PyWrapperMetaData ? PyWrapperMetaData->Enum : nullptr;
}

UEnum* FPyWrapperEnumMetaData::GetEnum(FPyWrapperEnum* Instance)
{
	return GetEnum(Py_TYPE(Instance));
}

struct FPythonGeneratedEnumUtil
{
	static UPythonGeneratedEnum* CreateEnum(const FString& InEnumName, UObject* InEnumOuter)
	{
		UPythonGeneratedEnum* Enum = NewObject<UPythonGeneratedEnum>(InEnumOuter, *InEnumName, RF_Public | RF_Standalone);
		Enum->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
		return Enum;
	}

	static void FinalizeEnum(UPythonGeneratedEnum* InEnum, PyTypeObject* InPyType)
	{
		// Finalize the enum
		InEnum->Bind();

		// Add the object meta-data to the type
		InEnum->PyMetaData.Enum = InEnum;
		FPyWrapperEnumMetaData::SetMetaData(InPyType, &InEnum->PyMetaData);

		// Map the Unreal enum to the Python type
		InEnum->PyType = FPyTypeObjectPtr::NewReference(InPyType);
		FPyWrapperTypeRegistry::Get().RegisterWrappedEnumType(InEnum->GetFName(), InPyType);
	}

	static bool CreateValueFromDefinition(UPythonGeneratedEnum* InEnum, PyTypeObject* InPyType, const FString& InFieldName, FPyUValueDef* InPyValueDef)
	{
		int64 EnumValue = 0;
		if (!PyConversion::Nativize(InPyValueDef->Value, EnumValue))
		{
			PyUtil::SetPythonError(PyExc_TypeError, InPyType, *FString::Printf(TEXT("Failed to convert enum value for '%s'"), *InFieldName));
			return false;
		}

		// Build the definition data for the new enum value
		UPythonGeneratedEnum::FEnumValueDef& EnumValueDef = *InEnum->EnumValueDefs[InEnum->EnumValueDefs.Add(MakeShared<UPythonGeneratedEnum::FEnumValueDef>())];
		EnumValueDef.Value = EnumValue;
		EnumValueDef.Name = InFieldName;

		return true;
	}

	static bool RegisterDescriptors(UPythonGeneratedEnum* InEnum, PyTypeObject* InPyType, const TArray<FPyUValueDef*>& InPyValueDefs)
	{
		// Populate the enum with its values
		check(InPyValueDefs.Num() == InEnum->EnumValueDefs.Num());
		{
			const FString EnumName = InEnum->GetName();

			TArray<TPair<FName, int64>> ValueNames;
			for (const TSharedPtr<UPythonGeneratedEnum::FEnumValueDef>& EnumValueDef : InEnum->EnumValueDefs)
			{
				const FString NamespacedValueName = FString::Printf(TEXT("%s::%s"), *EnumName, *EnumValueDef->Name);
				ValueNames.Emplace(MakeTuple(FName(*NamespacedValueName), EnumValueDef->Value));
			}
			if (!InEnum->SetEnums(ValueNames, UEnum::ECppForm::Namespaced))
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Failed to set enum values"));
				return false;
			}

			// Can't set the meta-data until SetEnums has been called
			for (int32 EnumEntryIndex = 0; EnumEntryIndex < InPyValueDefs.Num(); ++EnumEntryIndex)
			{
				FPyUValueDef::ApplyMetaData(InPyValueDefs[EnumEntryIndex], [InEnum, EnumEntryIndex](const FString& InMetaDataKey, const FString& InMetaDataValue)
				{
					InEnum->SetMetaData(*InMetaDataKey, *InMetaDataValue, EnumEntryIndex);
				});
				InEnum->EnumValueDefs[EnumEntryIndex]->DocString = PyGenUtil::GetEnumEntryTooltip(InEnum, EnumEntryIndex);
			}
		}

		// Replace the definitions with real descriptors
		for (const TSharedPtr<UPythonGeneratedEnum::FEnumValueDef>& EnumValueDef : InEnum->EnumValueDefs)
		{
			FPyWrapperEnum::SetEnumEntryValue(InPyType, EnumValueDef->Value, TCHAR_TO_UTF8(*EnumValueDef->Name), TCHAR_TO_UTF8(*EnumValueDef->DocString));
		}

		return true;
	}
};

UPythonGeneratedEnum* UPythonGeneratedEnum::GenerateEnum(PyTypeObject* InPyType)
{
	UObject* EnumOuter = GetPythonTypeContainer();
	const FString EnumName = PyUtil::GetCleanTypename(InPyType);

	UPythonGeneratedEnum* Enum = FindObject<UPythonGeneratedEnum>(EnumOuter, *EnumName);
	if (!Enum)
	{
		Enum = FPythonGeneratedEnumUtil::CreateEnum(EnumName, EnumOuter);
	}
	Enum->EnumValueDefs.Reset();

	// Add the values to this enum
	TArray<FPyUValueDef*> PyValueDefs;
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			const FString FieldName = PyUtil::PyObjectToUEString(FieldKey);

			if (PyObject_IsInstance(FieldValue, (PyObject*)&PyUValueDefType) == 1)
			{
				FPyUValueDef* PyValueDef = (FPyUValueDef*)FieldValue;
				PyValueDefs.Add(PyValueDef);

				if (!FPythonGeneratedEnumUtil::CreateValueFromDefinition(Enum, InPyType, FieldName, PyValueDef))
				{
					return nullptr;
				}
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&PyUPropertyDefType) == 1)
			{
				// Properties are not supported on enums
				PyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Enums do not support properties"));
				return nullptr;
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&PyUFunctionDefType) == 1)
			{
				// Functions are not supported on enums
				PyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Enums do not support methods"));
				return nullptr;
			}
		}
	}

	// Populate the enum with its values, and replace the definitions with real descriptors
	if (!FPythonGeneratedEnumUtil::RegisterDescriptors(Enum, InPyType, PyValueDefs))
	{
		return nullptr;
	}

	// Let Python know that we've changed its type
	PyType_Modified(InPyType);

	// Finalize the enum
	FPythonGeneratedEnumUtil::FinalizeEnum(Enum, InPyType);

	return Enum;
}

#endif	// WITH_PYTHON

// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyCore.h"
#include "PyUtil.h"
#include "PyConversion.h"
#include "PyReferenceCollector.h"

#include "PyWrapperBase.h"
#include "PyWrapperObject.h"
#include "PyWrapperStruct.h"
#include "PyWrapperEnum.h"
#include "PyWrapperDelegate.h"
#include "PyWrapperName.h"
#include "PyWrapperText.h"
#include "PyWrapperArray.h"
#include "PyWrapperFixedArray.h"
#include "PyWrapperSet.h"
#include "PyWrapperMap.h"
#include "PyWrapperMath.h"

#include "PythonScriptPlugin.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/PackageName.h"
#include "Misc/OutputDeviceRedirector.h"

#if WITH_PYTHON

namespace PyCoreUtil
{

bool ConvertOptionalString(PyObject* InObj, FString& OutString, const TCHAR* InErrorCtxt, const TCHAR* InErrorMsg)
{
	OutString.Reset();
	if (InObj && InObj != Py_None)
	{
		if (!PyConversion::Nativize(InObj, OutString))
		{
			PyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, InErrorMsg);
			return false;
		}
	}
	return true;
}

bool ConvertOptionalFunctionFlag(PyObject* InObj, EPyUFunctionDefFlags& OutFlags, const EPyUFunctionDefFlags InTrueFlagBit, const EPyUFunctionDefFlags InFalseFlagBit, const TCHAR* InErrorCtxt, const TCHAR* InErrorMsg)
{
	if (InObj && InObj != Py_None)
	{
		bool bFlagValue = false;
		if (!PyConversion::Nativize(InObj, bFlagValue))
		{
			PyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, InErrorMsg);
			return false;
		}
		OutFlags |= (bFlagValue ? InTrueFlagBit : InFalseFlagBit);
	}
	return true;
}

void ApplyMetaData(PyObject* InMetaData, const TFunctionRef<void(const FString&, const FString&)>& InPredicate)
{
	if (PyDict_Check(InMetaData))
	{
		PyObject* MetaDataKey = nullptr;
		PyObject* MetaDataValue = nullptr;
		Py_ssize_t MetaDataIndex = 0;
		while (PyDict_Next(InMetaData, &MetaDataIndex, &MetaDataKey, &MetaDataValue))
		{
			const FString MetaDataKeyStr = PyUtil::PyObjectToUEString(MetaDataKey);
			const FString MetaDataValueStr = PyUtil::PyObjectToUEString(MetaDataValue);
			InPredicate(MetaDataKeyStr, MetaDataValueStr);
		}
	}
}

} // namespace PyCoreUtil

TStrongObjectPtr<UStruct> GPythonPropertyContainer;
TStrongObjectPtr<UPackage> GPythonTypeContainer;

UObject* GetPythonPropertyContainer()
{
	return GPythonPropertyContainer.Get();
}

UObject* GetPythonTypeContainer()
{
	return GPythonTypeContainer.Get();
}

FPyUValueDef* FPyUValueDef::New(PyTypeObject* InType)
{
	FPyUValueDef* Self = (FPyUValueDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->Value = nullptr;
		Self->MetaData = nullptr;
	}
	return Self;
}

void FPyUValueDef::Free(FPyUValueDef* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FPyUValueDef::Init(FPyUValueDef* InSelf, PyObject* InValue, PyObject* InMetaData)
{
	Deinit(InSelf);

	Py_INCREF(InValue);
	InSelf->Value = InValue;

	Py_INCREF(InMetaData);
	InSelf->MetaData = InMetaData;

	return 0;
}

void FPyUValueDef::Deinit(FPyUValueDef* InSelf)
{
	Py_XDECREF(InSelf->Value);
	InSelf->Value = nullptr;

	Py_XDECREF(InSelf->MetaData);
	InSelf->MetaData = nullptr;
}

void FPyUValueDef::ApplyMetaData(FPyUValueDef* InSelf, const TFunctionRef<void(const FString&, const FString&)>& InPredicate)
{
	PyCoreUtil::ApplyMetaData(InSelf->MetaData, InPredicate);
}


FPyUPropertyDef* FPyUPropertyDef::New(PyTypeObject* InType)
{
	FPyUPropertyDef* Self = (FPyUPropertyDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->PropType = nullptr;
		Self->MetaData = nullptr;
		new(&Self->GetterFuncName) FString();
		new(&Self->SetterFuncName) FString();
	}
	return Self;
}

void FPyUPropertyDef::Free(FPyUPropertyDef* InSelf)
{
	Deinit(InSelf);

	InSelf->GetterFuncName.~FString();
	InSelf->SetterFuncName.~FString();
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FPyUPropertyDef::Init(FPyUPropertyDef* InSelf, PyObject* InPropType, PyObject* InMetaData, FString InGetterFuncName, FString InSetterFuncName)
{
	Deinit(InSelf);

	Py_INCREF(InPropType);
	InSelf->PropType = InPropType;

	Py_INCREF(InMetaData);
	InSelf->MetaData = InMetaData;

	InSelf->GetterFuncName = MoveTemp(InGetterFuncName);
	InSelf->SetterFuncName = MoveTemp(InSetterFuncName);

	return 0;
}

void FPyUPropertyDef::Deinit(FPyUPropertyDef* InSelf)
{
	Py_XDECREF(InSelf->PropType);
	InSelf->PropType = nullptr;

	Py_XDECREF(InSelf->MetaData);
	InSelf->MetaData = nullptr;

	InSelf->GetterFuncName.Reset();
	InSelf->SetterFuncName.Reset();
}

void FPyUPropertyDef::ApplyMetaData(FPyUPropertyDef* InSelf, UProperty* InProp)
{
	PyCoreUtil::ApplyMetaData(InSelf->MetaData, [InProp](const FString& InMetaDataKey, const FString& InMetaDataValue)
	{
		InProp->SetMetaData(*InMetaDataKey, *InMetaDataValue);
	});
}


FPyUFunctionDef* FPyUFunctionDef::New(PyTypeObject* InType)
{
	FPyUFunctionDef* Self = (FPyUFunctionDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->Func = nullptr;
		Self->FuncRetType = nullptr;
		Self->FuncParamTypes = nullptr;
		Self->MetaData = nullptr;
		Self->FuncFlags = EPyUFunctionDefFlags::None;
	}
	return Self;
}

void FPyUFunctionDef::Free(FPyUFunctionDef* InSelf)
{
	Deinit(InSelf);

	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FPyUFunctionDef::Init(FPyUFunctionDef* InSelf, PyObject* InFunc, PyObject* InFuncRetType, PyObject* InFuncParamTypes, PyObject* InMetaData, EPyUFunctionDefFlags InFuncFlags)
{
	Deinit(InSelf);

	Py_INCREF(InFunc);
	InSelf->Func = InFunc;

	Py_INCREF(InFuncRetType);
	InSelf->FuncRetType = InFuncRetType;

	Py_INCREF(InFuncParamTypes);
	InSelf->FuncParamTypes = InFuncParamTypes;

	Py_INCREF(InMetaData);
	InSelf->MetaData = InMetaData;

	InSelf->FuncFlags = InFuncFlags;

	return 0;
}

void FPyUFunctionDef::Deinit(FPyUFunctionDef* InSelf)
{
	Py_XDECREF(InSelf->Func);
	InSelf->Func = nullptr;

	Py_XDECREF(InSelf->FuncRetType);
	InSelf->FuncRetType = nullptr;

	Py_XDECREF(InSelf->FuncParamTypes);
	InSelf->FuncParamTypes = nullptr;

	Py_XDECREF(InSelf->MetaData);
	InSelf->MetaData = nullptr;

	InSelf->FuncFlags = EPyUFunctionDefFlags::None;
}

void FPyUFunctionDef::ApplyMetaData(FPyUFunctionDef* InSelf, UFunction* InFunc)
{
	PyCoreUtil::ApplyMetaData(InSelf->MetaData, [InFunc](const FString& InMetaDataKey, const FString& InMetaDataValue)
	{
		InFunc->SetMetaData(*InMetaDataKey, *InMetaDataValue);
	});
}


PyTypeObject InitializePyUValueDefType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyUValueDef::New(InType);
		}

		static void Dealloc(FPyUValueDef* InSelf)
		{
			FPyUValueDef::Free(InSelf);
		}

		static int Init(FPyUValueDef* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyValueObj = nullptr;
			PyObject* PyMetaObj = nullptr;

			static const char *ArgsKwdList[] = { "val", "meta", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:call", (char**)ArgsKwdList, &PyValueObj, &PyMetaObj))
			{
				return -1;
			}

			if (PyValueObj == Py_None)
			{
				PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("'val' cannot be 'None'"));
				return -1;
			}

			return FPyUValueDef::Init(InSelf, PyValueObj, PyMetaObj);
		}
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"ValueDef", /* tp_name */
		sizeof(FPyUValueDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define constant values from Python";

	return PyType;
}


PyTypeObject InitializePyUPropertyDefType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyUPropertyDef::New(InType);
		}

		static void Dealloc(FPyUPropertyDef* InSelf)
		{
			FPyUPropertyDef::Free(InSelf);
		}

		static int Init(FPyUPropertyDef* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyPropTypeObj = nullptr;
			PyObject* PyMetaObj = nullptr;
			PyObject* PyPropGetterObj = nullptr;
			PyObject* PyPropSetterObj = nullptr;

			static const char *ArgsKwdList[] = { "type", "meta", "getter", "setter", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OOOO:call", (char**)ArgsKwdList, &PyPropTypeObj, &PyMetaObj, &PyPropGetterObj, &PyPropSetterObj))
			{
				return -1;
			}

			const FString ErrorCtxt = PyUtil::GetErrorContext(InSelf);

			FString PropGetter;
			if (!PyCoreUtil::ConvertOptionalString(PyPropGetterObj, PropGetter, *ErrorCtxt, TEXT("Failed to convert parameter 'getter' to a string (expected 'None' or 'str')")))
			{
				return -1;
			}

			FString PropSetter;
			if (!PyCoreUtil::ConvertOptionalString(PyPropSetterObj, PropSetter, *ErrorCtxt, TEXT("Failed to convert parameter 'setter' to a string (expected 'None' or 'str')")))
			{
				return -1;
			}

			return FPyUPropertyDef::Init(InSelf, PyPropTypeObj, PyMetaObj, MoveTemp(PropGetter), MoveTemp(PropSetter));
		}
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"PropertyDef", /* tp_name */
		sizeof(FPyUPropertyDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define UProperty fields from Python";

	return PyType;
}


PyTypeObject InitializePyUFunctionDefType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyUFunctionDef::New(InType);
		}

		static void Dealloc(FPyUFunctionDef* InSelf)
		{
			FPyUFunctionDef::Free(InSelf);
		}

		static int Init(FPyUFunctionDef* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyFuncObj = nullptr;
			PyObject* PyMetaObj = nullptr;
			PyObject* PyFuncRetTypeObj = nullptr;
			PyObject* PyFuncParamTypesObj = nullptr;
			PyObject* PyFuncOverrideObj = nullptr;
			PyObject* PyFuncStaticObj = nullptr;
			PyObject* PyFuncPureObj = nullptr;
			PyObject* PyFuncGetterObj = nullptr;
			PyObject* PyFuncSetterObj = nullptr;

			static const char *ArgsKwdList[] = { "func", "meta", "ret", "params", "override", "static", "pure", "getter", "setter", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OOOOOOOOO:call", (char**)ArgsKwdList, &PyFuncObj, &PyMetaObj, &PyFuncRetTypeObj, &PyFuncParamTypesObj, &PyFuncOverrideObj, &PyFuncStaticObj, &PyFuncPureObj, &PyFuncGetterObj, &PyFuncSetterObj))
			{
				return -1;
			}

			const FString ErrorCtxt = PyUtil::GetErrorContext(InSelf);

			EPyUFunctionDefFlags FuncFlags = EPyUFunctionDefFlags::None;
			if (!PyCoreUtil::ConvertOptionalFunctionFlag(PyFuncOverrideObj, FuncFlags, EPyUFunctionDefFlags::Override, EPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'override' to a flag (expected 'None' or 'bool')")))
			{
				return -1;
			}
			if (!PyCoreUtil::ConvertOptionalFunctionFlag(PyFuncStaticObj, FuncFlags, EPyUFunctionDefFlags::Static, EPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'static' to a flag (expected 'None' or 'bool')")))
			{
				return -1;
			}
			if (!PyCoreUtil::ConvertOptionalFunctionFlag(PyFuncPureObj, FuncFlags, EPyUFunctionDefFlags::Pure, EPyUFunctionDefFlags::Impure, *ErrorCtxt, TEXT("Failed to convert parameter 'pure' to a flag (expected 'None' or 'bool')")))
			{
				return -1;
			}
			if (!PyCoreUtil::ConvertOptionalFunctionFlag(PyFuncGetterObj, FuncFlags, EPyUFunctionDefFlags::Getter, EPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'getter' to a flag (expected 'None' or 'bool')")))
			{
				return -1;
			}
			if (!PyCoreUtil::ConvertOptionalFunctionFlag(PyFuncSetterObj, FuncFlags, EPyUFunctionDefFlags::Setter, EPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'setter' to a flag (expected 'None' or 'bool')")))
			{
				return -1;
			}

			return FPyUFunctionDef::Init(InSelf, PyFuncObj, PyFuncRetTypeObj, PyFuncParamTypesObj, PyMetaObj, FuncFlags);
		}
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"FunctionDef", /* tp_name */
		sizeof(FPyUFunctionDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define UFunction fields from Python";

	return PyType;
}


PyTypeObject PyUValueDefType = InitializePyUValueDefType();
PyTypeObject PyUPropertyDefType = InitializePyUPropertyDefType();
PyTypeObject PyUFunctionDefType = InitializePyUFunctionDefType();


namespace PyCore
{

PyObject* Log(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log", &PyObj))
	{
		const FString LogMessage = PyUtil::PyObjectToUEString(PyObj);
		UE_LOG(LogPython, Log, TEXT("%s"), *LogMessage);

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* LogWarning(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log", &PyObj))
	{
		const FString LogMessage = PyUtil::PyObjectToUEString(PyObj);
		UE_LOG(LogPython, Warning, TEXT("%s"), *LogMessage);

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* LogError(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log", &PyObj))
	{
		const FString LogMessage = PyUtil::PyObjectToUEString(PyObj);
		UE_LOG(LogPython, Error, TEXT("%s"), *LogMessage);

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* LogFlush(PyObject* InSelf)
{
	if (GLog)
	{
		GLog->Flush();
	}

	Py_RETURN_NONE;
}

PyObject* Reload(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:reload", &PyObj))
	{
		return nullptr;
	}
	
	FString ModuleName;
	if (!PyConversion::Nativize(PyObj, ModuleName))
	{
		return nullptr;
	}

	FPythonScriptPlugin* PythonScriptPlugin = FPythonScriptPlugin::Get();
	if (PythonScriptPlugin)
	{
		PythonScriptPlugin->ImportUnrealModule(*ModuleName);
	}

	Py_RETURN_NONE;
}

PyObject* FindOrLoadObjectImpl(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds, const TCHAR* InFuncName, const TFunctionRef<UObject*(UClass*, UObject*, const TCHAR*)>& InFunc)
{
	PyObject* PyOuterObj = nullptr;
	PyObject* PyNameObj = nullptr;
	PyObject* PyTypeObj = nullptr;

	static const char *ArgsKwdList[] = { "outer", "name", "type", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, TCHAR_TO_UTF8(*FString::Printf(TEXT("OO|O:%s"), InFuncName)), (char**)ArgsKwdList, &PyOuterObj, &PyNameObj, &PyTypeObj))
	{
		return nullptr;
	}

	UObject* ObjectOuter = nullptr;
	if (!PyConversion::Nativize(PyOuterObj, ObjectOuter))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *PyUtil::GetFriendlyTypename(PyOuterObj)));
		return nullptr;
	}

	FString ObjectName;
	if (!PyConversion::Nativize(PyNameObj, ObjectName))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'String'"), *PyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	UClass* ObjectType = UObject::StaticClass();
	if (PyTypeObj && !PyConversion::NativizeClass(PyTypeObj, ObjectType, UObject::StaticClass()))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class'"), *PyUtil::GetFriendlyTypename(PyTypeObj)));
		return nullptr;
	}

	return PyConversion::Pythonize(InFunc(ObjectType, ObjectOuter, *ObjectName));
}

PyObject* FindObject(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadObjectImpl(InSelf, InArgs, InKwds, TEXT("find_object"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		return ::StaticFindObject(ObjectType, ObjectOuter, ObjectName);
	});
}

PyObject* LoadObject(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadObjectImpl(InSelf, InArgs, InKwds, TEXT("load_object"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		return ::StaticLoadObject(ObjectType, ObjectOuter, ObjectName);
	});
}

PyObject* LoadClass(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadObjectImpl(InSelf, InArgs, InKwds, TEXT("load_class"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		return ::StaticLoadClass(ObjectType, ObjectOuter, ObjectName);
	});
}

PyObject* FindOrLoadAssetImpl(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds, const TCHAR* InFuncName, const TFunctionRef<UObject*(UClass*, UObject*, const TCHAR*)>& InFunc)
{
	PyObject* PyOuterObj = nullptr;
	PyObject* PyNameObj = nullptr;
	PyObject* PyTypeObj = nullptr;

	static const char *ArgsKwdList[] = { "outer", "name", "type", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, TCHAR_TO_UTF8(*FString::Printf(TEXT("OO|O:%s"), InFuncName)), (char**)ArgsKwdList, &PyOuterObj, &PyNameObj, &PyTypeObj))
	{
		return nullptr;
	}

	UObject* ObjectOuter = nullptr;
	if (!PyConversion::Nativize(PyOuterObj, ObjectOuter))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *PyUtil::GetFriendlyTypename(PyOuterObj)));
		return nullptr;
	}

	FString ObjectName;
	if (!PyConversion::Nativize(PyNameObj, ObjectName))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'String'"), *PyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	UClass* ObjectType = UObject::StaticClass();
	if (PyTypeObj && !PyConversion::NativizeClass(PyTypeObj, ObjectType, UObject::StaticClass()))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class'"), *PyUtil::GetFriendlyTypename(PyTypeObj)));
		return nullptr;
	}

	UObject* PotentialAsset = InFunc(UObject::StaticClass(), ObjectOuter, *ObjectName);
	
	// If we found a package, try and get the primary asset from it
	if (UPackage* FoundPackage = Cast<UPackage>(PotentialAsset))
	{
		PotentialAsset = InFunc(UObject::StaticClass(), FoundPackage, *FPackageName::GetShortName(FoundPackage));
	}

	// Make sure the object is an asset of the correct type
	if (PotentialAsset && (!PotentialAsset->IsAsset() || !PotentialAsset->IsA(ObjectType)))
	{
		PotentialAsset = nullptr;
	}

	return PyConversion::Pythonize(PotentialAsset);
}

PyObject* FindAsset(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadAssetImpl(InSelf, InArgs, InKwds, TEXT("find_asset"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		return ::StaticFindObject(ObjectType, ObjectOuter, ObjectName);
	});
}

PyObject* LoadAsset(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadAssetImpl(InSelf, InArgs, InKwds, TEXT("load_asset"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		return ::StaticLoadObject(ObjectType, ObjectOuter, ObjectName);
	});
}

PyObject* FindOrLoadPackageImpl(PyObject* InSelf, PyObject* InArgs, const TCHAR* InFuncName, const TFunctionRef<UPackage*(UPackage*, const TCHAR*)>& InFunc)
{
	PyObject* PyOuterObj = nullptr;
	PyObject* PyNameObj = nullptr;

	if (!PyArg_ParseTuple(InArgs, TCHAR_TO_UTF8(*FString::Printf(TEXT("OO|O:%s"), InFuncName)), &PyOuterObj, &PyNameObj))
	{
		return nullptr;
	}

	UPackage* PackageOuter = nullptr;
	if (!PyConversion::Nativize(PyOuterObj, PackageOuter))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Package'"), *PyUtil::GetFriendlyTypename(PyOuterObj)));
		return nullptr;
	}

	FString PackageName;
	if (!PyConversion::Nativize(PyNameObj, PackageName))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'String'"), *PyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	return PyConversion::Pythonize(InFunc(PackageOuter, *PackageName));
}

PyObject* FindPackage(PyObject* InSelf, PyObject* InArgs)
{
	return FindOrLoadPackageImpl(InSelf, InArgs, TEXT("find_package"), [](UPackage* PackageOuter, const TCHAR* PackageName)
	{
		return ::FindPackage(PackageOuter, PackageName);
	});
}

PyObject* LoadPackage(PyObject* InSelf, PyObject* InArgs)
{
	return FindOrLoadPackageImpl(InSelf, InArgs, TEXT("load_package"), [](UPackage* PackageOuter, const TCHAR* PackageName)
	{
		return ::LoadPackage(PackageOuter, PackageName, LOAD_None);
	});
}

PyObject* PurgeObjectReferences(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	PyObject* PyIncludeInnersObj = nullptr;

	static const char *ArgsKwdList[] = { "obj", "include_inners", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:purge_object_references", (char**)ArgsKwdList, &PyObj, &PyIncludeInnersObj))
	{
		return nullptr;
	}

	UObject* Object = nullptr;
	if (!PyConversion::Nativize(PyObj, Object))
	{
		PyUtil::SetPythonError(PyExc_TypeError, TEXT("purge_object_references"), *FString::Printf(TEXT("Failed to convert 'obj' (%s) to 'Object'"), *PyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	bool bIncludeInners = true;
	if (PyIncludeInnersObj && !PyConversion::Nativize(PyIncludeInnersObj, bIncludeInners))
	{
		PyUtil::SetPythonError(PyExc_TypeError, TEXT("purge_object_references"), *FString::Printf(TEXT("Failed to convert 'include_inners' (%s) to 'bool'"), *PyUtil::GetFriendlyTypename(PyIncludeInnersObj)));
		return nullptr;
	}

	FPyReferenceCollector::Get().PurgeUnrealObjectReferences(Object, bIncludeInners);

	Py_RETURN_NONE;
}

PyObject* GenerateClass(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:generate_class", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	if (!PyType_Check(PyObj))
	{
		PyUtil::SetPythonError(PyExc_TypeError, TEXT("generate_class"), *FString::Printf(TEXT("Parameter must be a 'type' not '%s'"), *PyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!PyType_IsSubtype(PyType, &PyWrapperObjectType))
	{
		PyUtil::SetPythonError(PyExc_Exception, TEXT("generate_class"), *FString::Printf(TEXT("Type '%s' does not derive from an Unreal class type"), *PyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}
	
	// We only need to generate classes for types without meta-data, as any types with meta-data have already been generated
	if (!FPyWrapperObjectMetaData::GetMetaData(PyType) && !UPythonGeneratedClass::GenerateClass(PyType))
	{
		PyUtil::SetPythonError(PyExc_Exception, TEXT("generate_class"), *FString::Printf(TEXT("Failed to generate an Unreal class for the Python type '%s'"), *PyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* GenerateStruct(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:generate_struct", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	if (!PyType_Check(PyObj))
	{
		PyUtil::SetPythonError(PyExc_TypeError, TEXT("generate_struct"), *FString::Printf(TEXT("Parameter must be a 'type' not '%s'"), *PyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!PyType_IsSubtype(PyType, &PyWrapperStructType))
	{
		PyUtil::SetPythonError(PyExc_Exception, TEXT("generate_struct"), *FString::Printf(TEXT("Type '%s' does not derive from an Unreal struct type"), *PyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	// We only need to generate structs for types without meta-data, as any types with meta-data have already been generated
	if (!FPyWrapperStructMetaData::GetMetaData(PyType) && !UPythonGeneratedStruct::GenerateStruct(PyType))
	{
		PyUtil::SetPythonError(PyExc_Exception, TEXT("generate_struct"), *FString::Printf(TEXT("Failed to generate an Unreal struct for the Python type '%s'"), *PyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* GenerateEnum(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:generate_enum", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	if (!PyType_Check(PyObj))
	{
		PyUtil::SetPythonError(PyExc_TypeError, TEXT("generate_enum"), *FString::Printf(TEXT("Parameter must be a 'type' not '%s'"), *PyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!PyType_IsSubtype(PyType, &PyWrapperEnumType))
	{
		PyUtil::SetPythonError(PyExc_Exception, TEXT("generate_enum"), *FString::Printf(TEXT("Type '%s' does not derive from the Unreal enum type"), *PyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	// We only need to generate enums for types without meta-data, as any types with meta-data have already been generated
	if (!FPyWrapperEnumMetaData::GetMetaData(PyType) && !UPythonGeneratedEnum::GenerateEnum(PyType))
	{
		PyUtil::SetPythonError(PyExc_Exception, TEXT("generate_enum"), *FString::Printf(TEXT("Failed to generate an Unreal enum for the Python type '%s'"), *PyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* CreateLocalizedText(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyNamespaceObj = nullptr;
	PyObject* PyKeyObj = nullptr;
	PyObject* PySourceObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "OOO:NSLOCTEXT", &PyNamespaceObj, &PyKeyObj, &PySourceObj))
	{
		return nullptr;
	}

	FString Namespace;
	if (!PyConversion::Nativize(PyNamespaceObj, Namespace))
	{
		return nullptr;
	}

	FString Key;
	if (!PyConversion::Nativize(PyKeyObj, Key))
	{
		return nullptr;
	}

	FString Source;
	if (!PyConversion::Nativize(PySourceObj, Source))
	{
		return nullptr;
	}

	return PyConversion::Pythonize(FInternationalization::Get().ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(*Source, *Namespace, *Key));
}

PyObject* CreateLocalizedTextFromStringTable(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyIdObj = nullptr;
	PyObject* PyKeyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "OO:LOCTABLE", &PyIdObj, &PyKeyObj))
	{
		return nullptr;
	}

	FName Id;
	if (!PyConversion::Nativize(PyIdObj, Id))
	{
		return nullptr;
	}

	FString Key;
	if (!PyConversion::Nativize(PyKeyObj, Key))
	{
		return nullptr;
	}

	return PyConversion::Pythonize(FText::FromStringTable(Id, Key));
}

PyMethodDef PyCoreMethods[] = {
	{ "log", PyCFunctionCast(&Log), METH_VARARGS, "x.log(str) -- log the given argument as information in the LogPython category" },
	{ "log_warning", PyCFunctionCast(&LogWarning), METH_VARARGS, "x.log_warning(str) -- log the given argument as a warning in the LogPython category" },
	{ "log_error", PyCFunctionCast(&LogError), METH_VARARGS, "x.log_error(str) -- log the given argument as an error in the LogPython category" },
	{ "log_flush", PyCFunctionCast(&LogFlush), METH_NOARGS, "x.log_flush() -- flush the log to disk" },
	{ "reload", PyCFunctionCast(&Reload), METH_VARARGS, "x.reload(str) -- reload the given Unreal Python module" },
	{ "find_object", PyCFunctionCast(&FindObject), METH_VARARGS | METH_KEYWORDS, "x.find_object(outer, name, type=Object) -> Object -- find an Unreal object with the given outer and name, optionally validating its type" },
	{ "load_object", PyCFunctionCast(&LoadObject), METH_VARARGS | METH_KEYWORDS, "x.load_object(outer, name, type=Object) -> Object -- load an Unreal object with the given outer and name, optionally validating its type" },
	{ "load_class", PyCFunctionCast(&LoadClass), METH_VARARGS | METH_KEYWORDS, "x.load_class(outer, name, type=Object) -> Class -- load an Unreal class with the given outer and name, optionally validating its base type" },
	{ "find_asset", PyCFunctionCast(&FindAsset), METH_VARARGS | METH_KEYWORDS, "x.find_asset(outer, name, type=Object) -> Object -- find an Unreal asset with the given outer and name, optionally validating its type" },
	{ "load_asset", PyCFunctionCast(&LoadAsset), METH_VARARGS | METH_KEYWORDS, "x.load_asset(outer, name, type=Object) -> Object -- load an Unreal asset with the given outer and name, optionally validating its type" },
	{ "find_package", PyCFunctionCast(&FindPackage), METH_VARARGS, "x.find_package(outer, name) -> Package -- find an Unreal package with the given outer and name" },
	{ "load_package", PyCFunctionCast(&LoadPackage), METH_VARARGS, "x.load_package(outer, name) -> Package -- load an Unreal package with the given outer and name" },
	{ "purge_object_references", PyCFunctionCast(&PurgeObjectReferences), METH_VARARGS, "x.purge_object_references(obj, include_inners=True) -- purge all references to the given Unreal object from any living Python objects" },
	{ "generate_class", PyCFunctionCast(&GenerateClass), METH_VARARGS, "x.generate_class(type) -- generate an Unreal class for the given Python type" },
	{ "generate_struct", PyCFunctionCast(&GenerateStruct), METH_VARARGS, "x.generate_struct(type) -- generate an Unreal struct for the given Python type" },
	{ "generate_enum", PyCFunctionCast(&GenerateEnum), METH_VARARGS, "x.generate_enum(type) -- generate an Unreal enum for the given Python type" },
	{ "NSLOCTEXT", PyCFunctionCast(&CreateLocalizedText), METH_VARARGS, "x.NSLOCTEXT(ns, key, source) -> FText -- create a localized FText from the given namespace, key, and source string" },
	{ "LOCTABLE", PyCFunctionCast(&CreateLocalizedTextFromStringTable), METH_VARARGS, "x.LOCTABLE(id, key) -> FText -- get a localized FText from the given string table id and key" },
	{ nullptr, nullptr, 0, nullptr }
};

void InitializeModule()
{
	GPythonPropertyContainer.Reset(NewObject<UStruct>(GetTransientPackage(), TEXT("PythonProperties")));

	GPythonTypeContainer.Reset(NewObject<UPackage>(nullptr, TEXT("/Engine/PythonTypes"), RF_Public));
	GPythonTypeContainer->SetPackageFlags(PKG_CompiledIn | PKG_ContainsScript);

#if PY_MAJOR_VERSION >= 3
	PyObject* PyModule = PyImport_AddModule("_unreal_core");
	PyModule_AddFunctions(PyModule, PyCoreMethods);
#else	// PY_MAJOR_VERSION >= 3
	PyObject* PyModule = Py_InitModule("_unreal_core", PyCoreMethods);
#endif	// PY_MAJOR_VERSION >= 3

	if (PyType_Ready(&PyUValueDefType) == 0)
	{
		Py_INCREF(&PyUValueDefType);
		PyModule_AddObject(PyModule, PyUValueDefType.tp_name, (PyObject*)&PyUValueDefType);
	}

	if (PyType_Ready(&PyUPropertyDefType) == 0)
	{
		Py_INCREF(&PyUPropertyDefType);
		PyModule_AddObject(PyModule, PyUPropertyDefType.tp_name, (PyObject*)&PyUPropertyDefType);
	}

	if (PyType_Ready(&PyUFunctionDefType) == 0)
	{
		Py_INCREF(&PyUFunctionDefType);
		PyModule_AddObject(PyModule, PyUFunctionDefType.tp_name, (PyObject*)&PyUFunctionDefType);
	}

	InitializePyWrapperBase(PyModule);
	InitializePyWrapperObject(PyModule);
	InitializePyWrapperStruct(PyModule);
	InitializePyWrapperEnum(PyModule);
	InitializePyWrapperDelegate(PyModule);
	InitializePyWrapperName(PyModule);
	InitializePyWrapperText(PyModule);
	InitializePyWrapperArray(PyModule);
	InitializePyWrapperFixedArray(PyModule);
	InitializePyWrapperSet(PyModule);
	InitializePyWrapperMap(PyModule);
	InitializePyWrapperMath(PyModule);
}

}

#endif	// WITH_PYTHON

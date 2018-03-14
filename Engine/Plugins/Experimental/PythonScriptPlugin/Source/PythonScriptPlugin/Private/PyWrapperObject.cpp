// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperObject.h"
#include "PyWrapperOwnerContext.h"
#include "PyWrapperTypeRegistry.h"
#include "PyCore.h"
#include "PyUtil.h"
#include "PyConversion.h"
#include "UObject/Package.h"
#include "UObject/Class.h"
#include "UObject/MetaData.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"
#include "UObject/StructOnScope.h"
#include "UObject/PropertyPortFlags.h"
#include "Engine/World.h"
#include "Templates/Casts.h"

#if WITH_PYTHON

void InitializePyWrapperObject(PyObject* PyModule)
{
	if (PyType_Ready(&PyWrapperObjectType) == 0)
	{
		static FPyWrapperObjectMetaData MetaData;
		FPyWrapperObjectMetaData::SetMetaData(&PyWrapperObjectType, &MetaData);

		Py_INCREF(&PyWrapperObjectType);
		PyModule_AddObject(PyModule, PyWrapperObjectType.tp_name, (PyObject*)&PyWrapperObjectType);
	}
}

FPyWrapperObject* FPyWrapperObject::New(PyTypeObject* InType)
{
	FPyWrapperObject* Self = (FPyWrapperObject*)FPyWrapperBase::New(InType);
	if (Self)
	{
		Self->ObjectInstance = nullptr;
	}
	return Self;
}

void FPyWrapperObject::Free(FPyWrapperObject* InSelf)
{
	Deinit(InSelf);

	FPyWrapperBase::Free(InSelf);
}

int FPyWrapperObject::Init(FPyWrapperObject* InSelf, UObject* InValue)
{
	Deinit(InSelf);

	const int BaseInit = FPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InValue);

	InSelf->ObjectInstance = InValue;
	FPyWrapperObjectFactory::Get().MapInstance(InSelf->ObjectInstance, InSelf);
	return 0;
}

void FPyWrapperObject::Deinit(FPyWrapperObject* InSelf)
{
	if (InSelf->ObjectInstance)
	{
		FPyWrapperObjectFactory::Get().UnmapInstance(InSelf->ObjectInstance, Py_TYPE(InSelf));
	}
	InSelf->ObjectInstance = nullptr;
}

bool FPyWrapperObject::ValidateInternalState(FPyWrapperObject* InSelf)
{
	if (!InSelf->ObjectInstance)
	{
		PyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - ObjectInstance is null!"));
		return false;
	}

	return true;
}

FPyWrapperObject* FPyWrapperObject::CastPyObject(PyObject* InPyObject)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperObjectType) == 1)
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperObject*)InPyObject;
	}

	return nullptr;
}

FPyWrapperObject* FPyWrapperObject::CastPyObject(PyObject* InPyObject, PyTypeObject* InType)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &PyWrapperObjectType || PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperObjectType) == 1))
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperObject*)InPyObject;
	}

	return nullptr;
}

PyObject* FPyWrapperObject::GetPropertyValue(FPyWrapperObject* InSelf, const FName InPropName, const char* InPythonAttrName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return PyUtil::GetUEPropValue(InSelf->ObjectInstance->GetClass(), InSelf->ObjectInstance, InPropName, InPythonAttrName, (PyObject*)InSelf, *PyUtil::GetErrorContext(InSelf));
}

int FPyWrapperObject::SetPropertyValue(FPyWrapperObject* InSelf, PyObject* InValue, const FName InPropName, const char* InPythonAttrName, const bool InNotifyChange, const uint64 InReadOnlyFlags)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const FPyWrapperOwnerContext ChangeOwner = InNotifyChange ? FPyWrapperOwnerContext((PyObject*)InSelf, InPropName) : FPyWrapperOwnerContext();
	return PyUtil::SetUEPropValue(InSelf->ObjectInstance->GetClass(), InSelf->ObjectInstance, InValue, InPropName, InPythonAttrName, ChangeOwner, InReadOnlyFlags, *PyUtil::GetErrorContext(InSelf));
}

PyObject* FPyWrapperObject::CallGetterFunction(UClass* InClass, FPyWrapperObject* InSelf, const FName InFuncName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return CallFunction_Impl(InClass, InSelf->ObjectInstance, InFuncName, *PyUtil::GetErrorContext(InSelf));
}

int FPyWrapperObject::CallSetterFunction(UClass* InClass, FPyWrapperObject* InSelf, PyObject* InValue, const FName InFuncName)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UFunction* Func = InClass->FindFunctionByName(InFuncName);
	if (ensureAlways(Func))
	{
		check(InSelf->ObjectInstance->IsA(InClass) || InSelf->ObjectInstance->GetClass()->ImplementsInterface(InClass));

		FStructOnScope FuncParams(Func);

		UProperty* ArgProp = nullptr;
		for (TFieldIterator<UProperty> ParamIt(FuncParams.GetStruct()); ParamIt; ++ParamIt)
		{
			UProperty* Param = *ParamIt;
			if (!Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ArgProp = Param;
				break;
			}
		}

		if (!ArgProp)
		{
			PyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Failed to find input parameter when calling function '%s' on '%s'"), *Func->GetName(), *InSelf->ObjectInstance->GetName()));
			return -1;
		}

		if (InValue)
		{
			if (!PyConversion::NativizeProperty_InContainer(InValue, ArgProp, FuncParams.GetStructMemory(), 0))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert input parameter when calling function '%s' on '%s'"), *Func->GetName(), *InSelf->ObjectInstance->GetName()));
				return -1;
			}
		}

		CallFunction_InvokeImpl(InSelf->ObjectInstance, Func, FuncParams.GetStructMemory(), *PyUtil::GetErrorContext(InSelf));
	}

	return 0;
}

PyObject* FPyWrapperObject::CallFunction(UClass* InClass, PyTypeObject* InType, const FName InFuncName)
{
	UObject* Obj = InClass ? InClass->GetDefaultObject() : nullptr;
	return CallFunction_Impl(InClass, Obj, InFuncName, *PyUtil::GetErrorContext(InType));
}

PyObject* FPyWrapperObject::CallFunction(UClass* InClass, PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, const FName InFuncName, const char* InPythonFuncName, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef)
{
	UObject* Obj = InClass ? InClass->GetDefaultObject() : nullptr;
	return CallFunction_Impl(InClass, Obj, InArgs, InKwds, InFuncName, InPythonFuncName, InParamDef, *PyUtil::GetErrorContext(InType));
}

PyObject* FPyWrapperObject::CallFunction(UClass* InClass, FPyWrapperObject* InSelf, const FName InFuncName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return CallFunction_Impl(InClass, InSelf->ObjectInstance, InFuncName, *PyUtil::GetErrorContext(InSelf));
}

PyObject* FPyWrapperObject::CallFunction(UClass* InClass, FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds, const FName InFuncName, const char* InPythonFuncName, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return CallFunction_Impl(InClass, InSelf->ObjectInstance, InArgs, InKwds, InFuncName, InPythonFuncName, InParamDef, *PyUtil::GetErrorContext(InSelf));
}

PyObject* FPyWrapperObject::CallFunction_Impl(UClass* InClass, UObject* InObj, const FName InFuncName, const TCHAR* InErrorCtxt)
{
	if (InClass && InObj)
	{
		check(InObj->IsA(InClass) || InObj->GetClass()->ImplementsInterface(InClass));

		// Find the function from the class rather than the object, as we may be specifically calling a super class function
		UFunction* Func = InClass->FindFunctionByName(InFuncName);
		if (ensureAlways(Func))
		{
			if (Func->Children == nullptr)
			{
				// No return value
				CallFunction_InvokeImpl(InObj, Func, nullptr, InErrorCtxt);
			}
			else
			{
				// Return value requires that we create a params struct to hold the result
				FStructOnScope FuncParams(Func);
				CallFunction_InvokeImpl(InObj, Func, FuncParams.GetStructMemory(), InErrorCtxt);
				return CallFunction_ReturnImpl(InObj, Func, FuncParams.GetStructMemory(), InErrorCtxt);
			}
		}
	}

	Py_RETURN_NONE;
}

PyObject* FPyWrapperObject::CallFunction_Impl(UClass* InClass, UObject* InObj, PyObject* InArgs, PyObject* InKwds, const FName InFuncName, const char* InPythonFuncName, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef, const TCHAR* InErrorCtxt)
{
	TArray<PyObject*> Params;
	if (!PyGenUtil::ParseMethodParameters(InArgs, InKwds, InParamDef, InPythonFuncName, Params))
	{
		return nullptr;
	}

	if (InClass && InObj)
	{
		check(InObj->IsA(InClass) || InObj->GetClass()->ImplementsInterface(InClass));

		// Find the function from the class rather than the object, as we may be specifically calling a super class function
		UFunction* Func = InClass->FindFunctionByName(InFuncName);
		if (ensureAlways(Func))
		{
			FStructOnScope FuncParams(Func);
			CallFunction_ApplyDefaults(Func, FuncParams.GetStructMemory(), InParamDef);
			for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
			{
				const PyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = InParamDef[ParamIndex];

				UProperty* ArgProp = FuncParams.GetStruct()->FindPropertyByName(ParamDef.ParamPropName);
				if (!ArgProp)
				{
					PyUtil::SetPythonError(PyExc_Exception, InErrorCtxt, *FString::Printf(TEXT("Failed to find property '%s' for parameter '%s' when calling function '%s' on '%s'"), *ParamDef.ParamPropName.ToString(), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *Func->GetName(), *InObj->GetName()));
					return nullptr;
				}

				PyObject* PyValue = Params[ParamIndex];
				if (PyValue)
				{
					if (!PyConversion::NativizeProperty_InContainer(PyValue, ArgProp, FuncParams.GetStructMemory(), 0))
					{
						PyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Failed to convert parameter '%s' when calling function '%s' on '%s'"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *Func->GetName(), *InObj->GetName()));
						return nullptr;
					}
				}
			}
			CallFunction_InvokeImpl(InObj, Func, FuncParams.GetStructMemory(), InErrorCtxt);
			return CallFunction_ReturnImpl(InObj, Func, FuncParams.GetStructMemory(), InErrorCtxt);
		}
	}

	Py_RETURN_NONE;
}

void FPyWrapperObject::CallFunction_ApplyDefaults(UFunction* InFunc, void* InBaseParamsAddr, const TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& InParamDef)
{
	for (const PyGenUtil::FGeneratedWrappedMethodParameter& ParamDef : InParamDef)
	{
		UProperty* ArgProp = InFunc->FindPropertyByName(ParamDef.ParamPropName);
		if (ArgProp && ParamDef.ParamDefaultValue.IsSet())
		{
			ArgProp->ImportText(*ParamDef.ParamDefaultValue.GetValue(), ArgProp->ContainerPtrToValuePtr<void>(InBaseParamsAddr), PPF_None, nullptr);
		}
	}
}

void FPyWrapperObject::CallFunction_InvokeImpl(UObject* InObj, UFunction* InFunc, void* InBaseParamsAddr, const TCHAR* InErrorCtxt)
{
	FEditorScriptExecutionGuard ScriptGuard;
	InObj->ProcessEvent(InFunc, InBaseParamsAddr);
}

PyObject* FPyWrapperObject::CallFunction_ReturnImpl(UObject* InObj, UFunction* InFunc, const void* InBaseParamsAddr, const TCHAR* InErrorCtxt)
{
	TArray<const UProperty*, TInlineAllocator<4>> OutParams;

	// Add the return property first
	if (const UProperty* ReturnProp = InFunc->GetReturnProperty())
	{
		OutParams.Add(ReturnProp);
	}

	// Add any other output params in order
	for (TFieldIterator<const UProperty> ParamIt(InFunc); ParamIt; ++ParamIt)
	{
		const UProperty* Param = *ParamIt;
		if (PyUtil::IsOutputParameter(Param))
		{
			OutParams.Add(Param);
		}
	}

	return PyUtil::PackReturnValues(InFunc, OutParams, InBaseParamsAddr, InErrorCtxt, *FString::Printf(TEXT("function '%s' on '%s'"), *InFunc->GetName(), *InObj->GetName()));
}

PyObject* FPyWrapperObject::CallClassMethodNoArgs_Impl(PyTypeObject* InType, void* InClosure)
{
	const PyGenUtil::FGeneratedWrappedMethod* Closure = (PyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(Closure->Class, InType, Closure->MethodFuncName);
}

PyObject* FPyWrapperObject::CallClassMethodWithArgs_Impl(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, void* InClosure)
{
	const PyGenUtil::FGeneratedWrappedMethod* Closure = (PyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(Closure->Class, InType, InArgs, InKwds, Closure->MethodFuncName, Closure->MethodName.GetData(), Closure->MethodParams);
}

PyObject* FPyWrapperObject::CallMethodNoArgs_Impl(FPyWrapperObject* InSelf, void* InClosure)
{
	const PyGenUtil::FGeneratedWrappedMethod* Closure = (PyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(Closure->Class, InSelf, Closure->MethodFuncName);
}

PyObject* FPyWrapperObject::CallMethodWithArgs_Impl(FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure)
{
	const PyGenUtil::FGeneratedWrappedMethod* Closure = (PyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(Closure->Class, InSelf, InArgs, InKwds, Closure->MethodFuncName, Closure->MethodName.GetData(), Closure->MethodParams);
}

PyObject* FPyWrapperObject::Getter_Impl(FPyWrapperObject* InSelf, void* InClosure)
{
	const PyGenUtil::FGeneratedWrappedGetSet* Closure = (PyGenUtil::FGeneratedWrappedGetSet*)InClosure;
	return Closure->GetFuncName.IsNone()
		? GetPropertyValue(InSelf, Closure->PropName, Closure->GetSetName.GetData())
		: CallGetterFunction(Closure->Class, InSelf, Closure->GetFuncName);
}

int FPyWrapperObject::Setter_Impl(FPyWrapperObject* InSelf, PyObject* InValue, void* InClosure)
{
	const PyGenUtil::FGeneratedWrappedGetSet* Closure = (PyGenUtil::FGeneratedWrappedGetSet*)InClosure;
	return Closure->SetFuncName.IsNone()
		? SetPropertyValue(InSelf, InValue, Closure->PropName, Closure->GetSetName.GetData())
		: CallSetterFunction(Closure->Class, InSelf, InValue, Closure->SetFuncName);
}

PyTypeObject InitializePyWrapperObjectType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyWrapperObject::New(InType);
		}

		static void Dealloc(FPyWrapperObject* InSelf)
		{
			FPyWrapperObject::Free(InSelf);
		}

		static int Init(FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			UObject* InitValue = nullptr;

			UObject* ObjectOuter = GetTransientPackage();
			FName ObjectName;

			// Parse the args
			{
				PyObject* PyOuterObj = nullptr;
				PyObject* PyNameObj = nullptr;

				static const char *ArgsKwdList[] = { "outer", "name", nullptr };
				if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:call", (char**)ArgsKwdList, &PyOuterObj, &PyNameObj))
				{
					return -1;
				}

				if (PyOuterObj && !PyConversion::Nativize(PyOuterObj, ObjectOuter))
				{
					PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *PyUtil::GetFriendlyTypename(PyOuterObj)));
					return -1;
				}

				if (PyNameObj && !PyConversion::Nativize(PyNameObj, ObjectName))
				{
					PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *PyUtil::GetFriendlyTypename(PyNameObj)));
					return -1;
				}
			}

			UClass* ObjClass = FPyWrapperObjectMetaData::GetClass(InSelf);
			if (ObjClass)
			{
				if (ObjClass->HasAnyClassFlags(CLASS_Abstract))
				{
					PyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Class '%s' is abstract"), *ObjClass->GetName()));
					return -1;
				}
				else
				{
					InitValue = NewObject<UObject>(ObjectOuter, ObjClass, ObjectName);
				}
			}
			else
			{
				PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Class is null"));
				return -1;
			}

			// Do we have an object instance to wrap?
			if (!InitValue)
			{
				PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Object instance was null during init"));
				return -1;
			}

			return FPyWrapperObject::Init(InSelf, InitValue);
		}

		static PyObject* Str(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyUnicode_FromFormat("<Object '%s' (%p) Class '%s'>", TCHAR_TO_UTF8(*InSelf->ObjectInstance->GetPathName()), InSelf->ObjectInstance, TCHAR_TO_UTF8(*InSelf->ObjectInstance->GetClass()->GetName()));
		}

		static PyUtil::FPyHashType Hash(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return -1;
			}

			const PyUtil::FPyHashType PyHash = (PyUtil::FPyHashType)GetTypeHash(InSelf->ObjectInstance);
			return PyHash != -1 ? PyHash : 0;
		}
	};

	struct FMethods
	{
		static PyObject* PostInit(FPyWrapperObject* InSelf)
		{
			Py_RETURN_NONE;
		}

		static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
		{
			PyObject* PyObj = nullptr;
			if (PyArg_ParseTuple(InArgs, "O:cast", &PyObj))
			{
				PyObject* PyCastResult = (PyObject*)FPyWrapperObject::CastPyObject(PyObj, InType);
				if (!PyCastResult)
				{
					PyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *PyUtil::GetFriendlyTypename(PyObj), *PyUtil::GetFriendlyTypename(InType)));
				}
				return PyCastResult;
			}

			return nullptr;
		}

		static PyObject* StaticClass(PyTypeObject* InType)
		{
			UClass* Class = FPyWrapperObjectMetaData::GetClass(InType);
			return PyConversion::PythonizeClass(Class);
		}

		static PyObject* GetClass(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::PythonizeClass(InSelf->ObjectInstance->GetClass());
		}

		static PyObject* GetOuter(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetOuter());
		}

		static PyObject* GetTypedOuter(FPyWrapperObject* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyOuterType = nullptr;
			if (!PyArg_ParseTuple(InArgs, "O:get_typed_outer", &PyOuterType))
			{
				return nullptr;
			}

			UClass* OuterType = nullptr;
			if (!PyConversion::NativizeClass(PyOuterType, OuterType, UObject::StaticClass()))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetTypedOuter(OuterType));
		}

		static PyObject* GetOutermost(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetOutermost());
		}

		static PyObject* GetName(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetName());
		}

		static PyObject* GetFName(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetFName());
		}

		static PyObject* GetFullName(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetFullName());
		}

		static PyObject* GetPathName(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetPathName());
		}

		static PyObject* GetWorld(FPyWrapperObject* InSelf)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyConversion::Pythonize(InSelf->ObjectInstance->GetWorld());
		}

		static PyObject* Modify(FPyWrapperObject* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyAlwaysMarkDirty = nullptr;
			if (!PyArg_ParseTuple(InArgs, "|O:modify", &PyAlwaysMarkDirty))
			{
				return nullptr;
			}

			bool bAlwaysMarkDirty = true;
			if (PyAlwaysMarkDirty && !PyConversion::Nativize(PyAlwaysMarkDirty, bAlwaysMarkDirty))
			{
				return nullptr;
			}

			const bool bResult = InSelf->ObjectInstance->Modify(bAlwaysMarkDirty);
			return PyConversion::Pythonize(bResult);
		}

		static PyObject* Rename(FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (!FPyWrapperObject::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyNameObj = nullptr;
			PyObject* PyOuterObj = nullptr;

			static const char *ArgsKwdList[] = { "name", "outer", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:rename", (char**)ArgsKwdList, &PyNameObj, &PyOuterObj))
			{
				return nullptr;
			}

			FName NewName;
			if (PyNameObj && PyNameObj != Py_None && !PyConversion::Nativize(PyNameObj, NewName))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *PyUtil::GetFriendlyTypename(InSelf)));
				return nullptr;
			}

			UObject* NewOuter = nullptr;
			if (PyOuterObj && !PyConversion::Nativize(PyOuterObj, NewOuter))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *PyUtil::GetFriendlyTypename(PyOuterObj)));
				return nullptr;
			}

			const bool bResult = InSelf->ObjectInstance->Rename(NewName.IsNone() ? nullptr : *NewName.ToString(), NewOuter);

			return PyConversion::Pythonize(bResult);
		}

		static PyObject* GetEditorProperty(FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyNameObj = nullptr;

			static const char *ArgsKwdList[] = { "name", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:get_editor_property", (char**)ArgsKwdList, &PyNameObj))
			{
				return nullptr;
			}

			FName Name;
			if (!PyConversion::Nativize(PyNameObj, Name))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *PyUtil::GetFriendlyTypename(InSelf)));
				return nullptr;
			}

			const FName ResolvedName = FPyWrapperObjectMetaData::ResolvePropertyName(InSelf, Name);
			return FPyWrapperObject::GetPropertyValue(InSelf, ResolvedName, TCHAR_TO_UTF8(*Name.ToString()));
		}

		static PyObject* SetEditorProperty(FPyWrapperObject* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyNameObj = nullptr;
			PyObject* PyValueObj = nullptr;

			static const char *ArgsKwdList[] = { "name", "value", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:set_editor_property", (char**)ArgsKwdList, &PyNameObj, &PyValueObj))
			{
				return nullptr;
			}

			FName Name;
			if (!PyConversion::Nativize(PyNameObj, Name))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *PyUtil::GetFriendlyTypename(InSelf)));
				return nullptr;
			}

			const FName ResolvedName = FPyWrapperObjectMetaData::ResolvePropertyName(InSelf, Name);
			const int Result = FPyWrapperObject::SetPropertyValue(InSelf, PyValueObj, ResolvedName, TCHAR_TO_UTF8(*Name.ToString()), /*InNotifyChange*/true, CPF_EditConst);
			if (Result != 0)
			{
				return nullptr;
			}

			Py_RETURN_NONE;
		}
	};

	static PyMethodDef PyMethods[] = {
		{ PyGenUtil::PostInitFuncName, PyCFunctionCast(&FMethods::PostInit), METH_NOARGS, "x._post_init() -- called during Unreal object initialization (equivalent to PostInitProperties in C++)" },
		{ "cast", PyCFunctionCast(&FMethods::Cast), METH_VARARGS | METH_CLASS, "X.cast(object) -> UObject -- cast the given object to this Unreal object type" },
		{ "static_class", PyCFunctionCast(&FMethods::StaticClass), METH_NOARGS | METH_CLASS, "X.static_class() -> UClass -- get the Unreal class of this type" },
		{ "get_class", PyCFunctionCast(&FMethods::GetClass), METH_NOARGS, "x.get_class() -> UClass -- get the Unreal class of this instance" },
		{ "get_outer", PyCFunctionCast(&FMethods::GetOuter), METH_NOARGS, "x.get_outer() -> UObject -- get the outer object from this instance (if any)" },
		{ "get_typed_outer", PyCFunctionCast(&FMethods::GetTypedOuter), METH_VARARGS, "x.get_typed_outer(type) -> type() -- get the first outer object of the given type from this instance (if any)" },
		{ "get_outermost", PyCFunctionCast(&FMethods::GetOutermost), METH_NOARGS, "x.get_outermost() -> UPackage -- get the outermost object (the package) from this instance" },
		{ "get_name", PyCFunctionCast(&FMethods::GetName), METH_NOARGS, "x.get_name() -> str -- get the name of this instance" },
		{ "get_fname", PyCFunctionCast(&FMethods::GetFName), METH_NOARGS, "x.get_fname() -> FName -- get the name of this instance" },
		{ "get_full_name", PyCFunctionCast(&FMethods::GetFullName), METH_NOARGS, "x.get_full_name() -> str -- get the full name (class name + full path) of this instance" },
		{ "get_path_name", PyCFunctionCast(&FMethods::GetPathName), METH_NOARGS, "x.get_path_name() -> str -- get the path name of this instance" },
		{ "get_world", PyCFunctionCast(&FMethods::GetWorld), METH_NOARGS, "x.get_world() -> UWorld -- get the world associated with this instance (if any)" },
		{ "modify", PyCFunctionCast(&FMethods::Modify), METH_VARARGS, "x.modify(bool) -> bool -- inform that this instance is about to be modified (tracks changes for undo/redo if transactional)" },
		{ "rename", PyCFunctionCast(&FMethods::Rename), METH_VARARGS | METH_KEYWORDS, "x.rename(name=None, outer=None) -> bool -- rename this instance" },
		{ "get_editor_property", PyCFunctionCast(&FMethods::GetEditorProperty), METH_VARARGS | METH_KEYWORDS, "x.get_editor_property(name) -> object -- get the value of any property visible to the editor" },
		{ "set_editor_property", PyCFunctionCast(&FMethods::SetEditorProperty), METH_VARARGS | METH_KEYWORDS, "x.set_editor_property(name, value) -- set the value of any property visible to the editor, ensuring that the pre/post change notifications are called" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"_ObjectBase", /* tp_name */
		sizeof(FPyWrapperObject), /* tp_basicsize */
	};

	PyType.tp_base = &PyWrapperBaseType;
	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;
	PyType.tp_str = (reprfunc)&FFuncs::Str;
	PyType.tp_hash = (hashfunc)&FFuncs::Hash;

	PyType.tp_methods = PyMethods;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType.tp_doc = "Type for all UE4 exposed object instances";

	return PyType;
}

PyTypeObject PyWrapperObjectType = InitializePyWrapperObjectType();

FPyWrapperObjectMetaData::FPyWrapperObjectMetaData()
	: Class(nullptr)
{
	AddReferencedObjects = [](FPyWrapperBase* Self, FReferenceCollector& Collector)
	{
		FPyWrapperObject* ObjSelf = static_cast<FPyWrapperObject*>(Self);

		UObject* OldInstance = ObjSelf->ObjectInstance;
		Collector.AddReferencedObject(ObjSelf->ObjectInstance);
		if (ObjSelf->ObjectInstance != OldInstance && ObjSelf->ObjectInstance != nullptr)
		{
			// Object instance has been re-pointed, make sure we're still the correct type
			PyTypeObject* NewPyType = FPyWrapperTypeRegistry::Get().GetWrappedClassType(ObjSelf->ObjectInstance->GetClass());
			if (PyType_IsSubtype(NewPyType, &PyWrapperObjectType) && NewPyType->tp_basicsize == Py_TYPE(ObjSelf)->tp_basicsize)
			{
				Py_TYPE(ObjSelf) = NewPyType; // todo: is this safe?
			}
			else
			{
				ObjSelf->ObjectInstance = nullptr;
			}
		}
	};
}

UClass* FPyWrapperObjectMetaData::GetClass(PyTypeObject* PyType)
{
	FPyWrapperObjectMetaData* PyWrapperMetaData = FPyWrapperObjectMetaData::GetMetaData(PyType);
	return PyWrapperMetaData ? PyWrapperMetaData->Class : nullptr;
}

UClass* FPyWrapperObjectMetaData::GetClass(FPyWrapperObject* Instance)
{
	return GetClass(Py_TYPE(Instance));
}

FName FPyWrapperObjectMetaData::ResolvePropertyName(PyTypeObject* PyType, const FName InPythonPropertyName)
{
	if (FPyWrapperObjectMetaData* PyWrapperMetaData = FPyWrapperObjectMetaData::GetMetaData(PyType))
	{
		if (const FName* MappedPropName = PyWrapperMetaData->PythonProperties.Find(InPythonPropertyName))
		{
			return *MappedPropName;
		}

		if (const UClass* SuperClass = PyWrapperMetaData->Class ? PyWrapperMetaData->Class->GetSuperClass() : nullptr)
		{
			PyTypeObject* SuperClassPyType = FPyWrapperTypeRegistry::Get().GetWrappedClassType(SuperClass);
			return ResolvePropertyName(SuperClassPyType, InPythonPropertyName);
		}
	}

	return InPythonPropertyName;
}

FName FPyWrapperObjectMetaData::ResolvePropertyName(FPyWrapperObject* Instance, const FName InPythonPropertyName)
{
	return ResolvePropertyName(Py_TYPE(Instance), InPythonPropertyName);
}

FName FPyWrapperObjectMetaData::ResolveFunctionName(PyTypeObject* PyType, const FName InPythonMethodName)
{
	if (FPyWrapperObjectMetaData* PyWrapperMetaData = FPyWrapperObjectMetaData::GetMetaData(PyType))
	{
		if (const FName* MappedFuncName = PyWrapperMetaData->PythonMethods.Find(InPythonMethodName))
		{
			return *MappedFuncName;
		}

		if (const UClass* SuperClass = PyWrapperMetaData->Class ? PyWrapperMetaData->Class->GetSuperClass() : nullptr)
		{
			PyTypeObject* SuperClassPyType = FPyWrapperTypeRegistry::Get().GetWrappedClassType(SuperClass);
			return ResolveFunctionName(SuperClassPyType, InPythonMethodName);
		}
	}

	return InPythonMethodName;
}

FName FPyWrapperObjectMetaData::ResolveFunctionName(FPyWrapperObject* Instance, const FName InPythonMethodName)
{
	return ResolveFunctionName(Py_TYPE(Instance), InPythonMethodName);
}

struct FPythonGeneratedClassUtil
{
	static void PrepareOldClassForReinstancing(UPythonGeneratedClass* InOldClass)
	{
		const FString OldClassName = MakeUniqueObjectName(InOldClass->GetOuter(), InOldClass->GetClass(), *FString::Printf(TEXT("%s_REINST"), *InOldClass->GetName())).ToString();
		InOldClass->ClassFlags |= CLASS_NewerVersionExists;
		InOldClass->SetFlags(RF_NewerVersionExists);
		InOldClass->ClearFlags(RF_Public | RF_Standalone);
		InOldClass->Rename(*OldClassName, nullptr, REN_DontCreateRedirectors);
	}

	static UPythonGeneratedClass* CreateClass(const FString& InClassName, UObject* InClassOuter, UClass* InSuperClass)
	{
		UPythonGeneratedClass* Class = NewObject<UPythonGeneratedClass>(InClassOuter, *InClassName, RF_Public | RF_Standalone);
		Class->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
		Class->SetSuperStruct(InSuperClass);
		return Class;
	}

	static void FinalizeClass(UPythonGeneratedClass* InClass, PyTypeObject* InPyType)
	{
		// Finalize the class
		InClass->Bind();
		InClass->StaticLink(true);
		InClass->AssembleReferenceTokenStream();

		// Add the object meta-data to the type
		InClass->PyMetaData.Class = InClass;
		FPyWrapperObjectMetaData::SetMetaData(InPyType, &InClass->PyMetaData);

		// Map the Unreal class to the Python type
		InClass->PyType = FPyTypeObjectPtr::NewReference(InPyType);
		FPyWrapperTypeRegistry::Get().RegisterWrappedClassType(InClass->GetFName(), InPyType);
	}

	static bool CreatePropertyFromDefinition(UPythonGeneratedClass* InClass, PyTypeObject* InPyType, const FString& InFieldName, FPyUPropertyDef* InPyPropDef)
	{
		UClass* SuperClass = InClass->GetSuperClass();

		// Resolve the property name to match any previously exported properties from the parent type
		const FName PropName = FPyWrapperObjectMetaData::ResolvePropertyName(InPyType->tp_base, *InFieldName);
		if (SuperClass->FindPropertyByName(PropName))
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Property '%s' (%s) cannot override a property from the base type"), *InFieldName, *PyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Create the property from its definition
		UProperty* Prop = PyUtil::CreateProperty(InPyPropDef->PropType, 1, InClass, PropName);
		if (!Prop)
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to create property for '%s' (%s)"), *InFieldName, *PyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}
		Prop->PropertyFlags |= (CPF_Edit | CPF_BlueprintVisible);
		FPyUPropertyDef::ApplyMetaData(InPyPropDef, Prop);
		InClass->AddCppProperty(Prop);

		// Resolve any getter/setter function names
		const FName GetterFuncName = FPyWrapperObjectMetaData::ResolveFunctionName(InPyType->tp_base, *InPyPropDef->GetterFuncName);
		const FName SetterFuncName = FPyWrapperObjectMetaData::ResolveFunctionName(InPyType->tp_base, *InPyPropDef->SetterFuncName);
		if (!GetterFuncName.IsNone())
		{
			Prop->SetMetaData(PyGenUtil::BlueprintGetterMetaDataKey, *GetterFuncName.ToString());
		}
		if (!SetterFuncName.IsNone())
		{
			Prop->SetMetaData(PyGenUtil::BlueprintSetterMetaDataKey, *SetterFuncName.ToString());
		}

		// Build the definition data for the new property accessor
		PyGenUtil::FPropertyDef& PropDef = *InClass->PropertyDefs[InClass->PropertyDefs.Add(MakeShared<PyGenUtil::FPropertyDef>())];
		PropDef.GeneratedWrappedGetSet.Class = InClass;
		PropDef.GeneratedWrappedGetSet.GetSetName = PyGenUtil::TCHARToUTF8Buffer(*InFieldName);
		PropDef.GeneratedWrappedGetSet.GetSetDoc = PyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("type: %s\n%s"), *PyGenUtil::GetPropertyPythonType(Prop), *PyGenUtil::GetFieldTooltip(Prop)));
		PropDef.GeneratedWrappedGetSet.PropName = Prop->GetFName();
		PropDef.GeneratedWrappedGetSet.GetFuncName = GetterFuncName;
		PropDef.GeneratedWrappedGetSet.SetFuncName = SetterFuncName;
		PropDef.GeneratedWrappedGetSet.GetCallback = (getter)&FPyWrapperObject::Getter_Impl;
		PropDef.GeneratedWrappedGetSet.SetCallback = (setter)&FPyWrapperObject::Setter_Impl;
		PropDef.GeneratedWrappedGetSet.ToPython(PropDef.PyGetSet);

		// If this property has a getter or setter, also make an internal version with the get/set function cleared so that Python can read/write the internal property value
		if (!GetterFuncName.IsNone() || !SetterFuncName.IsNone())
		{
			PyGenUtil::FPropertyDef& InternalPropDef = *InClass->PropertyDefs[InClass->PropertyDefs.Add(MakeShared<PyGenUtil::FPropertyDef>())];
			InternalPropDef.GeneratedWrappedGetSet.Class = InClass;
			InternalPropDef.GeneratedWrappedGetSet.GetSetName = PyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("_%s"), *InFieldName));
			InternalPropDef.GeneratedWrappedGetSet.GetSetDoc = PropDef.GeneratedWrappedGetSet.GetSetDoc;
			InternalPropDef.GeneratedWrappedGetSet.PropName = Prop->GetFName();
			InternalPropDef.GeneratedWrappedGetSet.GetCallback = (getter)&FPyWrapperObject::Getter_Impl;
			InternalPropDef.GeneratedWrappedGetSet.SetCallback = (setter)&FPyWrapperObject::Setter_Impl;
			InternalPropDef.GeneratedWrappedGetSet.ToPython(InternalPropDef.PyGetSet);
		}

		return true;
	}

	static bool CreateFunctionFromDefinition(UPythonGeneratedClass* InClass, PyTypeObject* InPyType, const FString& InFieldName, FPyUFunctionDef* InPyFuncDef)
	{
		UClass* SuperClass = InClass->GetSuperClass();

		// Validate the function definition makes sense
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Override))
		{
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Static | EPyUFunctionDefFlags::Getter | EPyUFunctionDefFlags::Setter))
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' specified as 'override' cannot also specify 'static', 'getter', or 'setter'"), *InFieldName));
				return false;
			}
			if (InPyFuncDef->FuncRetType != Py_None || InPyFuncDef->FuncParamTypes != Py_None)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' specified as 'override' cannot also specify 'ret' or 'params'"), *InFieldName));
				return false;
			}
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Static) && EnumHasAnyFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Getter | EPyUFunctionDefFlags::Setter))
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' specified as 'static' cannot also specify 'getter' or 'setter'"), *InFieldName));
			return false;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Getter))
		{
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Setter))
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' specified as 'getter' cannot also specify 'setter'"), *InFieldName));
				return false;
			}
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Impure))
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' specified as 'getter' must also specify 'pure=True'"), *InFieldName));
				return false;
			}
		}

		// Resolve the function name to match any previously exported functions from the parent type
		const FName FuncName = FPyWrapperObjectMetaData::ResolveFunctionName(InPyType->tp_base, *InFieldName);
		const UFunction* SuperFunc = SuperClass->FindFunctionByName(FuncName);
		if (SuperFunc && !EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Override))
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' cannot override a method from the base type (did you forget to specify 'override=True'?)"), *InFieldName));
			return false;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Override))
		{
			if (!SuperFunc)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' was set to 'override', but no method was found to override"), *InFieldName));
				return false;
			}
			if (!SuperFunc->HasAnyFunctionFlags(FUNC_BlueprintEvent))
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Method '%s' was set to 'override', but the method found to override was not a blueprint event"), *InFieldName));
				return false;
			}
		}

		// Inspect the argument names and defaults from the Python function
		TArray<FString> FuncArgNames;
		TArray<FPyObjectPtr> FuncArgDefaults;
		if (!PyUtil::InspectFunctionArgs(InPyFuncDef->Func, FuncArgNames, &FuncArgDefaults))
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to inspect the arguments for '%s'"), *InFieldName));
			return false;
		}

		// Create the function, either from the definition, or from the super-function found to override
		InClass->AddNativeFunction(*FuncName.ToString(), &UPythonGeneratedClass::CallPythonFunction); // Need to do this before the call to DuplicateObject in the case that the super-function already has FUNC_Native
		UFunction* Func = SuperFunc ? DuplicateObject<UFunction>(SuperFunc, InClass, FuncName) : NewObject<UFunction>(InClass, FuncName);
		if (!SuperFunc)
		{
			Func->FunctionFlags |= FUNC_Public;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Static))
		{
			Func->FunctionFlags |= FUNC_Static;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Pure))
		{
			Func->FunctionFlags |= FUNC_BlueprintPure;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Impure))
		{
			Func->FunctionFlags &= ~FUNC_BlueprintPure;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Getter))
		{
			Func->SetMetaData(PyGenUtil::BlueprintGetterMetaDataKey, TEXT(""));
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Setter))
		{
			Func->SetMetaData(PyGenUtil::BlueprintSetterMetaDataKey, TEXT(""));
		}
		Func->FunctionFlags |= (FUNC_Native | FUNC_Event | FUNC_BlueprintEvent | FUNC_BlueprintCallable);
		FPyUFunctionDef::ApplyMetaData(InPyFuncDef, Func);
		InClass->AddFunctionToFunctionMap(Func, Func->GetFName());
		if (!Func->HasAnyFunctionFlags(FUNC_Static))
		{
			// Strip the zero'th 'self' argument when processing a non-static function
			FuncArgNames.RemoveAt(0, 1, /*bAllowShrinking*/false);
			FuncArgDefaults.RemoveAt(0, 1, /*bAllowShrinking*/false);
		}
		if (!SuperFunc)
		{
			// Make sure the number of function arguments matches the number of argument types specified
			const int32 NumArgTypes = (InPyFuncDef->FuncParamTypes && InPyFuncDef->FuncParamTypes != Py_None) ? PySequence_Size(InPyFuncDef->FuncParamTypes) : 0;
			if (NumArgTypes != FuncArgNames.Num())
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Incorrect number of arguments specified for '%s' (expected %d, got %d)"), *InFieldName, NumArgTypes, FuncArgNames.Num()));
				return false;
			}

			// Build the arguments struct if not overriding a function
			if (InPyFuncDef->FuncRetType && InPyFuncDef->FuncRetType != Py_None)
			{
				// If we have a tuple, then we actually want to return a bool but add every type within the tuple as output parameters
				const bool bOptionalReturn = PyTuple_Check(InPyFuncDef->FuncRetType);

				PyObject* RetType = bOptionalReturn ? (PyObject*)&PyBool_Type : InPyFuncDef->FuncRetType;
				UProperty* RetProp = PyUtil::CreateProperty(RetType, 1, Func, TEXT("ReturnValue"));
				if (!RetProp)
				{
					PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to create return property (%s) for function '%s'"), *PyUtil::GetFriendlyTypename(RetType), *InFieldName));
					return false;
				}
				RetProp->PropertyFlags |= (CPF_Parm | CPF_ReturnParm);
				Func->AddCppProperty(RetProp);

				if (bOptionalReturn)
				{
					const int32 NumOutArgs = PyTuple_Size(InPyFuncDef->FuncRetType);
					for (int32 ArgIndex = 0; ArgIndex < NumOutArgs; ++ArgIndex)
					{
						PyObject* ArgTypeObj = PySequence_GetItem(InPyFuncDef->FuncRetType, ArgIndex);
						UProperty* ArgProp = PyUtil::CreateProperty(ArgTypeObj, 1, Func, *FString::Printf(TEXT("OutValue%d"), ArgIndex));
						if (!ArgProp)
						{
							PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to create output property (%s) for function '%s' at index %d"), *PyUtil::GetFriendlyTypename(ArgTypeObj), *InFieldName, ArgIndex));
							return false;
						}
						ArgProp->PropertyFlags |= (CPF_Parm | CPF_OutParm);
						Func->AddCppProperty(ArgProp);
						Func->FunctionFlags |= FUNC_HasOutParms;
					}
				}
			}
			for (int32 ArgIndex = 0; ArgIndex < FuncArgNames.Num(); ++ArgIndex)
			{
				PyObject* ArgTypeObj = PySequence_GetItem(InPyFuncDef->FuncParamTypes, ArgIndex);
				UProperty* ArgProp = PyUtil::CreateProperty(ArgTypeObj, 1, Func, *FuncArgNames[ArgIndex]);
				if (!ArgProp)
				{
					PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to create property (%s) for function '%s' argument '%s'"), *PyUtil::GetFriendlyTypename(ArgTypeObj), *InFieldName, *FuncArgNames[ArgIndex]));
					return false;
				}
				ArgProp->PropertyFlags |= CPF_Parm;
				Func->AddCppProperty(ArgProp);
			}
		}
		// Apply the defaults to the function arguments and build the Python method params
		TArray<PyGenUtil::FGeneratedWrappedMethodParameter> MethodParams;
		{
			int32 InputArgIndex = 0;
			for (TFieldIterator<const UProperty> ParamIt(Func); ParamIt; ++ParamIt)
			{
				const UProperty* Param = *ParamIt;

				if (PyUtil::IsInputParameter(Param))
				{
					const FName DefaultValueMetaDataKey = *FString::Printf(TEXT("CPP_Default_%s"), *Param->GetName());

					TOptional<FString> ResolvedDefaultValue;
					if (FuncArgDefaults.IsValidIndex(InputArgIndex) && FuncArgDefaults[InputArgIndex])
					{
						// Convert the default value to the given property...
						PyUtil::FPropValueOnScope DefaultValue(Param);
						if (!DefaultValue.IsValid() || !DefaultValue.SetValue(FuncArgDefaults[InputArgIndex], *PyUtil::GetErrorContext(InPyType)))
						{
							PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to convert default value for function '%s' argument '%s' (%s)"), *InFieldName, *FuncArgNames[InputArgIndex], *Param->GetClass()->GetName()));
							return false;
						}

						// ... and export it as meta-data
						FString ExportedDefaultValue;
						if (!DefaultValue.GetProp()->ExportText_Direct(ExportedDefaultValue, DefaultValue.GetValue(), DefaultValue.GetValue(), nullptr, PPF_None))
						{
							PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to export default value for function '%s' argument '%s' (%s)"), *InFieldName, *FuncArgNames[InputArgIndex], *Param->GetClass()->GetName()));
							return false;
						}

						ResolvedDefaultValue = ExportedDefaultValue;
					}
					if (!ResolvedDefaultValue.IsSet() && SuperFunc && SuperFunc->HasAnyFunctionFlags(FUNC_HasDefaults))
					{
						if (SuperFunc->HasMetaData(DefaultValueMetaDataKey))
						{
							ResolvedDefaultValue = SuperFunc->GetMetaData(DefaultValueMetaDataKey);
						}
					}
					if (ResolvedDefaultValue.IsSet())
					{
						Func->SetMetaData(DefaultValueMetaDataKey, *ResolvedDefaultValue.GetValue());
						Func->FunctionFlags |= FUNC_HasDefaults;
					}

					PyGenUtil::FGeneratedWrappedMethodParameter& GeneratedWrappedMethodParam = MethodParams[MethodParams.AddDefaulted()];
					GeneratedWrappedMethodParam.ParamName = PyGenUtil::TCHARToUTF8Buffer(FuncArgNames.IsValidIndex(InputArgIndex) ? *FuncArgNames[InputArgIndex] : *Param->GetName());
					GeneratedWrappedMethodParam.ParamPropName = Param->GetFName();
					GeneratedWrappedMethodParam.ParamDefaultValue = MoveTemp(ResolvedDefaultValue);

					++InputArgIndex;
				}
			}
		}
		Func->Bind();
		Func->StaticLink(true);

		if (MethodParams.Num() != FuncArgNames.Num())
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Incorrect number of arguments specified for '%s' (expected %d, got %d)"), *InFieldName, MethodParams.Num(), FuncArgNames.Num()));
			return false;
		}

		// Apply the doc string as the function tooltip
		{
			static const FName ToolTipKey = TEXT("ToolTip");

			const FString DocString = PyUtil::GetDocString(InPyFuncDef->Func);
			if (!DocString.IsEmpty())
			{
				Func->SetMetaData(ToolTipKey, *DocString);
			}
		}

		// Build the definition data for the new method
		PyGenUtil::FFunctionDef& FuncDef = *InClass->FunctionDefs[InClass->FunctionDefs.Add(MakeShared<PyGenUtil::FFunctionDef>())];
		FuncDef.GeneratedWrappedMethod.Class = InClass;
		FuncDef.GeneratedWrappedMethod.MethodName = PyGenUtil::TCHARToUTF8Buffer(*InFieldName);
		FuncDef.GeneratedWrappedMethod.MethodDoc = PyGenUtil::TCHARToUTF8Buffer(*PyGenUtil::GetFieldTooltip(Func));
		FuncDef.GeneratedWrappedMethod.MethodFuncName = Func->GetFName();
		FuncDef.GeneratedWrappedMethod.MethodFlags = FuncArgNames.Num() > 0 ? METH_VARARGS | METH_KEYWORDS : METH_NOARGS;
		if (Func->HasAnyFunctionFlags(FUNC_Static))
		{
			FuncDef.GeneratedWrappedMethod.MethodFlags |= METH_CLASS;
			FuncDef.GeneratedWrappedMethod.MethodCallback = FuncArgNames.Num() > 0 ? PyCFunctionWithClosureCast(&FPyWrapperObject::CallClassMethodWithArgs_Impl) : PyCFunctionWithClosureCast(&FPyWrapperObject::CallClassMethodNoArgs_Impl);
		}
		else
		{
			FuncDef.GeneratedWrappedMethod.MethodCallback = FuncArgNames.Num() > 0 ? PyCFunctionWithClosureCast(&FPyWrapperObject::CallMethodWithArgs_Impl) : PyCFunctionWithClosureCast(&FPyWrapperObject::CallMethodNoArgs_Impl);
		}
		FuncDef.GeneratedWrappedMethod.MethodParams = MoveTemp(MethodParams);
		FuncDef.GeneratedWrappedMethod.ToPython(FuncDef.PyMethod);
		FuncDef.PyFunction = FPyObjectPtr::NewReference(InPyFuncDef->Func);
		FuncDef.bIsHidden = EnumHasAnyFlags(InPyFuncDef->FuncFlags, EPyUFunctionDefFlags::Getter | EPyUFunctionDefFlags::Setter);

		return true;
	}

	static bool CopyPropertiesFromOldClass(UPythonGeneratedClass* InClass, UPythonGeneratedClass* InOldClass, PyTypeObject* InPyType)
	{
		InClass->PropertyDefs.Reserve(InOldClass->PropertyDefs.Num());
		for (const TSharedPtr<PyGenUtil::FPropertyDef>& OldPropDef : InOldClass->PropertyDefs)
		{
			const FName PropName = OldPropDef->GeneratedWrappedGetSet.PropName;

			UProperty* OldProp = InOldClass->FindPropertyByName(PropName);
			if (!OldProp)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to find property for '%s'"), UTF8_TO_TCHAR(OldPropDef->PyGetSet.name)));
				return false;
			}

			UProperty* Prop = DuplicateObject<UProperty>(OldProp, InClass, PropName);
			if (!Prop)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to duplicate property for '%s'"), UTF8_TO_TCHAR(OldPropDef->PyGetSet.name)));
				return false;
			}

			UMetaData::CopyMetadata(OldProp, Prop);
			InClass->AddCppProperty(Prop);

			PyGenUtil::FPropertyDef& PropDef = *InClass->PropertyDefs[InClass->PropertyDefs.Add(MakeShared<PyGenUtil::FPropertyDef>(*OldPropDef))];
			PropDef.GeneratedWrappedGetSet.Class = InClass;
			PropDef.GeneratedWrappedGetSet.ToPython(PropDef.PyGetSet);
		}

		return true;
	}

	static bool CopyFunctionsFromOldClass(UPythonGeneratedClass* InClass, UPythonGeneratedClass* InOldClass, PyTypeObject* InPyType)
	{
		InClass->FunctionDefs.Reserve(InOldClass->FunctionDefs.Num());
		for (const TSharedPtr<PyGenUtil::FFunctionDef>& OldFuncDef : InOldClass->FunctionDefs)
		{
			const FName FuncName = OldFuncDef->GeneratedWrappedMethod.MethodFuncName;

			UFunction* OldFunc = InOldClass->FindFunctionByName(FuncName);
			if (!OldFunc)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to find function for '%s'"), UTF8_TO_TCHAR(OldFuncDef->PyMethod.MethodName)));
				return false;
			}

			InClass->AddNativeFunction(*FuncName.ToString(), &UPythonGeneratedClass::CallPythonFunction);
			UFunction* Func = DuplicateObject<UFunction>(OldFunc, InClass, FuncName);
			if (!Func)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to duplicate function for '%s'"), UTF8_TO_TCHAR(OldFuncDef->PyMethod.MethodName)));
				return false;
			}

			UMetaData::CopyMetadata(OldFunc, Func);
			InClass->AddFunctionToFunctionMap(Func, Func->GetFName());

			Func->Bind();
			Func->StaticLink(true);

			PyGenUtil::FFunctionDef& FuncDef = *InClass->FunctionDefs[InClass->FunctionDefs.Add(MakeShared<PyGenUtil::FFunctionDef>(*OldFuncDef))];
			FuncDef.GeneratedWrappedMethod.Class = InClass;
			FuncDef.GeneratedWrappedMethod.ToPython(FuncDef.PyMethod);
		}

		return true;
	}

	static bool RegisterDescriptors(UPythonGeneratedClass* InClass, PyTypeObject* InPyType)
	{
		for (const TSharedPtr<PyGenUtil::FPropertyDef>& PropDef : InClass->PropertyDefs)
		{
			FPyObjectPtr GetSetDesc = FPyObjectPtr::StealReference(PyDescr_NewGetSet(InPyType, &PropDef->PyGetSet));
			if (!GetSetDesc)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to create descriptor for '%s'"), UTF8_TO_TCHAR(PropDef->PyGetSet.name)));
				return false;
			}
			if (PyDict_SetItemString(InPyType->tp_dict, PropDef->PyGetSet.name, GetSetDesc) != 0)
			{
				PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to assign descriptor for '%s'"), UTF8_TO_TCHAR(PropDef->PyGetSet.name)));
				return false;
			}
		}

		for (const TSharedPtr<PyGenUtil::FFunctionDef>& FuncDef : InClass->FunctionDefs)
		{
			if (FuncDef->bIsHidden)
			{
				PyDict_DelItemString(InPyType->tp_dict, FuncDef->PyMethod.MethodName);
			}
			else
			{
				FPyObjectPtr MethodDesc = FPyObjectPtr::StealReference(FPyMethodWithClosureDef::NewMethodDescriptor(InPyType, &FuncDef->PyMethod));
				if (!MethodDesc)
				{
					PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to create descriptor for '%s'"), UTF8_TO_TCHAR(FuncDef->PyMethod.MethodName)));
					return false;
				}
				if (PyDict_SetItemString(InPyType->tp_dict, FuncDef->PyMethod.MethodName, MethodDesc) != 0)
				{
					PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to assign descriptor for '%s'"), UTF8_TO_TCHAR(FuncDef->PyMethod.MethodName)));
					return false;
				}
			}
		}

		return true;
	}

	static void ReparentPythonType(PyTypeObject* InPyType, PyTypeObject* InNewBasePyType)
	{
		auto UpdateTuple = [](PyObject* InTuple, PyTypeObject* InOldType, PyTypeObject* InNewType)
		{
			if (InTuple)
			{
				const int32 TupleSize = PyTuple_Size(InTuple);
				for (int32 TupleIndex = 0; TupleIndex < TupleSize; ++TupleIndex)
				{
					if (PyTuple_GetItem(InTuple, TupleIndex) == (PyObject*)InOldType)
					{
						FPyTypeObjectPtr NewType = FPyTypeObjectPtr::NewReference(InNewType);
						PyTuple_SetItem(InTuple, TupleIndex, (PyObject*)NewType.Release()); // PyTuple_SetItem steals the reference
					}
				}
			}
		};

		UpdateTuple(InPyType->tp_bases, InPyType->tp_base, InNewBasePyType);
		UpdateTuple(InPyType->tp_mro, InPyType->tp_base, InNewBasePyType);
		InPyType->tp_base = InNewBasePyType;
	}
};

void UPythonGeneratedClass::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	FPyWrapperTypeRegistry::Get().RegisterWrappedClassType(OldName, nullptr);
	FPyWrapperTypeRegistry::Get().RegisterWrappedClassType(GetFName(), PyType);
}

void UPythonGeneratedClass::PostInitInstance(UObject* InObj)
{
	Super::PostInitInstance(InObj);

	if (PyPostInitFunction)
	{
		FPyObjectPtr PySelf = FPyObjectPtr::StealReference((PyObject*)FPyWrapperObjectFactory::Get().CreateInstance(InObj));
		if (PySelf && ensureAlways(PySelf->ob_type == PyType))
		{
			FPyObjectPtr PyArgs = FPyObjectPtr::StealReference(PyTuple_New(1));
			PyTuple_SetItem(PyArgs, 0, PySelf.Release()); // SetItem steals the reference

			FPyObjectPtr Result = FPyObjectPtr::StealReference(PyObject_CallObject(PyPostInitFunction, PyArgs));
			if (!Result)
			{
				PyUtil::LogPythonError();
			}
		}
	}
}

UPythonGeneratedClass* UPythonGeneratedClass::GenerateClass(PyTypeObject* InPyType)
{
	UObject* ClassOuter = GetPythonTypeContainer();
	const FString ClassName = PyUtil::GetCleanTypename(InPyType);

	// Get the correct super class from the parent type in Python
	UClass* SuperClass = FPyWrapperObjectMetaData::GetClass(InPyType->tp_base);
	if (!SuperClass)
	{
		PyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("No super class could be found for this Python type"));
		return nullptr;
	}

	UPythonGeneratedClass* OldClass = FindObject<UPythonGeneratedClass>(ClassOuter, *ClassName);
	if (OldClass)
	{
		FPythonGeneratedClassUtil::PrepareOldClassForReinstancing(OldClass);
	}

	UPythonGeneratedClass* Class = FPythonGeneratedClassUtil::CreateClass(ClassName, ClassOuter, SuperClass);

	// Get the post-init function
	Class->PyPostInitFunction = FPyObjectPtr::StealReference(PyGenUtil::GetPostInitFunc(InPyType));
	if (!Class->PyPostInitFunction)
	{
		return nullptr;
	}

	// Add the fields to this class
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			const FString FieldName = PyUtil::PyObjectToUEString(FieldKey);

			if (PyObject_IsInstance(FieldValue, (PyObject*)&PyUValueDefType) == 1)
			{
				// Values are not supported on classes
				PyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Classes do not support values"));
				return nullptr;
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&PyUPropertyDefType) == 1)
			{
				FPyUPropertyDef* PyPropDef = (FPyUPropertyDef*)FieldValue;
				if (!FPythonGeneratedClassUtil::CreatePropertyFromDefinition(Class, InPyType, FieldName, PyPropDef))
				{
					return nullptr;
				}
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&PyUFunctionDefType) == 1)
			{
				FPyUFunctionDef* PyFuncDef = (FPyUFunctionDef*)FieldValue;
				if (!FPythonGeneratedClassUtil::CreateFunctionFromDefinition(Class, InPyType, FieldName, PyFuncDef))
				{
					return nullptr;
				}
			}
		}
	}

	// Replace the definitions with real descriptors
	if (!FPythonGeneratedClassUtil::RegisterDescriptors(Class, InPyType))
	{
		return nullptr;
	}

	// Let Python know that we've changed its type
	PyType_Modified(InPyType);

	// Finalize the class
	FPythonGeneratedClassUtil::FinalizeClass(Class, InPyType);

	// Re-instance the old class and re-parent any derived classes to this new type
	if (OldClass)
	{
		FPyWrapperTypeReinstancer::Get().AddPendingClass(OldClass, Class);
		ReparentDerivedClasses(OldClass, Class);
	}

	return Class;
}

bool UPythonGeneratedClass::ReparentDerivedClasses(UPythonGeneratedClass* InOldParent, UPythonGeneratedClass* InNewParent)
{
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(InOldParent, DerivedClasses, /*bRecursive*/false);

	bool bSuccess = true;

	for (UClass* DerivedClass : DerivedClasses)
	{
		if (DerivedClass->HasAnyClassFlags(CLASS_Native))
		{
			continue;
		}

		// todo: Blueprint classes?

		if (UPythonGeneratedClass* PyDerivedClass = Cast<UPythonGeneratedClass>(DerivedClass))
		{
			bSuccess &= !!ReparentClass(PyDerivedClass, InNewParent);
		}
	}

	return bSuccess;
}

UPythonGeneratedClass* UPythonGeneratedClass::ReparentClass(UPythonGeneratedClass* InOldClass, UPythonGeneratedClass* InNewParent)
{
	UObject* ClassOuter = GetPythonTypeContainer();
	const FString ClassName = InOldClass->GetName();

	FPythonGeneratedClassUtil::PrepareOldClassForReinstancing(InOldClass);
	UPythonGeneratedClass* Class = FPythonGeneratedClassUtil::CreateClass(ClassName, ClassOuter, InNewParent);
	PyTypeObject* PyType = InOldClass->PyType;

	// Copy the data from the old class
	Class->PyPostInitFunction = InOldClass->PyPostInitFunction;
	if (!FPythonGeneratedClassUtil::CopyPropertiesFromOldClass(Class, InOldClass, PyType))
	{
		return nullptr;
	}
	if (!FPythonGeneratedClassUtil::CopyFunctionsFromOldClass(Class, InOldClass, PyType))
	{
		return nullptr;
	}

	// Update the descriptors on the type so they reference the new class
	if (!FPythonGeneratedClassUtil::RegisterDescriptors(Class, PyType))
	{
		return nullptr;
	}

	// Update the base of the Python type
	FPythonGeneratedClassUtil::ReparentPythonType(PyType, InNewParent->PyType);

	// Let Python know that we've changed its type
	PyType_Modified(PyType);

	// Finalize the class
	FPythonGeneratedClassUtil::FinalizeClass(Class, PyType);

	// Re-instance the old class and re-parent any derived classes to this new type
	FPyWrapperTypeReinstancer::Get().AddPendingClass(InOldClass, Class);
	ReparentDerivedClasses(InOldClass, Class);

	return Class;
}

DEFINE_FUNCTION(UPythonGeneratedClass::CallPythonFunction)
{
	// Get the correct class from the UFunction so that we can perform static dispatch to the correct type
	const UPythonGeneratedClass* This = CastChecked<UPythonGeneratedClass>(Stack.Node->GetOwnerClass());

	// Find the Python function to call
	PyObject* PyFunc = nullptr;
	for (const TSharedPtr<PyGenUtil::FFunctionDef>& FuncDef : This->FunctionDefs)
	{
		if (FuncDef->GeneratedWrappedMethod.MethodFuncName == Stack.Node->GetFName())
		{
			PyFunc = FuncDef->PyFunction;
			break;
		}
	}
	if (!PyFunc)
	{
		UE_LOG(LogPython, Error, TEXT("Failed to find Python function for '%s' on '%s'"), *Stack.Node->GetName(), *This->GetName());
	}

	// Find the Python object to call the function on
	FPyObjectPtr PySelf;
	if (!Stack.Node->HasAnyFunctionFlags(FUNC_Static))
	{
		PySelf = FPyObjectPtr::StealReference((PyObject*)FPyWrapperObjectFactory::Get().CreateInstance(P_THIS_OBJECT));
		if (!PySelf)
		{
			UE_LOG(LogPython, Error, TEXT("Failed to create a Python wrapper for '%s'"), *P_THIS_OBJECT->GetName());
			return;
		}
	}

	auto DoCall = [&]() -> bool
	{
		if (Stack.Node->Children == nullptr)
		{
			// Simple case, no parameters or return value
			FPyObjectPtr PyArgs;
			if (PySelf)
			{
				PyArgs = FPyObjectPtr::StealReference(PyTuple_New(1));
				PyTuple_SetItem(PyArgs, 0, PySelf.Release()); // SetItem steals the reference
			}
			FPyObjectPtr RetVals = FPyObjectPtr::StealReference(PyObject_CallObject(PyFunc, PyArgs));
			if (!RetVals)
			{
				return false;
			}
		}
		else
		{
			// Complex case, parameters or return value
			TArray<FPyObjectPtr, TInlineAllocator<4>> PyParams;
			TArray<const UProperty*, TInlineAllocator<4>> OutParams;

			// Add the return property first
			const UProperty* ReturnProp = Stack.Node->GetReturnProperty();
			if (ReturnProp)
			{
				OutParams.Add(ReturnProp);
			}

			// Add any other output params in order, and get the value of the input params for the Python args
			int32 ArgIndex = 0;
			for (TFieldIterator<const UProperty> ParamIt(Stack.Node); ParamIt; ++ParamIt)
			{
				const UProperty* Param = *ParamIt;

				if (PyUtil::IsInputParameter(Param))
				{
					FPyObjectPtr& PyParam = PyParams[PyParams.AddDefaulted()];
					if (!PyConversion::PythonizeProperty_InContainer(Param, Stack.Locals, 0, PyParam.Get()))
					{
						PyUtil::SetPythonError(PyExc_TypeError, PyFunc, *FString::Printf(TEXT("Failed to convert argument at pos '%d' when calling function '%s' on '%s'"), ArgIndex + 1, *Stack.Node->GetName(), *P_THIS_OBJECT->GetName()));
						return false;
					}

					++ArgIndex;
				}

				if (PyUtil::IsOutputParameter(Param))
				{
					OutParams.Add(Param);
				}
			}

			const int32 PyParamOffset = (PySelf ? 1 : 0);
			FPyObjectPtr PyArgs = FPyObjectPtr::StealReference(PyTuple_New(PyParams.Num() + PyParamOffset));
			if (PySelf)
			{
				PyTuple_SetItem(PyArgs, 0, PySelf.Release()); // SetItem steals the reference
			}
			for (int32 PyParamIndex = 0; PyParamIndex < PyParams.Num(); ++PyParamIndex)
			{
				PyTuple_SetItem(PyArgs, PyParamIndex + PyParamOffset, PyParams[PyParamIndex].Release()); // SetItem steals the reference
			}

			FPyObjectPtr RetVals = FPyObjectPtr::StealReference(PyObject_CallObject(PyFunc, PyArgs));
			if (!RetVals)
			{
				return false;
			}

			if (!PyUtil::UnpackReturnValues(RetVals, Stack.Node, OutParams, Stack.Locals, *PyUtil::GetErrorContext(PyFunc), *FString::Printf(TEXT("function '%s' on '%s'"), *Stack.Node->GetName(), *P_THIS_OBJECT->GetName())))
			{
				return false;
			}

			if (ReturnProp)
			{
				ReturnProp->CopyCompleteValue(RESULT_PARAM, ReturnProp->ContainerPtrToValuePtr<void>(Stack.Locals));
				
				for (FOutParmRec* OutParamRec = Stack.OutParms; OutParamRec; OutParamRec = OutParamRec->NextOutParm)
				{
					ReturnProp->CopyCompleteValue(OutParamRec->PropAddr, OutParamRec->Property->ContainerPtrToValuePtr<void>(Stack.Locals));
				}
			}
		}

		return true;
	};

	if (!DoCall())
	{
		PyUtil::LogPythonError();
	}
}

#endif	// WITH_PYTHON

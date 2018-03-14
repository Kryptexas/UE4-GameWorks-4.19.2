// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperDelegate.h"
#include "PyWrapperObject.h"
#include "PyWrapperTypeRegistry.h"
#include "PyUtil.h"
#include "PyConversion.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "Templates/Casts.h"

#if WITH_PYTHON

void InitializePyWrapperDelegate(PyObject* PyModule)
{
	if (PyType_Ready(&PyWrapperDelegateType) == 0)
	{
		static FPyWrapperDelegateMetaData MetaData;
		FPyWrapperDelegateMetaData::SetMetaData(&PyWrapperDelegateType, &MetaData);

		Py_INCREF(&PyWrapperDelegateType);
		PyModule_AddObject(PyModule, PyWrapperDelegateType.tp_name, (PyObject*)&PyWrapperDelegateType);
	}

	if (PyType_Ready(&PyWrapperMulticastDelegateType) == 0)
	{
		static FPyWrapperMulticastDelegateMetaData MetaData;
		FPyWrapperMulticastDelegateMetaData::SetMetaData(&PyWrapperMulticastDelegateType, &MetaData);

		Py_INCREF(&PyWrapperMulticastDelegateType);
		PyModule_AddObject(PyModule, PyWrapperMulticastDelegateType.tp_name, (PyObject*)&PyWrapperMulticastDelegateType);
	}
}


namespace PyDelegateUtil
{

bool PythonArgsToDelegate(PyObject* InArgs, const UFunction* InDelegateSignature, FScriptDelegate& OutDelegate, const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt)
{
	PyObject* PyObj = nullptr;
	PyObject* PyFuncNameObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, TCHAR_TO_UTF8(*FString::Printf(TEXT("OO:%s"), InFuncCtxt)), &PyObj, &PyFuncNameObj))
	{
		return false;
	}

	UObject* Obj = nullptr;
	if (!PyConversion::Nativize(PyObj, Obj))
	{
		return false;
	}

	FName FuncName;
	if (!PyConversion::Nativize(PyFuncNameObj, FuncName))
	{
		return false;
	}

	if (Obj)
	{
		check(PyObj);

		// Is the function name we've been given a Python alias? If so, we need to resolve that now
		FuncName = FPyWrapperObjectMetaData::ResolveFunctionName(Py_TYPE(PyObj), FuncName);
		const UFunction* Func = Obj->FindFunction(FuncName);

		// Valid signature?
		if (Func && !InDelegateSignature->IsSignatureCompatibleWith(Func))
		{
			PyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Function '%s' on '%s' does not match the signature required by the delegate '%s'"), *Func->GetName(), *Obj->GetName(), *InDelegateSignature->GetName()));
			return false;
		}
	}

	OutDelegate.BindUFunction(Obj, FuncName);
	return true;
}

} // namespace PyDelegateUtil


template <typename DelegateType>
struct TPyDelegateInvocation
{
};

template <>
struct TPyDelegateInvocation<FScriptDelegate>
{
	static bool CanCall(const FScriptDelegate& Delegate)
	{
		return Delegate.IsBound();
	}

	static void Call(const FScriptDelegate& Delegate, void* Params)
	{
		Delegate.ProcessDelegate<UObject>(Params);
	}
};

template <>
struct TPyDelegateInvocation<FMulticastScriptDelegate>
{
	static bool CanCall(const FMulticastScriptDelegate& Delegate)
	{
		return true;
	}

	static void Call(const FMulticastScriptDelegate& Delegate, void* Params)
	{
		Delegate.ProcessMulticastDelegate<UObject>(Params);
	}
};


template <typename WrapperType, typename MetaDataType, typename DelegateType, typename FactoryType>
struct TPyWrapperDelegateImpl
{
	static WrapperType* New(PyTypeObject* InType)
	{
		WrapperType* Self = (WrapperType*)FPyWrapperBase::New(InType);
		if (Self)
		{
			new(&Self->OwnerContext) FPyWrapperOwnerContext();
			Self->DelegateSignature = nullptr;
			Self->DelegateInstance = nullptr;
			new(&Self->InternalDelegateInstance) DelegateType();
		}
		return Self;
	}

	static void Free(WrapperType* InSelf)
	{
		Deinit(InSelf);

		InSelf->OwnerContext.~FPyWrapperOwnerContext();
		InSelf->InternalDelegateInstance.~DelegateType();
		FPyWrapperBase::Free(InSelf);
	}

	static int Init(WrapperType* InSelf)
	{
		Deinit(InSelf);

		const int BaseInit = FPyWrapperBase::Init(InSelf);
		if (BaseInit != 0)
		{
			return BaseInit;
		}

		const UFunction* DelegateSignature = MetaDataType::GetDelegateSignature(InSelf);
		if (!DelegateSignature)
		{
			PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("DelegateSignature is null"));
			return -1;
		}

		InSelf->DelegateSignature = DelegateSignature;
		InSelf->DelegateInstance = &InSelf->InternalDelegateInstance;

		FactoryType::Get().MapInstance(InSelf->DelegateInstance, InSelf);
		return 0;
	}

	static int Init(WrapperType* InSelf, const FPyWrapperOwnerContext& InOwnerContext, const UFunction* InDelegateSignature, DelegateType* InValue, const EPyConversionMethod InConversionMethod)
	{
		InOwnerContext.AssertValidConversionMethod(InConversionMethod);

		Deinit(InSelf);

		const int BaseInit = FPyWrapperBase::Init(InSelf);
		if (BaseInit != 0)
		{
			return BaseInit;
		}

		check(InValue);

		DelegateType* DelegateInstanceToUse = &InSelf->InternalDelegateInstance;
		switch (InConversionMethod)
		{
		case EPyConversionMethod::Copy:
		case EPyConversionMethod::Steal:
			InSelf->InternalDelegateInstance = *InValue;
			break;

		case EPyConversionMethod::Reference:
			DelegateInstanceToUse = InValue;
			break;

		default:
			checkf(false, TEXT("Unknown EPyConversionMethod"));
			break;
		}

		check(DelegateInstanceToUse);

		InSelf->OwnerContext = InOwnerContext;
		InSelf->DelegateSignature = InDelegateSignature;
		InSelf->DelegateInstance = DelegateInstanceToUse;

		FactoryType::Get().MapInstance(InSelf->DelegateInstance, InSelf);
		return 0;
	}

	static void Deinit(WrapperType* InSelf)
	{
		if (InSelf->DelegateInstance)
		{
			FactoryType::Get().UnmapInstance(InSelf->DelegateInstance, Py_TYPE(InSelf));
		}

		if (InSelf->OwnerContext.HasOwner())
		{
			InSelf->OwnerContext.Reset();
		}

		InSelf->DelegateSignature = nullptr;
		InSelf->DelegateInstance = nullptr;
		InSelf->InternalDelegateInstance.Clear();
	}

	static bool ValidateInternalState(WrapperType* InSelf)
	{
		if (!InSelf->DelegateSignature)
		{
			PyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - DelegateSignature is null!"));
			return false;
		}

		if (!InSelf->DelegateInstance)
		{
			PyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - DelegateInstance is null!"));
			return false;
		}

		return true;
	}

	static PyObject* CallDelegate(WrapperType* InSelf, PyObject* InArgs)
	{
		typedef TPyDelegateInvocation<DelegateType> FDelegateInvocation;

		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (!FDelegateInvocation::CanCall(*InSelf->DelegateInstance))
		{
			PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot call an unbound delegate"));
			return nullptr;
		}

		if (InSelf->DelegateSignature->Children == nullptr)
		{
			// Simple case, no parameters or return value
			FDelegateInvocation::Call(*InSelf->DelegateInstance, nullptr);
			Py_RETURN_NONE;
		}

		// Complex case, parameters or return value
		FStructOnScope DelegateParams(InSelf->DelegateSignature);
		TArray<const UProperty*, TInlineAllocator<4>> OutParams;

		// Add the return property first
		if (const UProperty* ReturnProp = InSelf->DelegateSignature->GetReturnProperty())
		{
			OutParams.Add(ReturnProp);
		}

		// Add any other output params in order, and set the value of the input params from the Python args
		int32 ArgIndex = 0;
		const int32 NumArgs = PyTuple_GET_SIZE(InArgs);
		for (TFieldIterator<const UProperty> ParamIt(InSelf->DelegateSignature); ParamIt; ++ParamIt)
		{
			const UProperty* Param = *ParamIt;

			if (PyUtil::IsInputParameter(Param))
			{
				PyObject* ParsedArg = nullptr;
				if (ArgIndex < NumArgs)
				{
					ParsedArg = PyTuple_GET_ITEM(InArgs, ArgIndex);
				}

				if (!ParsedArg)
				{
					PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Required argument at pos %d not found"), ArgIndex + 1));
					return nullptr;
				}

				if (!PyConversion::NativizeProperty_InContainer(ParsedArg, Param, DelegateParams.GetStructMemory(), 0))
				{
					PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert argument at pos '%d' when calling delegate"), ArgIndex + 1));
					return nullptr;
				}

				++ArgIndex;
			}

			if (PyUtil::IsOutputParameter(Param))
			{
				OutParams.Add(Param);
			}
		}

		// Too many arguments?
		if (ArgIndex > NumArgs)
		{
			PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Delegate takes at most %d argument%s (%d given)"), ArgIndex, (ArgIndex == 1 ? TEXT("") : TEXT("s")), NumArgs));
			return nullptr;
		}

		FDelegateInvocation::Call(*InSelf->DelegateInstance, DelegateParams.GetStructMemory());
		return PyUtil::PackReturnValues(InSelf->DelegateSignature, OutParams, DelegateParams.GetStructMemory(), *PyUtil::GetErrorContext(InSelf), TEXT("delegate"));
	}
};

typedef TPyWrapperDelegateImpl<FPyWrapperDelegate, FPyWrapperDelegateMetaData, FScriptDelegate, FPyWrapperDelegateFactory> FPyWrapperDelegateImpl;
typedef TPyWrapperDelegateImpl<FPyWrapperMulticastDelegate, FPyWrapperMulticastDelegateMetaData, FMulticastScriptDelegate, FPyWrapperMulticastDelegateFactory> FPyWrapperMulticastDelegateImpl;


FPyWrapperDelegate* FPyWrapperDelegate::New(PyTypeObject* InType)
{
	return FPyWrapperDelegateImpl::New(InType);
}

void FPyWrapperDelegate::Free(FPyWrapperDelegate* InSelf)
{
	FPyWrapperDelegateImpl::Free(InSelf);
}

int FPyWrapperDelegate::Init(FPyWrapperDelegate* InSelf)
{
	return FPyWrapperDelegateImpl::Init(InSelf);
}

int FPyWrapperDelegate::Init(FPyWrapperDelegate* InSelf, const FPyWrapperOwnerContext& InOwnerContext, const UFunction* InDelegateSignature, FScriptDelegate* InValue, const EPyConversionMethod InConversionMethod)
{
	return FPyWrapperDelegateImpl::Init(InSelf, InOwnerContext, InDelegateSignature, InValue, InConversionMethod);
}

void FPyWrapperDelegate::Deinit(FPyWrapperDelegate* InSelf)
{
	FPyWrapperDelegateImpl::Deinit(InSelf);
}

bool FPyWrapperDelegate::ValidateInternalState(FPyWrapperDelegate* InSelf)
{
	return FPyWrapperDelegateImpl::ValidateInternalState(InSelf);
}

FPyWrapperDelegate* FPyWrapperDelegate::CastPyObject(PyObject* InPyObject)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperDelegateType) == 1)
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperDelegate*)InPyObject;
	}

	return nullptr;
}

FPyWrapperDelegate* FPyWrapperDelegate::CastPyObject(PyObject* InPyObject, PyTypeObject* InType)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &PyWrapperDelegateType || PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperDelegateType) == 1))
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperDelegate*)InPyObject;
	}

	return nullptr;
}

PyObject* FPyWrapperDelegate::CallDelegate(FPyWrapperDelegate* InSelf, PyObject* InArgs)
{
	return FPyWrapperDelegateImpl::CallDelegate(InSelf, InArgs);
}


FPyWrapperMulticastDelegate* FPyWrapperMulticastDelegate::New(PyTypeObject* InType)
{
	return FPyWrapperMulticastDelegateImpl::New(InType);
}

void FPyWrapperMulticastDelegate::Free(FPyWrapperMulticastDelegate* InSelf)
{
	FPyWrapperMulticastDelegateImpl::Free(InSelf);
}

int FPyWrapperMulticastDelegate::Init(FPyWrapperMulticastDelegate* InSelf)
{
	return FPyWrapperMulticastDelegateImpl::Init(InSelf);
}

int FPyWrapperMulticastDelegate::Init(FPyWrapperMulticastDelegate* InSelf, const FPyWrapperOwnerContext& InOwnerContext, const UFunction* InDelegateSignature, FMulticastScriptDelegate* InValue, const EPyConversionMethod InConversionMethod)
{
	return FPyWrapperMulticastDelegateImpl::Init(InSelf, InOwnerContext, InDelegateSignature, InValue, InConversionMethod);
}

void FPyWrapperMulticastDelegate::Deinit(FPyWrapperMulticastDelegate* InSelf)
{
	FPyWrapperMulticastDelegateImpl::Deinit(InSelf);
}

bool FPyWrapperMulticastDelegate::ValidateInternalState(FPyWrapperMulticastDelegate* InSelf)
{
	return FPyWrapperMulticastDelegateImpl::ValidateInternalState(InSelf);
}

FPyWrapperMulticastDelegate* FPyWrapperMulticastDelegate::CastPyObject(PyObject* InPyObject)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperMulticastDelegateType) == 1)
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperMulticastDelegate*)InPyObject;
	}

	return nullptr;
}

FPyWrapperMulticastDelegate* FPyWrapperMulticastDelegate::CastPyObject(PyObject* InPyObject, PyTypeObject* InType)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &PyWrapperMulticastDelegateType || PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperMulticastDelegateType) == 1))
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperMulticastDelegate*)InPyObject;
	}

	return nullptr;
}

PyObject* FPyWrapperMulticastDelegate::CallDelegate(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
{
	return FPyWrapperMulticastDelegateImpl::CallDelegate(InSelf, InArgs);
}


PyTypeObject InitializePyWrapperDelegateType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyWrapperDelegate::New(InType);
		}

		static void Dealloc(FPyWrapperDelegate* InSelf)
		{
			FPyWrapperDelegate::Free(InSelf);
		}

		static int Init(FPyWrapperDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			const int BaseInit = FPyWrapperDelegate::Init(InSelf);
			if (BaseInit != 0)
			{
				return BaseInit;
			}

			if (PyTuple_Size(InArgs) > 0 && !PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, *InSelf->DelegateInstance, TEXT("call"), *PyUtil::GetErrorContext(InSelf)))
			{
				return -1;
			}

			return 0;
		}

		static PyObject* Str(FPyWrapperDelegate* InSelf)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyUnicode_FromFormat("<Delegate '%s' (%p) %s>", TCHAR_TO_UTF8(*PyUtil::GetFriendlyTypename(InSelf)), InSelf->DelegateInstance, TCHAR_TO_UTF8(*InSelf->DelegateInstance->ToString<UObject>()));
		}

		static PyObject* Call(FPyWrapperDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (InKwds && PyDict_Size(InKwds) != 0)
			{
				PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot call a delegate with keyword arguments"));
				return nullptr;
			}

			return FPyWrapperDelegate::CallDelegate(InSelf, InArgs);
		}
	};

	struct FNumberFuncs
	{
		static int Bool(FPyWrapperDelegate* InSelf)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return -1;
			}

			return InSelf->DelegateInstance->IsBound() ? 1 : 0;
		}
	};

	struct FMethods
	{
		static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
		{
			PyObject* PyObj = nullptr;
			if (PyArg_ParseTuple(InArgs, "O:cast", &PyObj))
			{
				PyObject* PyCastResult = (PyObject*)FPyWrapperDelegate::CastPyObject(PyObj, InType);
				if (!PyCastResult)
				{
					PyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *PyUtil::GetFriendlyTypename(PyObj), *PyUtil::GetFriendlyTypename(InType)));
				}
				return PyCastResult;
			}

			return nullptr;
		}

		static PyObject* Copy(FPyWrapperDelegate* InSelf)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return (PyObject*)FPyWrapperDelegateFactory::Get().CreateInstance(InSelf->DelegateSignature, InSelf->DelegateInstance, FPyWrapperOwnerContext(), EPyConversionMethod::Copy);
		}

		static PyObject* IsBound(FPyWrapperDelegate* InSelf)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			if (InSelf->DelegateInstance->IsBound())
			{
				Py_RETURN_TRUE;
			}

			Py_RETURN_FALSE;
		}

		static PyObject* BindFunction(FPyWrapperDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			if (!PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, *InSelf->DelegateInstance, TEXT("bind_function"), *PyUtil::GetErrorContext(InSelf)))
			{
				return nullptr;
			}

			Py_RETURN_NONE;
		}

		static PyObject* Unbind(FPyWrapperDelegate* InSelf)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			InSelf->DelegateInstance->Unbind();

			Py_RETURN_NONE;
		}

		static PyObject* Execute(FPyWrapperDelegate* InSelf, PyObject* InArgs)
		{
			return FPyWrapperDelegate::CallDelegate(InSelf, InArgs);
		}

		static PyObject* ExecuteIfBound(FPyWrapperDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			if (InSelf->DelegateInstance->IsBound())
			{
				return FPyWrapperDelegate::CallDelegate(InSelf, InArgs);
			}

			Py_RETURN_NONE;
		}
	};

	static PyMethodDef PyMethods[] = {
		{ "cast", PyCFunctionCast(&FMethods::Cast), METH_VARARGS | METH_CLASS, "X.cast(object) -> struct -- cast the given object to this Unreal delegate type" },
		{ "__copy__", PyCFunctionCast(&FMethods::Copy), METH_NOARGS, "x.__copy__() -> struct -- copy this Unreal delegate" },
		{ "copy", PyCFunctionCast(&FMethods::Copy), METH_NOARGS, "x.copy() -> struct -- copy this Unreal delegate" },
		{ "is_bound", PyCFunctionCast(&FMethods::IsBound), METH_NOARGS, "x.is_bound() -> bool -- is this Unreal delegate bound to something?" },
		{ "bind_function", PyCFunctionCast(&FMethods::BindFunction), METH_VARARGS, "x.bind_function(obj, name) -- bind this Unreal delegate to a named Unreal function on the given object instance" },
		{ "unbind", PyCFunctionCast(&FMethods::Unbind), METH_NOARGS, "x.unbind() -- unbind this Unreal delegate" },
		{ "execute", PyCFunctionCast(&FMethods::Execute), METH_VARARGS, "x.execute(...) -> value -- call this Unreal delegate, but error if it's unbound" },
		{ "execute_if_bound", PyCFunctionCast(&FMethods::ExecuteIfBound), METH_VARARGS, "x.execute_if_bound(...) -> value -- call this Unreal delegate, but only if it's bound to something" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"PyWrapperDelegate", /* tp_name */
		sizeof(FPyWrapperDelegate), /* tp_basicsize */
	};

	PyType.tp_base = &PyWrapperBaseType;
	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;
	PyType.tp_str = (reprfunc)&FFuncs::Str;
	PyType.tp_call = (ternaryfunc)&FFuncs::Call;

	PyType.tp_methods = PyMethods;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type for all UE4 exposed delegate instances";

	static PyNumberMethods PyNumber;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_bool = (inquiry)&FNumberFuncs::Bool;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_nonzero = (inquiry)&FNumberFuncs::Bool;
#endif	// PY_MAJOR_VERSION >= 3

	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject InitializePyWrapperMulticastDelegateType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyWrapperMulticastDelegate::New(InType);
		}

		static void Dealloc(FPyWrapperMulticastDelegate* InSelf)
		{
			FPyWrapperMulticastDelegate::Free(InSelf);
		}

		static int Init(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			const int BaseInit = FPyWrapperMulticastDelegate::Init(InSelf);
			if (BaseInit != 0)
			{
				return BaseInit;
			}

			if (PyTuple_Size(InArgs) > 0)
			{
				FScriptDelegate Delegate;
				if (!PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, Delegate, TEXT("call"), *PyUtil::GetErrorContext(InSelf)))
				{
					return -1;
				}
				InSelf->DelegateInstance->Add(Delegate);
			}

			return 0;
		}

		static PyObject* Str(FPyWrapperMulticastDelegate* InSelf)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return PyUnicode_FromFormat("<Multicast delegate '%s' (%p) %s>", TCHAR_TO_UTF8(*PyUtil::GetFriendlyTypename(InSelf)), InSelf->DelegateInstance, TCHAR_TO_UTF8(*InSelf->DelegateInstance->ToString<UObject>()));
		}

		static PyObject* Call(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (InKwds && PyDict_Size(InKwds) != 0)
			{
				PyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot call a delegate with keyword arguments"));
				return nullptr;
			}

			return FPyWrapperMulticastDelegate::CallDelegate(InSelf, InArgs);
		}
	};

	struct FNumberFuncs
	{
		static int Bool(FPyWrapperMulticastDelegate* InSelf)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return -1;
			}

			return InSelf->DelegateInstance->IsBound() ? 1 : 0;
		}
	};

	struct FMethods
	{
		static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
		{
			PyObject* PyObj = nullptr;
			if (PyArg_ParseTuple(InArgs, "O:cast", &PyObj))
			{
				PyObject* PyCastResult = (PyObject*)FPyWrapperMulticastDelegate::CastPyObject(PyObj, InType);
				if (!PyCastResult)
				{
					PyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *PyUtil::GetFriendlyTypename(PyObj), *PyUtil::GetFriendlyTypename(InType)));
				}
				return PyCastResult;
			}

			return nullptr;
		}

		static PyObject* Copy(FPyWrapperMulticastDelegate* InSelf)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			return (PyObject*)FPyWrapperMulticastDelegateFactory::Get().CreateInstance(InSelf->DelegateSignature, InSelf->DelegateInstance, FPyWrapperOwnerContext(), EPyConversionMethod::Copy);
		}

		static PyObject* IsBound(FPyWrapperMulticastDelegate* InSelf)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			if (InSelf->DelegateInstance->IsBound())
			{
				Py_RETURN_TRUE;
			}

			Py_RETURN_FALSE;
		}

		static PyObject* AddFunction(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			FScriptDelegate Delegate;
			if (!PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, Delegate, TEXT("add_function"), *PyUtil::GetErrorContext(InSelf)))
			{
				return nullptr;
			}

			InSelf->DelegateInstance->Add(Delegate);

			Py_RETURN_NONE;
		}

		static PyObject* AddFunctionUnique(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			FScriptDelegate Delegate;
			if (!PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, Delegate, TEXT("add_function_unique"), *PyUtil::GetErrorContext(InSelf)))
			{
				return nullptr;
			}

			InSelf->DelegateInstance->AddUnique(Delegate);

			Py_RETURN_NONE;
		}

		static PyObject* RemoveFunction(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			FScriptDelegate Delegate;
			if (!PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, Delegate, TEXT("remove_function"), *PyUtil::GetErrorContext(InSelf)))
			{
				return nullptr;
			}

			InSelf->DelegateInstance->Remove(Delegate);

			Py_RETURN_NONE;
		}

		static PyObject* RemoveObject(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyObj = nullptr;
			if (!PyArg_ParseTuple(InArgs, "O:remove_object", &PyObj))
			{
				return nullptr;
			}

			UObject* Obj = nullptr;
			if (!PyConversion::Nativize(PyObj, Obj))
			{
				return nullptr;
			}

			InSelf->DelegateInstance->RemoveAll(Obj);

			Py_RETURN_NONE;
		}

		static PyObject* ContainsFunction(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			FScriptDelegate Delegate;
			if (!PyDelegateUtil::PythonArgsToDelegate(InArgs, InSelf->DelegateSignature, Delegate, TEXT("contains_function"), *PyUtil::GetErrorContext(InSelf)))
			{
				return nullptr;
			}

			if (InSelf->DelegateInstance->Contains(Delegate))
			{
				Py_RETURN_TRUE;
			}

			Py_RETURN_FALSE;
		}

		static PyObject* Clear(FPyWrapperMulticastDelegate* InSelf)
		{
			if (!FPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			InSelf->DelegateInstance->Clear();

			Py_RETURN_NONE;
		}

		static PyObject* Broadcast(FPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
		{
			return FPyWrapperMulticastDelegate::CallDelegate(InSelf, InArgs);
		}
	};

	static PyMethodDef PyMethods[] = {
		{ "cast", PyCFunctionCast(&FMethods::Cast), METH_VARARGS | METH_CLASS, "X.cast(object) -> struct -- cast the given object to this Unreal delegate type" },
		{ "__copy__", PyCFunctionCast(&FMethods::Copy), METH_NOARGS, "x.__copy__() -> struct -- copy this Unreal delegate" },
		{ "copy", PyCFunctionCast(&FMethods::Copy), METH_NOARGS, "x.copy() -> struct -- copy this Unreal delegate" },
		{ "is_bound", PyCFunctionCast(&FMethods::IsBound), METH_NOARGS, "x.is_bound() -> bool -- is this Unreal delegate bound to something?" },
		{ "add_function", PyCFunctionCast(&FMethods::AddFunction), METH_VARARGS, "x.add_function(obj, name) -- add a binding to a named Unreal function on the given object instance to the invocation list of this Unreal delegate" },
		{ "add_function_unique", PyCFunctionCast(&FMethods::AddFunctionUnique), METH_VARARGS, "x.add_function_unique(obj, name) -- add a unique binding to a named Unreal function on the given object instance to the invocation list of this Unreal delegate" },
		{ "remove_function", PyCFunctionCast(&FMethods::RemoveFunction), METH_VARARGS, "x.remove_function(obj, name) -- remove a binding to a named Unreal function on the given object instance from the invocation list of this Unreal delegate" },
		{ "remove_object", PyCFunctionCast(&FMethods::RemoveObject), METH_VARARGS, "x.remove_object(obj) -- remove all bindings for the given object instance from the invocation list of this Unreal delegate" },
		{ "contains_function", PyCFunctionCast(&FMethods::ContainsFunction), METH_VARARGS, "x.contains_function(obj, name) -- does the invocation list of this Unreal delegate contain a binding to a named Unreal function on the given object instance" },
		{ "clear", PyCFunctionCast(&FMethods::Clear), METH_NOARGS, "x.clear() -- clear the invocation list of this Unreal delegate" },
		{ "broadcast", PyCFunctionCast(&FMethods::Broadcast), METH_VARARGS, "x.broadcast(...) -- invoke this Unreal multicast delegate" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"PyWrapperMulticastDelegate", /* tp_name */
		sizeof(FPyWrapperMulticastDelegate), /* tp_basicsize */
	};

	PyType.tp_base = &PyWrapperBaseType;
	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;
	PyType.tp_str = (reprfunc)&FFuncs::Str;
	PyType.tp_call = (ternaryfunc)&FFuncs::Call;

	PyType.tp_methods = PyMethods;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type for all UE4 exposed multicast delegate instances";

	static PyNumberMethods PyNumber;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_bool = (inquiry)&FNumberFuncs::Bool;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_nonzero = (inquiry)&FNumberFuncs::Bool;
#endif	// PY_MAJOR_VERSION >= 3

	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject PyWrapperDelegateType = InitializePyWrapperDelegateType();
PyTypeObject PyWrapperMulticastDelegateType = InitializePyWrapperMulticastDelegateType();

#endif	// WITH_PYTHON

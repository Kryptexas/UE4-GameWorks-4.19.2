// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperName.h"
#include "PyWrapperTypeRegistry.h"
#include "PyUtil.h"
#include "PyConversion.h"

#if WITH_PYTHON

void InitializePyWrapperName(PyObject* PyModule)
{
	if (PyType_Ready(&PyWrapperNameType) == 0)
	{
		Py_INCREF(&PyWrapperNameType);
		PyModule_AddObject(PyModule, PyWrapperNameType.tp_name, (PyObject*)&PyWrapperNameType);
	}
}

FPyWrapperName* FPyWrapperName::New(PyTypeObject* InType)
{
	FPyWrapperName* Self = (FPyWrapperName*)FPyWrapperBase::New(InType);
	if (Self)
	{
		new(&Self->Name) FName();
	}
	return Self;
}

void FPyWrapperName::Free(FPyWrapperName* InSelf)
{
	Deinit(InSelf);

	InSelf->Name.~FName();
	FPyWrapperBase::Free(InSelf);
}

int FPyWrapperName::Init(FPyWrapperName* InSelf)
{
	Deinit(InSelf);

	const int BaseInit = FPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	FPyWrapperNameFactory::Get().MapInstance(InSelf->Name, InSelf);
	return 0;
}

int FPyWrapperName::Init(FPyWrapperName* InSelf, const FName InValue)
{
	Deinit(InSelf);

	const int BaseInit = FPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	InSelf->Name = InValue;
	FPyWrapperNameFactory::Get().MapInstance(InSelf->Name, InSelf);
	return 0;
}

void FPyWrapperName::Deinit(FPyWrapperName* InSelf)
{
	FPyWrapperNameFactory::Get().UnmapInstance(InSelf->Name, Py_TYPE(InSelf));
	InSelf->Name = FName();
}

FPyWrapperName* FPyWrapperName::CastPyObject(PyObject* InPyObject)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperNameType) == 1)
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperName*)InPyObject;
	}

	return nullptr;
}

FPyWrapperName* FPyWrapperName::CastPyObject(PyObject* InPyObject, PyTypeObject* InType)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &PyWrapperNameType || PyObject_IsInstance(InPyObject, (PyObject*)&PyWrapperNameType) == 1))
	{
		Py_INCREF(InPyObject);
		return (FPyWrapperName*)InPyObject;
	}

	{
		FName InitValue;
		if (PyConversion::Nativize(InPyObject, InitValue))
		{
			FPyWrapperNamePtr NewName = FPyWrapperNamePtr::StealReference(FPyWrapperName::New(InType));
			if (NewName)
			{
				if (FPyWrapperName::Init(NewName, InitValue) != 0)
				{
					return nullptr;
				}
			}
			return NewName.Release();
		}
	}

	return nullptr;
}

PyTypeObject InitializePyWrapperNameType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyWrapperName::New(InType);
		}

		static void Dealloc(FPyWrapperName* InSelf)
		{
			FPyWrapperName::Free(InSelf);
		}

		static int Init(FPyWrapperName* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			FName InitValue;

			PyObject* PyObj = nullptr;
			if (PyArg_ParseTuple(InArgs, "|O:call", &PyObj))
			{
				if (PyObj && !PyConversion::Nativize(PyObj, InitValue))
				{
					PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert init argument '%s' to 'Name'"), *PyUtil::GetFriendlyTypename(PyObj)));
					return -1;
				}
			}

			return FPyWrapperName::Init(InSelf, InitValue);
		}

		static PyObject* Str(FPyWrapperName* InSelf)
		{
			return PyUnicode_FromString(TCHAR_TO_UTF8(*InSelf->Name.ToString()));
		}

		static PyObject* RichCmp(FPyWrapperName* InSelf, PyObject* InOther, int InOp)
		{
			FName Other;
			if (!PyConversion::Nativize(InOther, Other, PyConversion::ESetErrorState::No))
			{
				Py_INCREF(Py_NotImplemented);
				return Py_NotImplemented;
			}

			return PyUtil::PyRichCmp(InSelf->Name.Compare(Other), 0, InOp);
		}

		static PyUtil::FPyHashType Hash(FPyWrapperName* InSelf)
		{
			const PyUtil::FPyHashType PyHash = (PyUtil::FPyHashType)GetTypeHash(InSelf->Name);
			return PyHash != -1 ? PyHash : 0;
		}
	};

	struct FMethods
	{
		static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
		{
			PyObject* PyObj = nullptr;
			if (PyArg_ParseTuple(InArgs, "O:call", &PyObj))
			{
				PyObject* PyCastResult = (PyObject*)FPyWrapperName::CastPyObject(PyObj, InType);
				if (!PyCastResult)
				{
					PyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *PyUtil::GetFriendlyTypename(PyObj), *PyUtil::GetFriendlyTypename(InType)));
				}
				return PyCastResult;
			}

			return nullptr;
		}

		static PyObject* IsValid(FPyWrapperName* InSelf)
		{
			if (InSelf->Name.IsValid())
			{
				Py_RETURN_TRUE;
			}

			Py_RETURN_FALSE;
		}

		static PyObject* IsNone(FPyWrapperName* InSelf)
		{
			if (InSelf->Name.IsNone())
			{
				Py_RETURN_TRUE;
			}

			Py_RETURN_FALSE;
		}
	};

	static PyMethodDef PyMethods[] = {
		{ "cast", PyCFunctionCast(&FMethods::Cast), METH_VARARGS | METH_CLASS, "X.cast(object) -> FName -- cast the given object to this Unreal name type" },
		{ "is_valid", PyCFunctionCast(&FMethods::IsValid), METH_NOARGS, "x.is_valid() -> bool -- is this Unreal name valid?" },
		{ "is_none", PyCFunctionCast(&FMethods::IsNone), METH_NOARGS, "x.is_none() -> bool -- is this Unreal name set to NAME_None?" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"Name", /* tp_name */
		sizeof(FPyWrapperName), /* tp_basicsize */
	};

	PyType.tp_base = &PyWrapperBaseType;
	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;
	PyType.tp_str = (reprfunc)&FFuncs::Str;
	PyType.tp_richcompare = (richcmpfunc)&FFuncs::RichCmp;
	PyType.tp_hash = (hashfunc)&FFuncs::Hash;

	PyType.tp_methods = PyMethods;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type for all UE4 exposed FName instances";

	return PyType;
}

PyTypeObject PyWrapperNameType = InitializePyWrapperNameType();

#endif	// WITH_PYTHON

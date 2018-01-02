// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyEditor.h"
#include "PyUtil.h"
#include "PyConversion.h"

#if WITH_EDITOR
#include "Editor.h"
#endif	// WITH_EDITOR

#if WITH_PYTHON

#if WITH_EDITOR

FPyScopedEditorTransaction* FPyScopedEditorTransaction::New(PyTypeObject* InType)
{
	FPyScopedEditorTransaction* Self = (FPyScopedEditorTransaction*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		new(&Self->Description) FText();
		Self->PendingTransactionId = INDEX_NONE;
	}
	return Self;
}

void FPyScopedEditorTransaction::Free(FPyScopedEditorTransaction* InSelf)
{
	Deinit(InSelf);

	InSelf->Description.~FText();
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FPyScopedEditorTransaction::Init(FPyScopedEditorTransaction* InSelf, const FText& InDescription)
{
	Deinit(InSelf);

	InSelf->Description = InDescription;

	return 0;
}

void FPyScopedEditorTransaction::Deinit(FPyScopedEditorTransaction* InSelf)
{
	InSelf->Description = FText();
	InSelf->PendingTransactionId = INDEX_NONE;
}

PyTypeObject InitializePyScopedEditorTransactionType()
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FPyScopedEditorTransaction::New(InType);
		}

		static void Dealloc(FPyScopedEditorTransaction* InSelf)
		{
			FPyScopedEditorTransaction::Free(InSelf);
		}

		static int Init(FPyScopedEditorTransaction* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyDescObj = nullptr;

			static const char *ArgsKwdList[] = { "desc", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:call", (char**)ArgsKwdList, &PyDescObj))
			{
				return -1;
			}

			FText Desc;
			if (!PyConversion::Nativize(PyDescObj, Desc))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'desc' (%s) to 'Text'"), *PyUtil::GetFriendlyTypename(PyDescObj)));
				return -1;
			}

			return FPyScopedEditorTransaction::Init(InSelf, Desc);
		}
	};

	struct FMethods
	{
		static PyObject* EnterScope(FPyScopedEditorTransaction* InSelf)
		{
			ensure(InSelf->PendingTransactionId == INDEX_NONE);
			InSelf->PendingTransactionId = GEditor->BeginTransaction(InSelf->Description);

			Py_INCREF(InSelf);
			return (PyObject*)InSelf;
		}

		static PyObject* ExitScope(FPyScopedEditorTransaction* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (InSelf->PendingTransactionId != INDEX_NONE)
			{
				GEditor->EndTransaction();
				InSelf->PendingTransactionId = INDEX_NONE;
			}

			Py_RETURN_NONE;
		}

		static PyObject* Cancel(FPyScopedEditorTransaction* InSelf)
		{
			if (InSelf->PendingTransactionId != INDEX_NONE)
			{
				GEditor->CancelTransaction(InSelf->PendingTransactionId);
				InSelf->PendingTransactionId = INDEX_NONE;
			}

			Py_RETURN_NONE;
		}
	};

	static PyMethodDef PyMethods[] = {
		{ "__enter__", PyCFunctionCast(&FMethods::EnterScope), METH_NOARGS, "x.__enter__() -> self -- begin this transaction" },
		{ "__exit__", PyCFunctionCast(&FMethods::ExitScope), METH_VARARGS | METH_KEYWORDS, "x.__exit__(type, value, traceback) -- end this transaction" },
		{ "cancel", PyCFunctionCast(&FMethods::Cancel), METH_NOARGS, "x.cancel() -- cancel this transaction" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"ScopedEditorTransaction", /* tp_name */
		sizeof(FPyScopedEditorTransaction), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;

	PyType.tp_methods = PyMethods;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to create and managed a scoped editor transaction in Python";

	return PyType;
}

PyTypeObject PyScopedEditorTransactionType = InitializePyScopedEditorTransactionType();

namespace PyEditor
{

PyMethodDef PyEditorMethods[] = {
	{ nullptr, nullptr, 0, nullptr }
};

void InitializeModule()
{
#if PY_MAJOR_VERSION >= 3
	PyObject* PyModule = PyImport_AddModule("_unreal_editor");
	PyModule_AddFunctions(PyModule, PyEditorMethods);
#else	// PY_MAJOR_VERSION >= 3
	PyObject* PyModule = Py_InitModule("_unreal_editor", PyEditorMethods);
#endif	// PY_MAJOR_VERSION >= 3

	if (PyType_Ready(&PyScopedEditorTransactionType) == 0)
	{
		Py_INCREF(&PyScopedEditorTransactionType);
		PyModule_AddObject(PyModule, PyScopedEditorTransactionType.tp_name, (PyObject*)&PyScopedEditorTransactionType);
	}
}

}

#endif	// WITH_EDITOR

#endif	// WITH_PYTHON

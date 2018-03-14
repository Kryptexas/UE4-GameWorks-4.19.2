// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyConversion.h"
#include "PyUtil.h"
#include "PyPtr.h"

#include "PyWrapperObject.h"
#include "PyWrapperStruct.h"
#include "PyWrapperDelegate.h"
#include "PyWrapperName.h"
#include "PyWrapperText.h"
#include "PyWrapperArray.h"
#include "PyWrapperFixedArray.h"
#include "PyWrapperSet.h"
#include "PyWrapperMap.h"
#include "PyWrapperTypeRegistry.h"

#include "Templates/Casts.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/PropertyPortFlags.h"

#if WITH_PYTHON

#define PYCONVERSION_RETURN(RESULT, ERROR_CTX, ERROR_MSG)									\
	{																						\
		const bool bPyConversionReturnResult_Internal = (RESULT);							\
		if (!bPyConversionReturnResult_Internal && SetErrorState == ESetErrorState::Yes)	\
		{																					\
			PyUtil::SetPythonError(PyExc_TypeError, (ERROR_CTX), (ERROR_MSG));				\
		}																					\
		return bPyConversionReturnResult_Internal;											\
	}

namespace PyConversion
{

namespace Internal
{

bool NativizeStruct(PyObject* PyObj, UScriptStruct* StructType, void* StructInstance, const ESetErrorState SetErrorState)
{
	PyTypeObject* PyStructType = FPyWrapperTypeRegistry::Get().GetWrappedStructType(StructType);
	FPyWrapperStructPtr PyStruct = FPyWrapperStructPtr::StealReference(FPyWrapperStruct::CastPyObject(PyObj, PyStructType));
	if (PyStruct)
	{
		StructType->CopyScriptStruct(StructInstance, PyStruct->StructInstance);
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *PyUtil::GetFriendlyTypename(PyObj), *StructType->GetName()));
}

bool PythonizeStruct(UScriptStruct* StructType, const void* StructInstance, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	OutPyObj = (PyObject*)FPyWrapperStructFactory::Get().CreateInstance(StructType, (void*)StructInstance, FPyWrapperOwnerContext(), EPyConversionMethod::Copy);
	return true;
}

template <typename T>
bool NativizeSigned(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(PyObj))
	{
		OutVal = PyInt_AsLong(PyObj);
		return true;
	}
#endif	// PY_MAJOR_VERSION < 3

	if (PyLong_Check(PyObj))
	{
		OutVal = PyLong_AsLongLong(PyObj);
		return true;
	}

	if (PyFloat_Check(PyObj))
	{
		OutVal = PyFloat_AsDouble(PyObj);
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *PyUtil::GetFriendlyTypename(PyObj), InErrorType));
}

template <typename T>
bool NativizeUnsigned(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(PyObj))
	{
		OutVal = PyInt_AsSsize_t(PyObj);
		return true;
	}
#endif	// PY_MAJOR_VERSION < 3

	if (PyLong_Check(PyObj))
	{
		OutVal = PyLong_AsUnsignedLongLong(PyObj);
		return true;
	}

	if (PyFloat_Check(PyObj))
	{
		OutVal = PyFloat_AsDouble(PyObj);
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *PyUtil::GetFriendlyTypename(PyObj), InErrorType));
}

template <typename T>
bool PythonizeSigned(const T Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
#if PY_MAJOR_VERSION < 3
	if (Val >= LONG_MIN && Val <= LONG_MAX)
	{
		OutPyObj = PyInt_FromLong(Val);
	}
	else
#endif	// PY_MAJOR_VERSION < 3
	{
		OutPyObj = PyLong_FromLongLong(Val);
	}

	return true;
}

template <typename T>
bool PythonizeUnsigned(const T Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
#if PY_MAJOR_VERSION < 3
	if (Val <= LONG_MAX)
	{
		OutPyObj = PyInt_FromSsize_t(Val);
	}
	else
#endif	// PY_MAJOR_VERSION < 3
	{
		OutPyObj = PyLong_FromUnsignedLongLong(Val);
	}

	return true;
}

template <typename T>
bool PythonizeReal(const T Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	OutPyObj = PyFloat_FromDouble(Val);
	return true;
}

} // namespace Internal

bool Nativize(PyObject* PyObj, bool& OutVal, const ESetErrorState SetErrorState)
{
	if (PyObj == Py_True)
	{
		OutVal = true;
		return true;
	}

	if (PyObj == Py_False)
	{
		OutVal = false;
		return true;
	}

	if (PyObj == Py_None)
	{
		OutVal = false;
		return true;
	}
	
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(PyObj))
	{
		OutVal = PyInt_AsLong(PyObj) != 0;
		return true;
	}
#endif	// PY_MAJOR_VERSION < 3

	if (PyLong_Check(PyObj))
	{
		OutVal = PyLong_AsLongLong(PyObj) != 0;
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'bool'"), *PyUtil::GetFriendlyTypename(PyObj)));
}

bool Pythonize(const bool Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	if (Val)
	{
		Py_INCREF(Py_True);
		OutPyObj = Py_True;
	}
	else
	{
		Py_INCREF(Py_False);
		OutPyObj = Py_False;
	}

	return true;
}

bool Nativize(PyObject* PyObj, int8& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int8"));
}

bool Pythonize(const int8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int8"));
}

bool Nativize(PyObject* PyObj, uint8& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint8"));
}

bool Pythonize(const uint8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint8"));
}

bool Nativize(PyObject* PyObj, int16& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int16"));
}

bool Pythonize(const int16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int16"));
}

bool Nativize(PyObject* PyObj, uint16& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint16"));
}

bool Pythonize(const uint16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint16"));
}

bool Nativize(PyObject* PyObj, int32& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int32"));
}

bool Pythonize(const int32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int32"));
}

bool Nativize(PyObject* PyObj, uint32& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint32"));
}

bool Pythonize(const uint32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint32"));
}

bool Nativize(PyObject* PyObj, int64& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int64"));
}

bool Pythonize(const int64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int64"));
}

bool Nativize(PyObject* PyObj, uint64& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint64"));
}

bool Pythonize(const uint64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint64"));
}

bool Nativize(PyObject* PyObj, float& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("float"));
}

bool Pythonize(const float Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeReal(Val, OutPyObj, SetErrorState, TEXT("float"));
}

bool Nativize(PyObject* PyObj, double& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("double"));
}

bool Pythonize(const double Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeReal(Val, OutPyObj, SetErrorState, TEXT("double"));
}

bool Nativize(PyObject* PyObj, FString& OutVal, const ESetErrorState SetErrorState)
{
	if (PyUnicode_Check(PyObj))
	{
		FPyObjectPtr PyBytesObj = FPyObjectPtr::StealReference(PyUnicode_AsUTF8String(PyObj));
		if (PyBytesObj)
		{
			const char* PyUtf8Buffer = PyBytes_AsString(PyBytesObj);
			OutVal = UTF8_TO_TCHAR(PyUtf8Buffer);
			return true;
		}
	}

#if PY_MAJOR_VERSION < 3
	if (PyString_Check(PyObj))
	{
		const char* PyUtf8Buffer = PyString_AsString(PyObj);
		OutVal = UTF8_TO_TCHAR(PyUtf8Buffer);
		return true;
	}
#endif	// PY_MAJOR_VERSION < 3

	if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperNameType) == 1)
	{
		FPyWrapperName* PyWrappedName = (FPyWrapperName*)PyObj;
		OutVal = PyWrappedName->Name.ToString();
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'String'"), *PyUtil::GetFriendlyTypename(PyObj)));
}

bool Pythonize(const FString& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
#if PY_MAJOR_VERSION < 3
	if (FCString::IsPureAnsi(*Val))
	{
		OutPyObj = PyString_FromStringAndSize(TCHAR_TO_ANSI(*Val), Val.Len());
	}
	else
#endif	// PY_MAJOR_VERSION < 3
	{
		OutPyObj = PyUnicode_FromString(TCHAR_TO_UTF8(*Val));
	}

	return true;
}

bool Nativize(PyObject* PyObj, FName& OutVal, const ESetErrorState SetErrorState)
{
	if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperNameType) == 1)
	{
		FPyWrapperName* PyWrappedName = (FPyWrapperName*)PyObj;
		OutVal = PyWrappedName->Name;
		return true;
	}

	FString NameStr;
	if (Nativize(PyObj, NameStr, ESetErrorState::No))
	{
		OutVal = *NameStr;
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Name'"), *PyUtil::GetFriendlyTypename(PyObj)));
}

bool Pythonize(const FName& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	OutPyObj = (PyObject*)FPyWrapperNameFactory::Get().CreateInstance(Val);
	return true;
}

bool Nativize(PyObject* PyObj, FText& OutVal, const ESetErrorState SetErrorState)
{
	if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperTextType) == 1)
	{
		FPyWrapperText* PyWrappedName = (FPyWrapperText*)PyObj;
		OutVal = PyWrappedName->Text;
		return true;
	}

	FString TextStr;
	if (Nativize(PyObj, TextStr, ESetErrorState::No))
	{
		OutVal = FText::AsCultureInvariant(TextStr);
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Text'"), *PyUtil::GetFriendlyTypename(PyObj)));
}

bool Pythonize(const FText& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	OutPyObj = (PyObject*)FPyWrapperTextFactory::Get().CreateInstance(Val);
	return true;
}

bool Nativize(PyObject* PyObj, UObject*& OutVal, const ESetErrorState SetErrorState)
{
	return NativizeObject(PyObj, OutVal, UObject::StaticClass(), SetErrorState);
}

bool Pythonize(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return PythonizeObject(Val, OutPyObj, SetErrorState);
}

bool NativizeObject(PyObject* PyObj, UObject*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState)
{
	if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperObjectType) == 1)
	{
		FPyWrapperObject* PyWrappedObj = (FPyWrapperObject*)PyObj;
		if (!ExpectedType || PyWrappedObj->ObjectInstance->IsA(ExpectedType))
		{
			OutVal = PyWrappedObj->ObjectInstance;
			return true;
		}
	}

	if (PyObj == Py_None)
	{
		OutVal = nullptr;
		return true;
	}

	PYCONVERSION_RETURN(false, TEXT("NativizeObject"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Object*' (allowed Class type: '%s')"), *PyUtil::GetFriendlyTypename(PyObj), ExpectedType ? *ExpectedType->GetName() : TEXT("<any>")));
}

bool PythonizeObject(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	if (Val)
	{
		OutPyObj = (PyObject*)FPyWrapperObjectFactory::Get().CreateInstance(Val);
	}
	else
	{
		Py_INCREF(Py_None);
		OutPyObj = Py_None;
	}

	return true;
}

PyObject* PythonizeObject(UObject* Val, const ESetErrorState SetErrorState)
{
	PyObject* Obj = nullptr;
	PythonizeObject(Val, Obj, SetErrorState);
	return Obj;
}

bool NativizeClass(PyObject* PyObj, UClass*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState)
{
	UClass* Class = nullptr;

	if (PyType_Check(PyObj) && PyType_IsSubtype((PyTypeObject*)PyObj, &PyWrapperObjectType))
	{
		Class = FPyWrapperObjectMetaData::GetClass((PyTypeObject*)PyObj);
	}

	if (Class || NativizeObject(PyObj, (UObject*&)Class, UClass::StaticClass(), SetErrorState))
	{
		if (!Class || !ExpectedType || Class->IsChildOf(ExpectedType))
		{
			OutVal = Class;
			return true;
		}
	}

	PYCONVERSION_RETURN(false, TEXT("NativizeClass"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Class*' (allowed Class type: '%s')"), *PyUtil::GetFriendlyTypename(PyObj), ExpectedType ? *ExpectedType->GetName() : TEXT("<any>")));
}

bool PythonizeClass(UClass* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return PythonizeObject(Val, OutPyObj, SetErrorState);
}

PyObject* PythonizeClass(UClass* Val, const ESetErrorState SetErrorState)
{
	PyObject* Obj = nullptr;
	PythonizeClass(Val, Obj, SetErrorState);
	return Obj;
}

bool NativizeProperty(PyObject* PyObj, const UProperty* Prop, void* ValueAddr, const FPyWrapperOwnerContext& InChangeOwner, const ESetErrorState SetErrorState)
{
#define PYCONVERSION_PROPERTY_RETURN(RESULT) \
	PYCONVERSION_RETURN(RESULT, TEXT("NativizeProperty"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s' (%s)"), *PyUtil::GetFriendlyTypename(PyObj), *Prop->GetName(), *Prop->GetClass()->GetName()))

	if (Prop->ArrayDim > 1)
	{
		FPyWrapperFixedArrayPtr PyFixedArray = FPyWrapperFixedArrayPtr::StealReference(FPyWrapperFixedArray::CastPyObject(PyObj, &PyWrapperFixedArrayType, Prop));
		if (PyFixedArray)
		{
			const int32 ArrSize = FMath::Min(Prop->ArrayDim, PyFixedArray->ArrayProp->ArrayDim);
			for (int32 ArrIndex = 0; ArrIndex < ArrSize; ++ArrIndex)
			{
				Prop->CopySingleValue(static_cast<uint8*>(ValueAddr) + (Prop->ElementSize * ArrIndex), FPyWrapperFixedArray::GetItemPtr(PyFixedArray, ArrIndex));
			}
			return true;
		}

		PYCONVERSION_PROPERTY_RETURN(false);
	}

	return NativizeProperty_Direct(PyObj, Prop, ValueAddr, InChangeOwner, SetErrorState);

#undef PYCONVERSION_PROPERTY_RETURN
}

bool PythonizeProperty(const UProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EPyConversionMethod ConversionMethod, PyObject* OwnerPyObj, const ESetErrorState SetErrorState)
{
	if (Prop->ArrayDim > 1)
	{
		OutPyObj = (PyObject*)FPyWrapperFixedArrayFactory::Get().CreateInstance((void*)ValueAddr, Prop, FPyWrapperOwnerContext(OwnerPyObj, OwnerPyObj ? Prop->GetFName() : NAME_None), ConversionMethod);
		return true;
	}

	return PythonizeProperty_Direct(Prop, ValueAddr, OutPyObj, ConversionMethod, OwnerPyObj, SetErrorState);
}

bool NativizeProperty_Direct(PyObject* PyObj, const UProperty* Prop, void* ValueAddr, const FPyWrapperOwnerContext& InChangeOwner, const ESetErrorState SetErrorState)
{
#define PYCONVERSION_PROPERTY_RETURN(RESULT) \
	PYCONVERSION_RETURN(RESULT, TEXT("NativizeProperty"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s' (%s)"), *PyUtil::GetFriendlyTypename(PyObj), *Prop->GetName(), *Prop->GetClass()->GetName()))

#define NATIVIZE_SETTER_PROPERTY(PROPTYPE)											\
	if (auto* CastProp = Cast<PROPTYPE>(Prop))										\
	{																				\
		PROPTYPE::TCppType NewValue;												\
		const bool bResult = Nativize(PyObj, NewValue, SetErrorState);				\
		if (bResult)																\
		{																			\
			auto OldValue = CastProp->GetPropertyValue(ValueAddr);					\
			if (OldValue != NewValue)												\
			{																		\
				EmitPropertyChangeNotifications(InChangeOwner, [&]()				\
				{																	\
					CastProp->SetPropertyValue(ValueAddr, NewValue);				\
				});																	\
			}																		\
		}																			\
		PYCONVERSION_PROPERTY_RETURN(bResult);										\
	}

#define NATIVIZE_INLINE_PROPERTY(PROPTYPE)											\
	if (auto* CastProp = Cast<PROPTYPE>(Prop))										\
	{																				\
		PROPTYPE::TCppType NewValue;												\
		const bool bResult = Nativize(PyObj, NewValue, SetErrorState);				\
		if (bResult)																\
		{																			\
			auto* ValuePtr = static_cast<PROPTYPE::TCppType*>(ValueAddr);			\
			if (!CastProp->Identical(ValuePtr, &NewValue, PPF_None))				\
			{																		\
				EmitPropertyChangeNotifications(InChangeOwner, [&]()				\
				{																	\
					*ValuePtr = MoveTemp(NewValue);									\
				});																	\
			}																		\
		}																			\
		PYCONVERSION_PROPERTY_RETURN(bResult);										\
	}

	NATIVIZE_SETTER_PROPERTY(UBoolProperty);
	NATIVIZE_INLINE_PROPERTY(UInt8Property);
	NATIVIZE_INLINE_PROPERTY(UByteProperty);
	NATIVIZE_INLINE_PROPERTY(UInt16Property);
	NATIVIZE_INLINE_PROPERTY(UUInt16Property);
	NATIVIZE_INLINE_PROPERTY(UIntProperty);
	NATIVIZE_INLINE_PROPERTY(UUInt32Property);
	NATIVIZE_INLINE_PROPERTY(UInt64Property);
	NATIVIZE_INLINE_PROPERTY(UUInt64Property);
	NATIVIZE_INLINE_PROPERTY(UFloatProperty);
	NATIVIZE_INLINE_PROPERTY(UDoubleProperty);
	NATIVIZE_INLINE_PROPERTY(UStrProperty);
	NATIVIZE_INLINE_PROPERTY(UNameProperty);
	NATIVIZE_INLINE_PROPERTY(UTextProperty);

	if (auto* CastProp = Cast<UEnumProperty>(Prop))
	{
		UNumericProperty* EnumInternalProp = CastProp->GetUnderlyingProperty();
		PYCONVERSION_PROPERTY_RETURN(EnumInternalProp && NativizeProperty_Direct(PyObj, EnumInternalProp, ValueAddr, InChangeOwner, SetErrorState));
	}

	if (auto* CastProp = Cast<UClassProperty>(Prop))
	{
		UClass* NewValue = nullptr;
		const bool bResult = NativizeClass(PyObj, NewValue, CastProp->MetaClass, SetErrorState);
		if (bResult)
		{
			UObject* OldValue = CastProp->GetObjectPropertyValue(ValueAddr);
			if (OldValue != NewValue)
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->SetObjectPropertyValue(ValueAddr, NewValue);
				});
			}
		}
		PYCONVERSION_PROPERTY_RETURN(bResult);
	}

	if (auto* CastProp = Cast<UObjectPropertyBase>(Prop))
	{
		UObject* NewValue = nullptr;
		const bool bResult = NativizeObject(PyObj, NewValue, CastProp->PropertyClass, SetErrorState);
		if (bResult)
		{
			UObject* OldValue = CastProp->GetObjectPropertyValue(ValueAddr);
			if (OldValue != NewValue)
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->SetObjectPropertyValue(ValueAddr, NewValue);
				});
			}
		}
		PYCONVERSION_PROPERTY_RETURN(bResult);
	}

	if (auto* CastProp = Cast<UInterfaceProperty>(Prop))
	{
		UObject* NewValue = nullptr;
		const bool bResult = NativizeObject(PyObj, NewValue, CastProp->InterfaceClass, SetErrorState);
		if (bResult)
		{
			UObject* OldValue = CastProp->GetPropertyValue(ValueAddr).GetObject();
			if (OldValue != NewValue)
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->SetPropertyValue(ValueAddr, FScriptInterface(NewValue, NewValue ? NewValue->GetInterfaceAddress(CastProp->InterfaceClass) : nullptr));
				});
			}
		}
		PYCONVERSION_PROPERTY_RETURN(bResult);
	}

	if (auto* CastProp = Cast<UStructProperty>(Prop))
	{
		PyTypeObject* PyStructType = FPyWrapperTypeRegistry::Get().GetWrappedStructType(CastProp->Struct);
		FPyWrapperStructPtr PyStruct = FPyWrapperStructPtr::StealReference(FPyWrapperStruct::CastPyObject(PyObj, PyStructType));
		if (PyStruct)
		{
			if (!CastProp->Identical(ValueAddr, PyStruct->StructInstance, PPF_None))
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->Struct->CopyScriptStruct(ValueAddr, PyStruct->StructInstance);
				});
			}
			return true;
		}
		PYCONVERSION_PROPERTY_RETURN(false);
	}

	if (auto* CastProp = Cast<UDelegateProperty>(Prop))
	{
		PyTypeObject* PyDelegateType = FPyWrapperTypeRegistry::Get().GetWrappedDelegateType(CastProp->SignatureFunction);
		FPyWrapperDelegatePtr PyDelegate = FPyWrapperDelegatePtr::StealReference(FPyWrapperDelegate::CastPyObject(PyObj, PyDelegateType));
		if (PyDelegate)
		{
			if (!CastProp->Identical(ValueAddr, PyDelegate->DelegateInstance, PPF_None))
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->SetPropertyValue(ValueAddr, *PyDelegate->DelegateInstance);
				});
			}
			return true;
		}
		PYCONVERSION_PROPERTY_RETURN(false);
	}

	if (auto* CastProp = Cast<UMulticastDelegateProperty>(Prop))
	{
		PyTypeObject* PyDelegateType = FPyWrapperTypeRegistry::Get().GetWrappedDelegateType(CastProp->SignatureFunction);
		FPyWrapperMulticastDelegatePtr PyDelegate = FPyWrapperMulticastDelegatePtr::StealReference(FPyWrapperMulticastDelegate::CastPyObject(PyObj, PyDelegateType));
		if (PyDelegate)
		{
			if (!CastProp->Identical(ValueAddr, PyDelegate->DelegateInstance, PPF_None))
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->SetPropertyValue(ValueAddr, *PyDelegate->DelegateInstance);
				});
			}
			return true;
		}
		PYCONVERSION_PROPERTY_RETURN(false);
	}

	if (auto* CastProp = Cast<UArrayProperty>(Prop))
	{
		FPyWrapperArrayPtr PyArray = FPyWrapperArrayPtr::StealReference(FPyWrapperArray::CastPyObject(PyObj, &PyWrapperArrayType, CastProp->Inner));
		if (PyArray)
		{
			if (!CastProp->Identical(ValueAddr, PyArray->ArrayInstance, PPF_None))
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->CopyCompleteValue(ValueAddr, PyArray->ArrayInstance);
				});
			}
			return true;
		}
		PYCONVERSION_PROPERTY_RETURN(false);
	}

	if (auto* CastProp = Cast<USetProperty>(Prop))
	{
		FPyWrapperSetPtr PySet = FPyWrapperSetPtr::StealReference(FPyWrapperSet::CastPyObject(PyObj, &PyWrapperSetType, CastProp->ElementProp));
		if (PySet)
		{
			if (!CastProp->Identical(ValueAddr, PySet->SetInstance, PPF_None))
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->CopyCompleteValue(ValueAddr, PySet->SetInstance);
				});
			}
			return true;
		}
		PYCONVERSION_PROPERTY_RETURN(false);
	}

	if (auto* CastProp = Cast<UMapProperty>(Prop))
	{
		FPyWrapperMapPtr PyMap = FPyWrapperMapPtr::StealReference(FPyWrapperMap::CastPyObject(PyObj, &PyWrapperMapType, CastProp->KeyProp, CastProp->ValueProp));
		if (PyMap)
		{
			if (!CastProp->Identical(ValueAddr, PyMap->MapInstance, PPF_None))
			{
				EmitPropertyChangeNotifications(InChangeOwner, [&]()
				{
					CastProp->CopyCompleteValue(ValueAddr, PyMap->MapInstance);
				});
			}
			return true;
		}
		PYCONVERSION_PROPERTY_RETURN(false);
	}

#undef NATIVIZE_SETTER_PROPERTY
#undef NATIVIZE_INLINE_PROPERTY

	PYCONVERSION_RETURN(false, TEXT("NativizeProperty"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s' (%s). %s conversion not implemented!"), *PyUtil::GetFriendlyTypename(PyObj), *Prop->GetName(), *Prop->GetClass()->GetName(), *Prop->GetClass()->GetName()));

#undef PYCONVERSION_PROPERTY_RETURN
}

bool PythonizeProperty_Direct(const UProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EPyConversionMethod ConversionMethod, PyObject* OwnerPyObj, const ESetErrorState SetErrorState)
{
#define PYCONVERSION_PROPERTY_RETURN(RESULT) \
	PYCONVERSION_RETURN(RESULT, TEXT("PythonizeProperty"), *FString::Printf(TEXT("Cannot pythonize '%s' (%s)"), *Prop->GetName(), *Prop->GetClass()->GetName()))

	const FPyWrapperOwnerContext OwnerContext(OwnerPyObj, OwnerPyObj ? Prop->GetFName() : NAME_None);
	OwnerContext.AssertValidConversionMethod(ConversionMethod);

#define PYTHONIZE_GETTER_PROPERTY(PROPTYPE)											\
	if (auto* CastProp = Cast<PROPTYPE>(Prop))										\
	{																				\
		auto&& Value = CastProp->GetPropertyValue(ValueAddr);						\
		PYCONVERSION_PROPERTY_RETURN(Pythonize(Value, OutPyObj, SetErrorState));	\
	}

	PYTHONIZE_GETTER_PROPERTY(UBoolProperty);
	PYTHONIZE_GETTER_PROPERTY(UInt8Property);
	PYTHONIZE_GETTER_PROPERTY(UByteProperty);
	PYTHONIZE_GETTER_PROPERTY(UInt16Property);
	PYTHONIZE_GETTER_PROPERTY(UUInt16Property);
	PYTHONIZE_GETTER_PROPERTY(UIntProperty);
	PYTHONIZE_GETTER_PROPERTY(UUInt32Property);
	PYTHONIZE_GETTER_PROPERTY(UInt64Property);
	PYTHONIZE_GETTER_PROPERTY(UUInt64Property);
	PYTHONIZE_GETTER_PROPERTY(UFloatProperty);
	PYTHONIZE_GETTER_PROPERTY(UDoubleProperty);
	PYTHONIZE_GETTER_PROPERTY(UStrProperty);
	PYTHONIZE_GETTER_PROPERTY(UNameProperty);
	PYTHONIZE_GETTER_PROPERTY(UTextProperty);

	if (auto* CastProp = Cast<UEnumProperty>(Prop))
	{
		UNumericProperty* EnumInternalProp = CastProp->GetUnderlyingProperty();
		PYCONVERSION_PROPERTY_RETURN(EnumInternalProp && PythonizeProperty_Direct(EnumInternalProp, ValueAddr, OutPyObj, ConversionMethod, OwnerPyObj, SetErrorState));
	}

	if (auto* CastProp = Cast<UClassProperty>(Prop))
	{
		UClass* Value = Cast<UClass>(CastProp->GetObjectPropertyValue(ValueAddr));
		PYCONVERSION_PROPERTY_RETURN(PythonizeClass(Value, OutPyObj, SetErrorState));
	}

	if (auto* CastProp = Cast<UObjectPropertyBase>(Prop))
	{
		UObject* Value = CastProp->GetObjectPropertyValue(ValueAddr);
		PYCONVERSION_PROPERTY_RETURN(Pythonize(Value, OutPyObj, SetErrorState));
	}

	if (auto* CastProp = Cast<UInterfaceProperty>(Prop))
	{
		UObject* Value = CastProp->GetPropertyValue(ValueAddr).GetObject();
		if (Value)
		{
			OutPyObj = (PyObject*)FPyWrapperObjectFactory::Get().CreateInstance(CastProp->InterfaceClass, Value);
		}
		else
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
		}
		return true;
	}

	if (auto* CastProp = Cast<UStructProperty>(Prop))
	{
		OutPyObj = (PyObject*)FPyWrapperStructFactory::Get().CreateInstance(CastProp->Struct, (void*)ValueAddr, OwnerContext, ConversionMethod);
		return true;
	}

	if (auto* CastProp = Cast<UDelegateProperty>(Prop))
	{
		const FScriptDelegate* Value = CastProp->GetPropertyValuePtr(ValueAddr);
		OutPyObj = (PyObject*)FPyWrapperDelegateFactory::Get().CreateInstance(CastProp->SignatureFunction, (FScriptDelegate*)Value, OwnerContext, ConversionMethod);
		return true;
	}

	if (auto* CastProp = Cast<UMulticastDelegateProperty>(Prop))
	{
		const FMulticastScriptDelegate* Value = CastProp->GetPropertyValuePtr(ValueAddr);
		OutPyObj = (PyObject*)FPyWrapperMulticastDelegateFactory::Get().CreateInstance(CastProp->SignatureFunction, (FMulticastScriptDelegate*)Value, OwnerContext, ConversionMethod);
		return true;
	}

	if (auto* CastProp = Cast<UArrayProperty>(Prop))
	{
		OutPyObj = (PyObject*)FPyWrapperArrayFactory::Get().CreateInstance((void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return true;
	}

	if (auto* CastProp = Cast<USetProperty>(Prop))
	{
		OutPyObj = (PyObject*)FPyWrapperSetFactory::Get().CreateInstance((void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return true;
	}

	if (auto* CastProp = Cast<UMapProperty>(Prop))
	{
		OutPyObj = (PyObject*)FPyWrapperMapFactory::Get().CreateInstance((void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return true;
	}

#undef PYTHONIZE_GETTER_PROPERTY

	PYCONVERSION_RETURN(false, TEXT("PythonizeProperty"), *FString::Printf(TEXT("Cannot pythonize '%s' (%s). %s conversion not implemented!"), *Prop->GetName(), *Prop->GetClass()->GetName(), *Prop->GetClass()->GetName()));

#undef PYCONVERSION_PROPERTY_RETURN
}

bool NativizeProperty_InContainer(PyObject* PyObj, const UProperty* Prop, void* BaseAddr, const int32 ArrayIndex, const FPyWrapperOwnerContext& InChangeOwner, const ESetErrorState SetErrorState)
{
	check(ArrayIndex < Prop->ArrayDim);
	return NativizeProperty(PyObj, Prop, Prop->ContainerPtrToValuePtr<void>(BaseAddr, ArrayIndex), InChangeOwner, SetErrorState);
}

bool PythonizeProperty_InContainer(const UProperty* Prop, const void* BaseAddr, const int32 ArrayIndex, PyObject*& OutPyObj, const EPyConversionMethod ConversionMethod, PyObject* OwnerPyObj, const ESetErrorState SetErrorState)
{
	check(ArrayIndex < Prop->ArrayDim);
	return PythonizeProperty(Prop, Prop->ContainerPtrToValuePtr<void>(BaseAddr, ArrayIndex), OutPyObj, ConversionMethod, OwnerPyObj, SetErrorState);
}

void EmitPropertyChangeNotifications(const FPyWrapperOwnerContext& InChangeOwner, const TFunctionRef<void()>& InDoChangeFunc)
{
#if WITH_EDITOR
	auto BuildPropertyChain = [&InChangeOwner](FEditPropertyChain& OutPropertyChain) -> UObject*
	{
		auto AppendOwnerPropertyToChain = [&OutPropertyChain](const FPyWrapperOwnerContext& InOwnerContext) -> bool
		{
			const UStruct* LeafStruct = nullptr;
			if (PyObject_IsInstance(InOwnerContext.GetOwnerObject(), (PyObject*)&PyWrapperObjectType) == 1)
			{
				LeafStruct = FPyWrapperObjectMetaData::GetClass((FPyWrapperObject*)InOwnerContext.GetOwnerObject());
			}
			else if (PyObject_IsInstance(InOwnerContext.GetOwnerObject(), (PyObject*)&PyWrapperStructType) == 1)
			{
				LeafStruct = FPyWrapperStructMetaData::GetStruct((FPyWrapperStruct*)InOwnerContext.GetOwnerObject());
			}

			if (LeafStruct)
			{
				UProperty* LeafProp = LeafStruct->FindPropertyByName(InOwnerContext.GetOwnerPropertyName());
				if (LeafProp)
				{
					OutPropertyChain.AddHead(LeafProp);
					return true;
				}
			}

			return false;
		};

		FPyWrapperOwnerContext OwnerContext = InChangeOwner;
		while (OwnerContext.HasOwner() && AppendOwnerPropertyToChain(OwnerContext))
		{
			PyObject* PyObj = OwnerContext.GetOwnerObject();

			if (PyObj == InChangeOwner.GetOwnerObject())
			{
				OutPropertyChain.SetActivePropertyNode(OutPropertyChain.GetHead()->GetValue());
			}

			if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperObjectType) == 1)
			{
				// Found an object, this is the end of the chain
				OutPropertyChain.SetActiveMemberPropertyNode(OutPropertyChain.GetHead()->GetValue());
				return ((FPyWrapperObject*)PyObj)->ObjectInstance;
			}

			if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperStructType) == 1)
			{
				// Found a struct, recurse up the chain
				OwnerContext = ((FPyWrapperStruct*)PyObj)->OwnerContext;
				continue;
			}

			// Unknown object type - just bail
			break;
		}

		return nullptr;
	};

	// Build the property chain we should notify of the change
	FEditPropertyChain PropertyChain;
	UObject* ObjectToNotify = BuildPropertyChain(PropertyChain);

	// Notify that a change is about to occur
	if (ObjectToNotify)
	{
		ObjectToNotify->PreEditChange(PropertyChain);
	}

	// Perform the change
	InDoChangeFunc();

	// Notify that the change has occurred
	if (ObjectToNotify)
	{
		FPropertyChangedEvent PropertyEvent(PropertyChain.GetActiveNode()->GetValue());
		PropertyEvent.SetActiveMemberProperty(PropertyChain.GetActiveMemberNode()->GetValue());
		FPropertyChangedChainEvent PropertyChainEvent(PropertyChain, PropertyEvent);
		ObjectToNotify->PostEditChangeChainProperty(PropertyChainEvent);
	}
#else	// WITH_EDITOR
	InDoChangeFunc();
#endif	// WITH_EDITOR
}

}

#undef PYCONVERSION_RETURN

#endif	// WITH_PYTHON

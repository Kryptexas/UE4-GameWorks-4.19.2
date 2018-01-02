// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyConversionMethod.h"
#include "PyWrapperOwnerContext.h"
#include "UObject/Class.h"
#include "Templates/EnableIf.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/PointerIsConvertibleFromTo.h"

#if WITH_PYTHON

/**
 * Conversion between native and Python types.
 * @note These functions may set error state when using ESetErrorState::Yes.
 */
namespace PyConversion
{
	enum class ESetErrorState : uint8 { No, Yes };

	/** bool overload */
	bool Nativize(PyObject* PyObj, bool& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const bool Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int8 overload */
	bool Nativize(PyObject* PyObj, int8& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const int8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint8 overload */
	bool Nativize(PyObject* PyObj, uint8& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const uint8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int16 overload */
	bool Nativize(PyObject* PyObj, int16& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const int16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint16 overload */
	bool Nativize(PyObject* PyObj, uint16& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const uint16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int32 overload */
	bool Nativize(PyObject* PyObj, int32& OutVal, const ESetErrorState ErrorState = ESetErrorState::Yes);
	bool Pythonize(const int32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint32 overload */
	bool Nativize(PyObject* PyObj, uint32& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const uint32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int64 overload */
	bool Nativize(PyObject* PyObj, int64& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const int64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint64 overload */
	bool Nativize(PyObject* PyObj, uint64& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const uint64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** float overload */
	bool Nativize(PyObject* PyObj, float& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const float Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** double overload */
	bool Nativize(PyObject* PyObj, double& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const double Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FString overload */
	bool Nativize(PyObject* PyObj, FString& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const FString& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FName overload */
	bool Nativize(PyObject* PyObj, FName& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const FName& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FText overload */
	bool Nativize(PyObject* PyObj, FText& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(const FText& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** UObject overload */
	bool Nativize(PyObject* PyObj, UObject*& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool Pythonize(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for object types, including optional type checking */
	bool NativizeObject(PyObject* PyObj, UObject*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool PythonizeObject(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	PyObject* PythonizeObject(UObject* Val, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for class types, including optional type checking */
	bool NativizeClass(PyObject* PyObj, UClass*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool PythonizeClass(UClass* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	PyObject* PythonizeClass(UClass* Val, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	namespace Internal
	{
		/** Internal version of NativizeStruct/PythonizeStruct that work on the type-erased data */
		bool NativizeStruct(PyObject* PyObj, UScriptStruct* StructType, void* StructInstance, const ESetErrorState SetErrorState);
		bool PythonizeStruct(UScriptStruct* StructType, const void* StructInstance, PyObject*& OutPyObj, const ESetErrorState SetErrorState);

		/** Dummy catch-all for type conversions that aren't yet implemented */
		template <typename T, typename Spec = void>
		struct FTypeConv
		{
			static bool Nativize(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState)
			{
				ensureAlwaysMsgf(false, TEXT("Nativize not implemented for type"));
				return false;
			}
			
			static bool Pythonize(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
			{
				ensureAlwaysMsgf(false, TEXT("Pythonize not implemented for type"));
				return false;
			}
		};

		/** Override the catch-all for UObject types */
		template <typename T>
		struct FTypeConv<T, typename TEnableIf<TPointerIsConvertibleFromTo<typename TRemovePointer<T>::Type, UObject>::Value>::Type>
		{
			static bool Nativize(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState)
			{
				return PyConversion::NativizeObject(PyObj, (UObject*&)OutVal, TRemovePointer<T>::Type::StaticClass(), SetErrorState);
			}

			static bool Pythonize(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
			{
				return PyConversion::PythonizeObject((UObject*)Val, OutPyObj, SetErrorState);
			}
		};
	}

	/**
	 * Generic version of Nativize used if there is no matching overload.
	 * Used to allow conversion from object and struct types that don't match a specific override (see FTypeConv).
	 */
	template <typename T>
	bool Nativize(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::FTypeConv<T>::Nativize(PyObj, OutVal, SetErrorState);
	}

	/**
	 * Generic version of Pythonize used if there is no matching overload.
	 * Used to allow conversion from object and struct types that don't match a specific override (see FTypeConv).
	 */
	template <typename T>
	bool Pythonize(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::FTypeConv<T>::Pythonize(Val, OutPyObj, SetErrorState);
	}

	/** Generic version of Pythonize that returns a PyObject rather than a bool */
	template <typename T>
	PyObject* Pythonize(const T& Val, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		PyObject* Obj = nullptr;
		Pythonize(Val, Obj, SetErrorState);
		return Obj;
	}

	/** Conversion for known struct types */
	template <typename T>
	bool NativizeStruct(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::NativizeStruct(PyObj, TBaseStructure<T>::Get(), &OutVal, SetErrorState);
	}

	/** Conversion for known struct types */
	template <typename T>
	bool PythonizeStruct(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::PythonizeStruct(TBaseStructure<T>::Get(), &Val, OutPyObj, SetErrorState);
	}

	/** Conversion for known struct types that returns a PyObject rather than a bool */
	template <typename T>
	PyObject* PythonizeStruct(const T& Val, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		PyObject* Obj = nullptr;
		Internal::PythonizeStruct(TBaseStructure<T>::Get(), &Val, Obj, SetErrorState);
		return Obj;
	}

	/** Conversion for property instances (including fixed arrays) - ValueAddr should point to the property data */
	bool NativizeProperty(PyObject* PyObj, const UProperty* Prop, void* ValueAddr, const FPyWrapperOwnerContext& InChangeOwner = FPyWrapperOwnerContext(), const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool PythonizeProperty(const UProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EPyConversionMethod ConversionMethod = EPyConversionMethod::Copy, PyObject* OwnerPyObj = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for single property instances - ValueAddr should point to the property data */
	bool NativizeProperty_Direct(PyObject* PyObj, const UProperty* Prop, void* ValueAddr, const FPyWrapperOwnerContext& InChangeOwner = FPyWrapperOwnerContext(), const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool PythonizeProperty_Direct(const UProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EPyConversionMethod ConversionMethod = EPyConversionMethod::Copy, PyObject* OwnerPyObj = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for property instances within a structure (including fixed arrays) - BaseAddr should point to the structure data */
	bool NativizeProperty_InContainer(PyObject* PyObj, const UProperty* Prop, void* BaseAddr, const int32 ArrayIndex, const FPyWrapperOwnerContext& InChangeOwner = FPyWrapperOwnerContext(), const ESetErrorState SetErrorState = ESetErrorState::Yes);
	bool PythonizeProperty_InContainer(const UProperty* Prop, const void* BaseAddr, const int32 ArrayIndex, PyObject*& OutPyObj, const EPyConversionMethod ConversionMethod = EPyConversionMethod::Copy, PyObject* OwnerPyObj = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/**
	 * Helper function used to emit property change notifications as value changes are made
	 * This function should be called when you know the value will actually change (or know you want to emit the notifications for it changing) and will do 
	 * the pre-change notify, invoke the passed delegate to perform the change, then do the post-change notify
	 */
	void EmitPropertyChangeNotifications(const FPyWrapperOwnerContext& InChangeOwner, const TFunctionRef<void()>& InDoChangeFunc);
}

#endif	// WITH_PYTHON

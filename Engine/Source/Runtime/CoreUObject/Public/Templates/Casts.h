// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// it doesn't seem worthwhile to risk changing Cast<>/IsA() to work with interfaces at this time,
// so we'll just make sure it fails... spectacularly
#define CATCH_UNSUPPORTED_INTERFACECAST(TheType) checkAtCompileTime((TheType::StaticClassFlags & CLASS_Interface) == 0, __CASTING_TO_INTERFACE_TYPE_##TheType##_IS_UNSUPPORTED)

COREUOBJECT_API void CastLogError(const TCHAR* FromType, const TCHAR* ToType);

// Dynamically cast an object type-safely.
template< class T > T* Cast( UObject* Src )
{
	CATCH_UNSUPPORTED_INTERFACECAST(T);

	return Src && Src->IsA<T>() ? (T*)Src : NULL;
}
template< class T > FORCEINLINE T* ExactCast( UObject* Src )
{
	return Src && (Src->GetClass() == T::StaticClass()) ? (T*)Src : NULL;
}

template< class T > T* CastChecked( UObject* Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked )
{
	CATCH_UNSUPPORTED_INTERFACECAST(T);
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if( (!Src && CheckType == ECastCheckedType::NullChecked) || (Src && !Src->template IsA<T>() ) )
	{
		CastLogError(Src ? *Src->GetFullName() : TEXT("NULL"), *T::StaticClass()->GetName());
	}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	return (T*)Src;
}

// auto weak versions

// Dynamically cast an object type-safely.
template< class T, class U > T* Cast( const TAutoWeakObjectPtr<U>& Src )
{
	return Cast<T>(Src.Get());
}

template< class T, class U > FORCEINLINE T* ExactCast( const TAutoWeakObjectPtr<U>& Src )
{
	return ExactCast<T>(Src.Get());
}

template< class T, class U > T* CastChecked( const TAutoWeakObjectPtr<U>& Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked )
{
	return CastChecked<T>(Src.Get(), CheckType);
}

// FSubobjectPtr versions

template< class T > T* Cast( const FSubobjectPtr& Src )
{
	return Cast<T>(Src.Get());
}

template< class T > FORCEINLINE T* ExactCast( const FSubobjectPtr& Src )
{
	return ExactCast<T>(Src.Get());
}

template< class T > T* CastChecked( const FSubobjectPtr& Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked )
{
	return CastChecked<T>(Src.Get(), CheckType);
}


/**
 * Dynamically casts an object pointer to an interface pointer.  InterfaceType must correspond to a native interface class.
 *
 * @param	Src		the object to be casted
 *
 * @return	a pointer to the interface type specified or NULL if the object doesn't support for specified interface
 */
template<typename InterfaceType>
FORCEINLINE InterfaceType* InterfaceCast( UObject* Src )
{
	return Src != NULL
		? static_cast<InterfaceType*>(Src->GetInterfaceAddress(InterfaceType::UClassType::StaticClass()))
		: NULL;
}
template<typename InterfaceType>
TScriptInterface<InterfaceType> ScriptInterfaceCast( UObject* Src )
{
	TScriptInterface<InterfaceType> Result;
	InterfaceType* ISrc = InterfaceCast<InterfaceType>(Src);
	if ( ISrc != NULL )
	{
		Result.SetObject(Src);
		Result.SetInterface(ISrc);
	}
	return Result;
}

// Const versions of the casts
template< class T > FORCEINLINE const T* Cast         ( const UObject* Src                                                                   ) { return Cast         <T>(const_cast<UObject*>(Src)); }
template< class T > FORCEINLINE const T* ExactCast    ( const UObject* Src                                                                   ) { return ExactCast    <T>(const_cast<UObject*>(Src)); }
template< class T > FORCEINLINE const T* CastChecked  ( const UObject* Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked ) { return CastChecked  <T>(const_cast<UObject*>(Src), CheckType); }
template< class T > FORCEINLINE const T* InterfaceCast( const UObject* Src                                                                   ) { return InterfaceCast<T>(const_cast<UObject*>(Src)); }

// specializations of cast methods
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)

#define DECLARE_CAST_BY_FLAG(ClassName) \
class ClassName; \
template<> FORCEINLINE ClassName* Cast( UObject* Src ) \
{ \
	return Src && Src->GetClass()->HasAnyCastFlag(CASTCLASS_##ClassName) ? (ClassName*)Src : NULL; \
} \
template<> FORCEINLINE ClassName* CastChecked( UObject* Src, ECastCheckedType::Type CheckType ) \
{ \
	return (ClassName*)Src; \
}

#else

#define DECLARE_CAST_BY_FLAG(ClassName) \
class ClassName; \
template<> FORCEINLINE ClassName* Cast( UObject* Src ) \
{ \
	return Src && Src->GetClass()->HasAnyCastFlag(CASTCLASS_##ClassName) ? (ClassName*)Src : NULL; \
} \
template<> FORCEINLINE ClassName* CastChecked( UObject* Src, ECastCheckedType::Type CheckType ) \
{ \
	if( (!Src && CheckType == ECastCheckedType::NullChecked) || (Src && !Src->GetClass()->HasAnyCastFlag(CASTCLASS_##ClassName) ) ) \
		CastLogError(Src ? *Src->GetFullName() : TEXT("NULL"), TEXT(#ClassName)); \
	return (ClassName*)Src; \
}

#endif

DECLARE_CAST_BY_FLAG(UField)
DECLARE_CAST_BY_FLAG(UEnum)
DECLARE_CAST_BY_FLAG(UStruct)
DECLARE_CAST_BY_FLAG(UScriptStruct)
DECLARE_CAST_BY_FLAG(UClass)
DECLARE_CAST_BY_FLAG(UProperty)
DECLARE_CAST_BY_FLAG(UObjectPropertyBase)
DECLARE_CAST_BY_FLAG(UObjectProperty)
DECLARE_CAST_BY_FLAG(UWeakObjectProperty)
DECLARE_CAST_BY_FLAG(ULazyObjectProperty)
DECLARE_CAST_BY_FLAG(UAssetObjectProperty)
DECLARE_CAST_BY_FLAG(UAssetClassProperty)
DECLARE_CAST_BY_FLAG(UBoolProperty)
DECLARE_CAST_BY_FLAG(UFunction)
DECLARE_CAST_BY_FLAG(UStructProperty)
DECLARE_CAST_BY_FLAG(UByteProperty)
DECLARE_CAST_BY_FLAG(UIntProperty)
DECLARE_CAST_BY_FLAG(UFloatProperty)
DECLARE_CAST_BY_FLAG(UDoubleProperty)
DECLARE_CAST_BY_FLAG(UClassProperty)
DECLARE_CAST_BY_FLAG(UInterfaceProperty)
DECLARE_CAST_BY_FLAG(UNameProperty)
DECLARE_CAST_BY_FLAG(UStrProperty)
DECLARE_CAST_BY_FLAG(UTextProperty)
DECLARE_CAST_BY_FLAG(UArrayProperty)
DECLARE_CAST_BY_FLAG(UDelegateProperty)
DECLARE_CAST_BY_FLAG(UMulticastDelegateProperty)

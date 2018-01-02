// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "CoreMinimal.h"
#include "PyPtr.h"
#include "PyUtil.h"
#include "PyGenUtil.h"
#include "PyConversionMethod.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/Function.h"

#if WITH_PYTHON

struct FPyWrapperObject;
struct FPyWrapperStruct;
struct FPyWrapperDelegate;
struct FPyWrapperMulticastDelegate;
struct FPyWrapperName;
struct FPyWrapperText;
struct FPyWrapperArray;
struct FPyWrapperFixedArray;
struct FPyWrapperSet;
struct FPyWrapperMap;

class FPyWrapperOwnerContext;
class UPythonGeneratedClass;
class UPythonGeneratedStruct;

/** Type conversion for TPyWrapperTypeFactory */
template <typename UnrealType, typename KeyType>
struct TPyWrapperTypeFactoryConversion
{
	static KeyType UnrealTypeToKeyType(UnrealType InUnrealInstance)
	{
		return InUnrealInstance;
	}
};

/** Type conversion specialization for FPyWrapperTextFactory */
template <>
struct TPyWrapperTypeFactoryConversion<FText, FString*>
{
	static FString* UnrealTypeToKeyType(FText InUnrealInstance)
	{
		FTextDisplayStringPtr DisplayString = FTextInspector::GetSharedDisplayString(InUnrealInstance);
		return DisplayString.Get();
	}
};

/** Generic factory implementation for Python wrapped types. Types should derive from this and implement a CreateInstance and FindInstance function */
template <typename UnrealType, typename PythonType, typename KeyType = UnrealType>
class TPyWrapperTypeFactory
{
protected:
	struct FInternalKey
	{
	public:
		FInternalKey(KeyType InWrapperKey, PyTypeObject* InPyType)
			: WrapperKey(InWrapperKey)
			, PyType(InPyType)
			, Hash(HashCombine(GetTypeHash(WrapperKey), GetTypeHash(InPyType)))
		{
		}

		FORCEINLINE bool operator==(const FInternalKey& Other) const
		{
			return WrapperKey == Other.WrapperKey
				&& PyType == Other.PyType;
		}

		FORCEINLINE bool operator!=(const FInternalKey& Other) const
		{
			return !(*this == Other);
		}

		friend inline uint32 GetTypeHash(const FInternalKey& Key)
		{
			return Key.Hash;
		}

	private:
		KeyType WrapperKey;
		PyTypeObject* PyType;
		uint32 Hash;
	};

public:
	/** Map a wrapped Python instance associated with the given Unreal instance (called internally by the Python type) */
	void MapInstance(UnrealType InUnrealInstance, PythonType* InPythonInstance)
	{
		MappedInstances.Add(FInternalKey(TPyWrapperTypeFactoryConversion<UnrealType, KeyType>::UnrealTypeToKeyType(InUnrealInstance), Py_TYPE(InPythonInstance)), InPythonInstance);
	}

	/** Unmap the wrapped instance associated with the given UObject instance (called internally by the Python type) */
	void UnmapInstance(UnrealType InUnrealInstance, PyTypeObject* InWrappedPyType)
	{
		MappedInstances.Remove(FInternalKey(TPyWrapperTypeFactoryConversion<UnrealType, KeyType>::UnrealTypeToKeyType(InUnrealInstance), InWrappedPyType));
	}

protected:
	/** Callback used to initialize this type */
	typedef TFunctionRef<int(PythonType*)> FCreateInstanceInitializerFunc;

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	PythonType* FindInstanceInternal(UnrealType InUnrealInstance, PyTypeObject* InWrappedPyType) const
	{
		return MappedInstances.FindRef(FInternalKey(TPyWrapperTypeFactoryConversion<UnrealType, KeyType>::UnrealTypeToKeyType(InUnrealInstance), InWrappedPyType));
	}

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	PythonType* CreateInstanceInternal(UnrealType InUnrealInstance, PyTypeObject* InWrappedPyType, FCreateInstanceInitializerFunc CreateInstanceInitializerFunc, const bool bForceCreate = false)
	{
		if (!bForceCreate)
		{
			if (PythonType* ExistingInstance = MappedInstances.FindRef(FInternalKey(TPyWrapperTypeFactoryConversion<UnrealType, KeyType>::UnrealTypeToKeyType(InUnrealInstance), InWrappedPyType)))
			{
				Py_INCREF(ExistingInstance);
				return ExistingInstance;
			}
		}

		TPyPtr<PythonType> NewInstance = TPyPtr<PythonType>::StealReference(PythonType::New(InWrappedPyType));
		if (NewInstance)
		{
			if (CreateInstanceInitializerFunc(NewInstance) != 0)
			{
				PyUtil::LogPythonError();
				return nullptr;
			}
		}
		return NewInstance.Release();
	}

	/** Map from the internal key to wrapped Python instance */
	TMap<FInternalKey, PythonType*> MappedInstances;
};

/** Factory for wrapped UObject instances */
class FPyWrapperObjectFactory : public TPyWrapperTypeFactory<UObject*, FPyWrapperObject>
{
public:
	/** Access the singleton instance */
	static FPyWrapperObjectFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperObject* FindInstance(UObject* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperObject* CreateInstance(UObject* InUnrealInstance);

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperObject* CreateInstance(UClass* InInterfaceClass, UObject* InUnrealInstance);
};

/** Factory for wrapped UStruct instances */
class FPyWrapperStructFactory : public TPyWrapperTypeFactory<void*, FPyWrapperStruct>
{
public:
	/** Access the singleton instance */
	static FPyWrapperStructFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperStruct* FindInstance(UScriptStruct* InStruct, void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperStruct* CreateInstance(UScriptStruct* InStruct, void* InUnrealInstance, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Factory for wrapped delegate instances */
class FPyWrapperDelegateFactory : public TPyWrapperTypeFactory<FScriptDelegate*, FPyWrapperDelegate>
{
public:
	/** Access the singleton instance */
	static FPyWrapperDelegateFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperDelegate* FindInstance(const UFunction* InDelegateSignature, FScriptDelegate* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperDelegate* CreateInstance(const UFunction* InDelegateSignature, FScriptDelegate* InUnrealInstance, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Factory for wrapped multicast delegate instances */
class FPyWrapperMulticastDelegateFactory : public TPyWrapperTypeFactory<FMulticastScriptDelegate*, FPyWrapperMulticastDelegate>
{
public:
	/** Access the singleton instance */
	static FPyWrapperMulticastDelegateFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperMulticastDelegate* FindInstance(const UFunction* InDelegateSignature, FMulticastScriptDelegate* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperMulticastDelegate* CreateInstance(const UFunction* InDelegateSignature, FMulticastScriptDelegate* InUnrealInstance, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Factory for wrapped FName instances */
class FPyWrapperNameFactory : public TPyWrapperTypeFactory<FName, FPyWrapperName>
{
public:
	/** Access the singleton instance */
	static FPyWrapperNameFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperName* FindInstance(const FName InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperName* CreateInstance(const FName InUnrealInstance);
};

/** Factory for wrapped FText instances */
class FPyWrapperTextFactory : public TPyWrapperTypeFactory<FText, FPyWrapperText, FString*>
{
public:
	/** Access the singleton instance */
	static FPyWrapperTextFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperText* FindInstance(const FText InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperText* CreateInstance(const FText InUnrealInstance);
};

/** Factory for wrapped array instances */
class FPyWrapperArrayFactory : public TPyWrapperTypeFactory<void*, FPyWrapperArray>
{
public:
	/** Access the singleton instance */
	static FPyWrapperArrayFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperArray* FindInstance(void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperArray* CreateInstance(void* InUnrealInstance, const UArrayProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Factory for wrapped fixed-array instances */
class FPyWrapperFixedArrayFactory : public TPyWrapperTypeFactory<void*, FPyWrapperFixedArray>
{
public:
	/** Access the singleton instance */
	static FPyWrapperFixedArrayFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperFixedArray* FindInstance(void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperFixedArray* CreateInstance(void* InUnrealInstance, const UProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Factory for wrapped set instances */
class FPyWrapperSetFactory : public TPyWrapperTypeFactory<void*, FPyWrapperSet>
{
public:
	/** Access the singleton instance */
	static FPyWrapperSetFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperSet* FindInstance(void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperSet* CreateInstance(void* InUnrealInstance, const USetProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Factory for wrapped map instances */
class FPyWrapperMapFactory : public TPyWrapperTypeFactory<void*, FPyWrapperMap>
{
public:
	/** Access the singleton instance */
	static FPyWrapperMapFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FPyWrapperMap* FindInstance(void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FPyWrapperMap* CreateInstance(void* InUnrealInstance, const UMapProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod);
};

/** Singleton instance that handles re-instancing Python types */
class FPyWrapperTypeReinstancer
{
public:
	/** Access the singleton instance */
	static FPyWrapperTypeReinstancer& Get();

	/** Add a pending pair of classes to be re-instanced */
	void AddPendingClass(UPythonGeneratedClass* OldClass, UPythonGeneratedClass* NewClass);

	/** Add a pending pair of structs to be re-instanced */
	void AddPendingStruct(UPythonGeneratedStruct* OldStruct, UPythonGeneratedStruct* NewStruct);

	/** Process any pending re-instance requests */
	void ProcessPending();

private:
	/** Pending pairs of classes that to be re-instanced */
	TArray<TPair<UPythonGeneratedClass*, UPythonGeneratedClass*>> ClassesToReinstance;

	/** Pending pairs of structs that to be re-instanced */
	TArray<TPair<UPythonGeneratedStruct*, UPythonGeneratedStruct*>> StructsToReinstance;
};

/** Singleton instance that maps Unreal types to Python types */
class FPyWrapperTypeRegistry
{
public:
	/** Struct used to build up a list of wrapped type references that still need to be generated */
	struct FGeneratedWrappedTypeReferences
	{
		bool HasReferences() const
		{
			return (ClassReferences.Num() + StructReferences.Num() + EnumReferences.Num() + DelegateReferences.Num()) > 0;
		}

		TSet<const UClass*> ClassReferences;
		TSet<const UStruct*> StructReferences;
		TSet<const UEnum*> EnumReferences;
		TSet<const UFunction*> DelegateReferences;
	};

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnModuleDirtied, FName);

	/** Access the singleton instance */
	static FPyWrapperTypeRegistry& Get();

	/** Callback for when a Python module is dirtied */
	FOnModuleDirtied& OnModuleDirtied()
	{
		return OnModuleDirtiedDelegate;
	}

	/** Generate a wrapped type for all currently available Unreal types */
	void GenerateWrappedTypes();

	/** Generate a wrapped type for all available Unreal types in the given Unreal module */
	void GenerateWrappedTypesForModule(const FName ModuleName);

	/** Orphan the wrapped types associated with the given Unreal module (when the module is unloaded/reloaded) */
	void OrphanWrappedTypesForModule(const FName ModuleName);

	/** Generate a wrapped type for all referenced types */
	void GenerateWrappedTypesForReferences(const FGeneratedWrappedTypeReferences& InGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules);

	/** Generate notifications to (re)load the dirtied modules in Python */
	void NotifyModulesDirtied(const TSet<FName>& InDirtyModules) const;

	/** Generate a wrapped type for the given object (if it's a valid type to be wrapped) */
	PyTypeObject* GenerateWrappedTypeForObject(const UObject* InObj, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate = false);

	/** Generate a wrapped type for the given class (only if this class has not yet been registered; will also register the type) */
	PyTypeObject* GenerateWrappedClassType(const UClass* InClass, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate = false);

	/** Register the wrapped type associated with the given class name */
	void RegisterWrappedClassType(const FName ClassName, PyTypeObject* PyType);

	/** Get the best wrapped type for the given class */
	PyTypeObject* GetWrappedClassType(const UClass* InClass) const;

	/** Generate a wrapped type for the given struct (only if this struct has not yet been registered; will also register the type) */
	PyTypeObject* GenerateWrappedStructType(const UStruct* InStruct, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate = false);

	/** Register the wrapped type associated with the given struct name */
	void RegisterWrappedStructType(const FName StructName, PyTypeObject* PyType);

	/** Get the best wrapped type for the given struct */
	PyTypeObject* GetWrappedStructType(const UStruct* InStruct) const;

	/** Generate a wrapped type for the given enum (only if this enum has not yet been registered; will also register the type) */
	PyTypeObject* GenerateWrappedEnumType(const UEnum* InEnum, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate = false);

	/** Register the wrapped type associated with the given enum name */
	void RegisterWrappedEnumType(const FName EnumName, PyTypeObject* PyType);

	/** Get the best wrapped type for the given enum */
	PyTypeObject* GetWrappedEnumType(const UEnum* InEnum) const;

	/** Generate a wrapped type for the given delegate signature (only if this delegate has not yet been registered; will also register the type) */
	PyTypeObject* GenerateWrappedDelegateType(const UFunction* InDelegateSignature, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate = false);

	/** Register the wrapped type associated with the given delegate name */
	void RegisterWrappedDelegateType(const FName DelegateName, PyTypeObject* PyType);

	/** Get the best wrapped type for the given delegate signature */
	PyTypeObject* GetWrappedDelegateType(const UFunction* InDelegateSignature) const;

private:
	/** Gather any types referenced by the given property are still need to be wrapped for use in Python */
	void GatherWrappedTypesForPropertyReferences(const UProperty* InProp, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences);

	/** Map from the Unreal class name to the Python type */
	TMap<FName, PyTypeObject*> PythonWrappedClasses;

	/** Map from the Unreal struct name to the Python type */
	TMap<FName, PyTypeObject*> PythonWrappedStructs;

	/** Map from the Unreal enum name to the Python type */
	TMap<FName, PyTypeObject*> PythonWrappedEnums;

	/** Map from the Unreal delegate signature name to the Python type */
	TMap<FName, PyTypeObject*> PythonWrappedDelegates;

	/** Map from the Unreal type name to the generated Python type data */
	TMap<FName, TSharedPtr<PyGenUtil::FGeneratedWrappedType>> GeneratedWrappedTypes;

	/** Map from the Unreal module name to its generated type names */
	TMultiMap<FName, FName> GeneratedWrappedTypesForModule;

	/** Array of generated Python type data that has been orphaned (due to its owner module being unloaded/reloaded) */
	TArray<TSharedPtr<PyGenUtil::FGeneratedWrappedType>> OrphanedWrappedTypes;

	/** Callback for when a Python module is dirtied */
	FOnModuleDirtied OnModuleDirtiedDelegate;
};

#endif	// WITH_PYTHON

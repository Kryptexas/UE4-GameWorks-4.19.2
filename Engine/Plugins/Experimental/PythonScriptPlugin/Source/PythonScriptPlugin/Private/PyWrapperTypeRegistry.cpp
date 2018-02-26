// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperTypeRegistry.h"
#include "PyWrapperOwnerContext.h"
#include "PyWrapperObject.h"
#include "PyWrapperStruct.h"
#include "PyWrapperDelegate.h"
#include "PyWrapperEnum.h"
#include "PyWrapperName.h"
#include "PyWrapperText.h"
#include "PyWrapperArray.h"
#include "PyWrapperFixedArray.h"
#include "PyWrapperSet.h"
#include "PyWrapperMap.h"
#include "PyConversion.h"
#include "PyGenUtil.h"
#include "ScopedTimers.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/EnumProperty.h"

#if WITH_PYTHON

FPyWrapperObjectFactory& FPyWrapperObjectFactory::Get()
{
	static FPyWrapperObjectFactory Instance;
	return Instance;
}

FPyWrapperObject* FPyWrapperObjectFactory::FindInstance(UObject* InUnrealInstance) const
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedClassType(InUnrealInstance->GetClass());
	return FindInstanceInternal(InUnrealInstance, PyType);
}

FPyWrapperObject* FPyWrapperObjectFactory::CreateInstance(UObject* InUnrealInstance)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedClassType(InUnrealInstance->GetClass());
	return CreateInstanceInternal(InUnrealInstance, PyType, [InUnrealInstance](FPyWrapperObject* InSelf)
	{
		return FPyWrapperObject::Init(InSelf, InUnrealInstance);
	});
}

FPyWrapperObject* FPyWrapperObjectFactory::CreateInstance(UClass* InInterfaceClass, UObject* InUnrealInstance)
{
	if (!InInterfaceClass || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedClassType(InInterfaceClass);
	return CreateInstanceInternal(InUnrealInstance, PyType, [InUnrealInstance](FPyWrapperObject* InSelf)
	{
		return FPyWrapperObject::Init(InSelf, InUnrealInstance);
	});
}


FPyWrapperStructFactory& FPyWrapperStructFactory::Get()
{
	static FPyWrapperStructFactory Instance;
	return Instance;
}

FPyWrapperStruct* FPyWrapperStructFactory::FindInstance(UScriptStruct* InStruct, void* InUnrealInstance) const
{
	if (!InStruct || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedStructType(InStruct);
	return FindInstanceInternal(InUnrealInstance, PyType);
}

FPyWrapperStruct* FPyWrapperStructFactory::CreateInstance(UScriptStruct* InStruct, void* InUnrealInstance, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InStruct || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedStructType(InStruct);
	return CreateInstanceInternal(InUnrealInstance, PyType, [InStruct, InUnrealInstance, &InOwnerContext, InConversionMethod](FPyWrapperStruct* InSelf)
	{
		return FPyWrapperStruct::Init(InSelf, InOwnerContext, InStruct, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperDelegateFactory& FPyWrapperDelegateFactory::Get()
{
	static FPyWrapperDelegateFactory Instance;
	return Instance;
}

FPyWrapperDelegate* FPyWrapperDelegateFactory::FindInstance(const UFunction* InDelegateSignature, FScriptDelegate* InUnrealInstance) const
{
	if (!InDelegateSignature || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedDelegateType(InDelegateSignature);
	return FindInstanceInternal(InUnrealInstance, PyType);
}

FPyWrapperDelegate* FPyWrapperDelegateFactory::CreateInstance(const UFunction* InDelegateSignature, FScriptDelegate* InUnrealInstance, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InDelegateSignature || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedDelegateType(InDelegateSignature);
	return CreateInstanceInternal(InUnrealInstance, PyType, [InDelegateSignature, InUnrealInstance, &InOwnerContext, InConversionMethod](FPyWrapperDelegate* InSelf)
	{
		return FPyWrapperDelegate::Init(InSelf, InOwnerContext, InDelegateSignature, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperMulticastDelegateFactory& FPyWrapperMulticastDelegateFactory::Get()
{
	static FPyWrapperMulticastDelegateFactory Instance;
	return Instance;
}

FPyWrapperMulticastDelegate* FPyWrapperMulticastDelegateFactory::FindInstance(const UFunction* InDelegateSignature, FMulticastScriptDelegate* InUnrealInstance) const
{
	if (!InDelegateSignature || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedDelegateType(InDelegateSignature);
	return FindInstanceInternal(InUnrealInstance, PyType);
}

FPyWrapperMulticastDelegate* FPyWrapperMulticastDelegateFactory::CreateInstance(const UFunction* InDelegateSignature, FMulticastScriptDelegate* InUnrealInstance, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InDelegateSignature || !InUnrealInstance)
	{
		return nullptr;
	}

	PyTypeObject* PyType = FPyWrapperTypeRegistry::Get().GetWrappedDelegateType(InDelegateSignature);
	return CreateInstanceInternal(InUnrealInstance, PyType, [InDelegateSignature, InUnrealInstance, &InOwnerContext, InConversionMethod](FPyWrapperMulticastDelegate* InSelf)
	{
		return FPyWrapperMulticastDelegate::Init(InSelf, InOwnerContext, InDelegateSignature, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperNameFactory& FPyWrapperNameFactory::Get()
{
	static FPyWrapperNameFactory Instance;
	return Instance;
}

FPyWrapperName* FPyWrapperNameFactory::FindInstance(const FName InUnrealInstance) const
{
	return FindInstanceInternal(InUnrealInstance, &PyWrapperNameType);
}

FPyWrapperName* FPyWrapperNameFactory::CreateInstance(const FName InUnrealInstance)
{
	return CreateInstanceInternal(InUnrealInstance, &PyWrapperNameType, [InUnrealInstance](FPyWrapperName* InSelf)
	{
		return FPyWrapperName::Init(InSelf, InUnrealInstance);
	});
}


FPyWrapperTextFactory& FPyWrapperTextFactory::Get()
{
	static FPyWrapperTextFactory Instance;
	return Instance;
}

FPyWrapperText* FPyWrapperTextFactory::FindInstance(const FText InUnrealInstance) const
{
	return FindInstanceInternal(InUnrealInstance, &PyWrapperTextType);
}

FPyWrapperText* FPyWrapperTextFactory::CreateInstance(const FText InUnrealInstance)
{
	return CreateInstanceInternal(InUnrealInstance, &PyWrapperTextType, [InUnrealInstance](FPyWrapperText* InSelf)
	{
		return FPyWrapperText::Init(InSelf, InUnrealInstance);
	});
}


FPyWrapperArrayFactory& FPyWrapperArrayFactory::Get()
{
	static FPyWrapperArrayFactory Instance;
	return Instance;
}

FPyWrapperArray* FPyWrapperArrayFactory::FindInstance(void* InUnrealInstance) const
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return FindInstanceInternal(InUnrealInstance, &PyWrapperArrayType);
}

FPyWrapperArray* FPyWrapperArrayFactory::CreateInstance(void* InUnrealInstance, const UArrayProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &PyWrapperArrayType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FPyWrapperArray* InSelf)
	{
		return FPyWrapperArray::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperFixedArrayFactory& FPyWrapperFixedArrayFactory::Get()
{
	static FPyWrapperFixedArrayFactory Instance;
	return Instance;
}

FPyWrapperFixedArray* FPyWrapperFixedArrayFactory::FindInstance(void* InUnrealInstance) const
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return FindInstanceInternal(InUnrealInstance, &PyWrapperFixedArrayType);
}

FPyWrapperFixedArray* FPyWrapperFixedArrayFactory::CreateInstance(void* InUnrealInstance, const UProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &PyWrapperFixedArrayType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FPyWrapperFixedArray* InSelf)
	{
		return FPyWrapperFixedArray::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperSetFactory& FPyWrapperSetFactory::Get()
{
	static FPyWrapperSetFactory Instance;
	return Instance;
}

FPyWrapperSet* FPyWrapperSetFactory::FindInstance(void* InUnrealInstance) const
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return FindInstanceInternal(InUnrealInstance, &PyWrapperSetType);
}

FPyWrapperSet* FPyWrapperSetFactory::CreateInstance(void* InUnrealInstance, const USetProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &PyWrapperSetType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FPyWrapperSet* InSelf)
	{
		return FPyWrapperSet::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperMapFactory& FPyWrapperMapFactory::Get()
{
	static FPyWrapperMapFactory Instance;
	return Instance;
}

FPyWrapperMap* FPyWrapperMapFactory::FindInstance(void* InUnrealInstance) const
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return FindInstanceInternal(InUnrealInstance, &PyWrapperMapType);
}

FPyWrapperMap* FPyWrapperMapFactory::CreateInstance(void* InUnrealInstance, const UMapProperty* InProp, const FPyWrapperOwnerContext& InOwnerContext, const EPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &PyWrapperMapType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FPyWrapperMap* InSelf)
	{
		return FPyWrapperMap::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EPyConversionMethod::Copy || InConversionMethod == EPyConversionMethod::Steal);
}


FPyWrapperTypeReinstancer& FPyWrapperTypeReinstancer::Get()
{
	static FPyWrapperTypeReinstancer Instance;
	return Instance;
}

void FPyWrapperTypeReinstancer::AddPendingClass(UPythonGeneratedClass* OldClass, UPythonGeneratedClass* NewClass)
{
	ClassesToReinstance.Emplace(MakeTuple(OldClass, NewClass));
}

void FPyWrapperTypeReinstancer::AddPendingStruct(UPythonGeneratedStruct* OldStruct, UPythonGeneratedStruct* NewStruct)
{
	StructsToReinstance.Emplace(MakeTuple(OldStruct, NewStruct));
}

void FPyWrapperTypeReinstancer::ProcessPending()
{
	if (ClassesToReinstance.Num() > 0)
	{
		for (const auto& ClassToReinstancePair : ClassesToReinstance)
		{
			FCoreUObjectDelegates::RegisterClassForHotReloadReinstancingDelegate.Broadcast(ClassToReinstancePair.Key, ClassToReinstancePair.Value);
		}
		FCoreUObjectDelegates::ReinstanceHotReloadedClassesDelegate.Broadcast();

		ClassesToReinstance.Reset();
	}

	// todo: need support for re-instancing structs
}


FPyWrapperTypeRegistry& FPyWrapperTypeRegistry::Get()
{
	static FPyWrapperTypeRegistry Instance;
	return Instance;
}

void FPyWrapperTypeRegistry::GenerateWrappedTypes()
{
	FGeneratedWrappedTypeReferences GeneratedWrappedTypeReferences;
	TSet<FName> DirtyModules;

	double GenerateDuration = 0.0;
	{
		FScopedDurationTimer GenerateDurationTimer(GenerateDuration);

		ForEachObjectOfClass(UObject::StaticClass(), [this, &GeneratedWrappedTypeReferences, &DirtyModules](UObject* InObj)
		{
			GenerateWrappedTypeForObject(InObj, GeneratedWrappedTypeReferences, DirtyModules);
		});

		GenerateWrappedTypesForReferences(GeneratedWrappedTypeReferences, DirtyModules);
	}

	NotifyModulesDirtied(DirtyModules);

	UE_LOG(LogPython, Verbose, TEXT("Took %f seconds to generate and initialize Python wrapped types for the initial load."), GenerateDuration);
}

void FPyWrapperTypeRegistry::GenerateWrappedTypesForModule(const FName ModuleName)
{
	UPackage* const ModulePackage = FindPackage(nullptr, *(FString("/Script/") + ModuleName.ToString()));
	if (!ModulePackage)
	{
		return;
	}

	FGeneratedWrappedTypeReferences GeneratedWrappedTypeReferences;
	TSet<FName> DirtyModules;

	double GenerateDuration = 0.0;
	{
		FScopedDurationTimer GenerateDurationTimer(GenerateDuration);

		ForEachObjectWithOuter(ModulePackage, [this, &GeneratedWrappedTypeReferences, &DirtyModules](UObject* InObj)
		{
			GenerateWrappedTypeForObject(InObj, GeneratedWrappedTypeReferences, DirtyModules);
		});

		GenerateWrappedTypesForReferences(GeneratedWrappedTypeReferences, DirtyModules);
	}

	NotifyModulesDirtied(DirtyModules);

	UE_LOG(LogPython, Verbose, TEXT("Took %f seconds to generate and initialize Python wrapped types for '%s'."), GenerateDuration, *ModuleName.ToString());
}

void FPyWrapperTypeRegistry::OrphanWrappedTypesForModule(const FName ModuleName)
{
	TArray<FName> ModuleTypeNames;
	GeneratedWrappedTypesForModule.MultiFind(ModuleName, ModuleTypeNames, true);
	GeneratedWrappedTypesForModule.Remove(ModuleName);

	for (const FName& ModuleTypeName : ModuleTypeNames)
	{
		TSharedPtr<PyGenUtil::FGeneratedWrappedType> GeneratedWrappedType;
		if (GeneratedWrappedTypes.RemoveAndCopyValue(ModuleTypeName, GeneratedWrappedType))
		{
			OrphanedWrappedTypes.Add(GeneratedWrappedType);

			PythonWrappedClasses.Remove(ModuleTypeName);
			PythonWrappedStructs.Remove(ModuleTypeName);
			PythonWrappedEnums.Remove(ModuleTypeName);
		}
	}
}

void FPyWrapperTypeRegistry::GenerateWrappedTypesForReferences(const FGeneratedWrappedTypeReferences& InGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules)
{
	if (!InGeneratedWrappedTypeReferences.HasReferences())
	{
		return;
	}
	
	FGeneratedWrappedTypeReferences GeneratedWrappedTypeReferences;

	for (const UClass* Class : InGeneratedWrappedTypeReferences.ClassReferences)
	{
		GenerateWrappedClassType(Class, GeneratedWrappedTypeReferences, OutDirtyModules, true);
	}

	for (const UStruct* Struct : InGeneratedWrappedTypeReferences.StructReferences)
	{
		GenerateWrappedStructType(Struct, GeneratedWrappedTypeReferences, OutDirtyModules, true);
	}

	for (const UEnum* Enum : InGeneratedWrappedTypeReferences.EnumReferences)
	{
		GenerateWrappedEnumType(Enum, GeneratedWrappedTypeReferences, OutDirtyModules, true);
	}

	for (const UFunction* DelegateSignature : InGeneratedWrappedTypeReferences.DelegateReferences)
	{
		check(DelegateSignature->HasAnyFunctionFlags(FUNC_Delegate));
		GenerateWrappedDelegateType(DelegateSignature, GeneratedWrappedTypeReferences, OutDirtyModules, true);
	}

	GenerateWrappedTypesForReferences(GeneratedWrappedTypeReferences, OutDirtyModules);
}

void FPyWrapperTypeRegistry::NotifyModulesDirtied(const TSet<FName>& InDirtyModules) const
{
	for (const FName& DirtyModule : InDirtyModules)
	{
		const FString PythonModuleName = PyGenUtil::GetModulePythonName(DirtyModule, false);
		OnModuleDirtiedDelegate.Broadcast(*PythonModuleName);
	}
}

PyTypeObject* FPyWrapperTypeRegistry::GenerateWrappedTypeForObject(const UObject* InObj, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate)
{
	if (const UClass* Class = Cast<const UClass>(InObj))
	{
		return GenerateWrappedClassType(Class, OutGeneratedWrappedTypeReferences, OutDirtyModules, bForceGenerate);
	}

	if (const UStruct* Struct = Cast<const UStruct>(InObj))
	{
		return GenerateWrappedStructType(Struct, OutGeneratedWrappedTypeReferences, OutDirtyModules, bForceGenerate);
	}

	if (const UEnum* Enum = Cast<const UEnum>(InObj))
	{
		return GenerateWrappedEnumType(Enum, OutGeneratedWrappedTypeReferences, OutDirtyModules, bForceGenerate);
	}

	if (const UFunction* Func = Cast<const UFunction>(InObj))
	{
		if (Func->HasAnyFunctionFlags(FUNC_Delegate))
		{
			return GenerateWrappedDelegateType(Func, OutGeneratedWrappedTypeReferences, OutDirtyModules, bForceGenerate);
		}
	}

	return nullptr;
}

PyTypeObject* FPyWrapperTypeRegistry::GenerateWrappedClassType(const UClass* InClass, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate)
{
	// Already processed? Nothing more to do
	if (PyTypeObject* ExistingPyType = PythonWrappedClasses.FindRef(InClass->GetFName()))
	{
		return ExistingPyType;
	}

	// todo: allow generation of Blueprint generated classes
	if (PyGenUtil::IsBlueprintGeneratedClass(InClass))
	{
		return nullptr;
	}

	// Should this type be exported?
	if (!bForceGenerate && !PyGenUtil::ShouldExportClass(InClass))
	{
		return nullptr;
	}

	// Make sure the parent class is also wrapped
	PyTypeObject* SuperPyType = nullptr;
	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		SuperPyType = GenerateWrappedClassType(SuperClass, OutGeneratedWrappedTypeReferences, OutDirtyModules, true);
	}

	check(!GeneratedWrappedTypes.Contains(InClass->GetFName()));
	TSharedPtr<PyGenUtil::FGeneratedWrappedType> GeneratedWrappedType = GeneratedWrappedTypes.Add(InClass->GetFName(), MakeShared<PyGenUtil::FGeneratedWrappedType>());

	TMap<FName, FName> PythonProperties;
	TMap<FName, FName> PythonMethods;

	auto GenerateWrappedProperty = [this, InClass, &PythonProperties, &GeneratedWrappedType, &OutGeneratedWrappedTypeReferences](const UProperty* InProp)
	{
		const bool bExportPropertyToScript = PyGenUtil::ShouldExportProperty(InProp);
		const bool bExportPropertyToEditor = PyGenUtil::ShouldExportEditorOnlyProperty(InProp);

		if (bExportPropertyToScript || bExportPropertyToEditor)
		{
			GatherWrappedTypesForPropertyReferences(InProp, OutGeneratedWrappedTypeReferences);

			const PyGenUtil::FGeneratedWrappedPropertyDoc& GeneratedPropertyDoc = GeneratedWrappedType->TypePropertyDocs[GeneratedWrappedType->TypePropertyDocs.Emplace(InProp)];
			PythonProperties.Add(*GeneratedPropertyDoc.PythonPropName, InProp->GetFName());

			if (bExportPropertyToScript)
			{
				PyGenUtil::FGeneratedWrappedGetSet& GeneratedWrappedGetSet = GeneratedWrappedType->TypeGetSets[GeneratedWrappedType->TypeGetSets.AddDefaulted()];
				GeneratedWrappedGetSet.Class = (UClass*)InClass;
				GeneratedWrappedGetSet.GetSetName = PyGenUtil::TCHARToUTF8Buffer(*GeneratedPropertyDoc.PythonPropName);
				GeneratedWrappedGetSet.GetSetDoc = PyGenUtil::TCHARToUTF8Buffer(*GeneratedPropertyDoc.DocString);
				GeneratedWrappedGetSet.PropName = InProp->GetFName();
				GeneratedWrappedGetSet.GetFuncName = *InProp->GetMetaData(PyGenUtil::BlueprintGetterMetaDataKey);
				GeneratedWrappedGetSet.SetFuncName = *InProp->GetMetaData(PyGenUtil::BlueprintSetterMetaDataKey);
				GeneratedWrappedGetSet.GetCallback = (getter)&FPyWrapperObject::Getter_Impl;
				GeneratedWrappedGetSet.SetCallback = (setter)&FPyWrapperObject::Setter_Impl;
			}
		}
	};

	auto GenerateWrappedMethod = [this, InClass, &PythonMethods, &GeneratedWrappedType, &OutGeneratedWrappedTypeReferences](const UFunction* InFunc)
	{
		if (!PyGenUtil::ShouldExportFunction(InFunc))
		{
			return;
		}

		const FString PythonFunctionName = PyGenUtil::GetFunctionPythonName(InFunc);
		const bool bIsStatic = InFunc->HasAnyFunctionFlags(FUNC_Static);
		
		PythonMethods.Add(*PythonFunctionName, InFunc->GetFName());

		PyGenUtil::FGeneratedWrappedMethod& GeneratedWrappedMethod = GeneratedWrappedType->TypeMethods[GeneratedWrappedType->TypeMethods.AddDefaulted()];
		GeneratedWrappedMethod.Class = (UClass*)InClass;
		GeneratedWrappedMethod.MethodName = PyGenUtil::TCHARToUTF8Buffer(*PythonFunctionName);
		GeneratedWrappedMethod.MethodFuncName = InFunc->GetFName();

		TArray<const UProperty*, TInlineAllocator<4>> OutParams;
		if (const UProperty* ReturnProp = InFunc->GetReturnProperty())
		{
			OutParams.Add(ReturnProp);
		}

		FString FunctionDeclDocString = FString::Printf(TEXT("%s.%s("), (bIsStatic ? TEXT("X") : TEXT("x")), *PythonFunctionName);
		for (TFieldIterator<const UProperty> ParamIt(InFunc); ParamIt; ++ParamIt)
		{
			const UProperty* Param = *ParamIt;

			if (PyUtil::IsInputParameter(Param))
			{
				const FString ParamName = Param->GetName();
				const FString PythonParamName = PyGenUtil::PythonizePropertyName(ParamName, PyGenUtil::EPythonizeNameCase::Lower);
				const FName DefaultValueMetaDataKey = *FString::Printf(TEXT("CPP_Default_%s"), *ParamName);

				PyGenUtil::FGeneratedWrappedMethodParameter& GeneratedWrappedMethodParam = GeneratedWrappedMethod.MethodParams[GeneratedWrappedMethod.MethodParams.AddDefaulted()];
				GeneratedWrappedMethodParam.ParamName = PyGenUtil::TCHARToUTF8Buffer(*PythonParamName);
				GeneratedWrappedMethodParam.ParamPropName = Param->GetFName();
				if (InFunc->HasMetaData(DefaultValueMetaDataKey))
				{
					GeneratedWrappedMethodParam.ParamDefaultValue = InFunc->GetMetaData(DefaultValueMetaDataKey);
				}

				if (FunctionDeclDocString[FunctionDeclDocString.Len() - 1] != TEXT('('))
				{
					FunctionDeclDocString += TEXT(", ");
				}
				FunctionDeclDocString += PythonParamName;
				if (GeneratedWrappedMethodParam.ParamDefaultValue.IsSet())
				{
					FunctionDeclDocString += TEXT('=');
					FunctionDeclDocString += GeneratedWrappedMethodParam.ParamDefaultValue.GetValue();
				}
			}

			if (PyUtil::IsOutputParameter(Param))
			{
				OutParams.Add(Param);
			}

			GatherWrappedTypesForPropertyReferences(Param, OutGeneratedWrappedTypeReferences);
		}
		FunctionDeclDocString += TEXT(")");

		if (OutParams.Num() > 0)
		{
			FunctionDeclDocString += TEXT(" -> ");

			// If we have multiple return values and the main return value is a bool, we return None (for false) or the (potentially packed) return value without the bool (for true)
			bool bStrippedBoolReturn = false;
			if (OutParams.Num() > 1 && OutParams[0]->IsA<UBoolProperty>())
			{
				OutParams.RemoveAt(0, 1, false);
				bStrippedBoolReturn = true;
			}

			if (OutParams.Num() == 1)
			{
				FunctionDeclDocString += PyGenUtil::GetPropertyTypePythonName(OutParams[0]);
			}
			else
			{
				FunctionDeclDocString += TEXT('(');
				for (int32 OutParamIndex = 0; OutParamIndex < OutParams.Num(); ++OutParamIndex)
				{
					if (OutParamIndex > 0)
					{
						FunctionDeclDocString += TEXT(", ");
					}
					if (OutParamIndex > 0 || bStrippedBoolReturn)
					{
						FunctionDeclDocString += PyGenUtil::PythonizePropertyName(OutParams[OutParamIndex]->GetName(), PyGenUtil::EPythonizeNameCase::Lower);
						FunctionDeclDocString += TEXT('=');
					}
					FunctionDeclDocString += PyGenUtil::GetPropertyTypePythonName(OutParams[OutParamIndex]);
				}
				FunctionDeclDocString += TEXT(')');
			}

			if (bStrippedBoolReturn)
			{
				FunctionDeclDocString += TEXT(" or None");
			}
		}

		GeneratedWrappedMethod.MethodDoc = PyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("%s -- %s"), *FunctionDeclDocString, *PyGenUtil::PythonizeFunctionTooltip(PyGenUtil::GetFieldTooltip(InFunc), InFunc)));
		GeneratedWrappedMethod.MethodFlags = GeneratedWrappedMethod.MethodParams.Num() > 0 ? METH_VARARGS | METH_KEYWORDS : METH_NOARGS;
		if (bIsStatic)
		{
			GeneratedWrappedMethod.MethodFlags |= METH_CLASS;
			GeneratedWrappedMethod.MethodCallback = GeneratedWrappedMethod.MethodParams.Num() > 0 ? PyCFunctionWithClosureCast(&FPyWrapperObject::CallClassMethodWithArgs_Impl) : PyCFunctionWithClosureCast(&FPyWrapperObject::CallClassMethodNoArgs_Impl);
		}
		else
		{
			GeneratedWrappedMethod.MethodCallback = GeneratedWrappedMethod.MethodParams.Num() > 0 ? PyCFunctionWithClosureCast(&FPyWrapperObject::CallMethodWithArgs_Impl) : PyCFunctionWithClosureCast(&FPyWrapperObject::CallMethodNoArgs_Impl);
		}
	};

	GeneratedWrappedType->TypeName = PyGenUtil::TCHARToUTF8Buffer(*PyGenUtil::GetClassPythonName(InClass));

	for (TFieldIterator<const UField> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
	{
		if (const UProperty* Prop = Cast<const UProperty>(*FieldIt))
		{
			GenerateWrappedProperty(Prop);
			continue;
		}

		if (const UFunction* Func = Cast<const UFunction>(*FieldIt))
		{
			GenerateWrappedMethod(Func);
			continue;
		}
	}

	FString TypeDocString = PyGenUtil::PythonizeTooltip(PyGenUtil::GetFieldTooltip(InClass));
	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		TSharedPtr<PyGenUtil::FGeneratedWrappedType> SuperGeneratedWrappedType = GeneratedWrappedTypes.FindRef(SuperClass->GetFName());
		if (SuperGeneratedWrappedType.IsValid())
		{
			GeneratedWrappedType->TypePropertyDocs.Append(SuperGeneratedWrappedType->TypePropertyDocs);
		}
	}
	GeneratedWrappedType->TypePropertyDocs.Sort(&PyGenUtil::FGeneratedWrappedPropertyDoc::SortPredicate);
	PyGenUtil::FGeneratedWrappedPropertyDoc::AppendDocString(GeneratedWrappedType->TypePropertyDocs, TypeDocString, /*bEditorVariant*/true);
	GeneratedWrappedType->TypeDoc = PyGenUtil::TCHARToUTF8Buffer(*TypeDocString);

	GeneratedWrappedType->PyType.tp_basicsize = sizeof(FPyWrapperObject);
	GeneratedWrappedType->PyType.tp_base = SuperPyType ? SuperPyType : &PyWrapperObjectType;
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;

	TSharedPtr<FPyWrapperObjectMetaData> ObjectMetaData = MakeShared<FPyWrapperObjectMetaData>();
	ObjectMetaData->Class = (UClass*)InClass;
	ObjectMetaData->PythonProperties = MoveTemp(PythonProperties);
	ObjectMetaData->PythonMethods = MoveTemp(PythonMethods);
	GeneratedWrappedType->MetaData = ObjectMetaData;

	if (GeneratedWrappedType->Finalize())
	{
		const FName UnrealModuleName = *PyGenUtil::GetFieldModule(InClass);
		GeneratedWrappedTypesForModule.Add(UnrealModuleName, InClass->GetFName());
		OutDirtyModules.Add(UnrealModuleName);

		const FString PyModuleName = PyGenUtil::GetModulePythonName(UnrealModuleName);
		PyObject* PyModule = PyImport_AddModule(TCHAR_TO_UTF8(*PyModuleName));

		Py_INCREF(&GeneratedWrappedType->PyType);
		PyModule_AddObject(PyModule, GeneratedWrappedType->PyType.tp_name, (PyObject*)&GeneratedWrappedType->PyType);

		RegisterWrappedClassType(InClass->GetFName(), &GeneratedWrappedType->PyType);
		return &GeneratedWrappedType->PyType;
	}
	
	UE_LOG(LogPython, Fatal, TEXT("Failed to generate Python glue code for class '%s'!"), *InClass->GetName());
	return nullptr;
}

void FPyWrapperTypeRegistry::RegisterWrappedClassType(const FName ClassName, PyTypeObject* PyType)
{
	PythonWrappedClasses.Add(ClassName, PyType);
}

PyTypeObject* FPyWrapperTypeRegistry::GetWrappedClassType(const UClass* InClass) const
{
	PyTypeObject* PyType = &PyWrapperObjectType;

	for (const UClass* Class = InClass; Class; Class = Class->GetSuperClass())
	{
		if (PyTypeObject* ClassPyType = PythonWrappedClasses.FindRef(Class->GetFName()))
		{
			PyType = ClassPyType;
			break;
		}
	}

	return PyType;
}

PyTypeObject* FPyWrapperTypeRegistry::GenerateWrappedStructType(const UStruct* InStruct, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate)
{
	struct FFuncs
	{
		static int Init(FPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			const int SuperResult = PyWrapperStructType.tp_init((PyObject*)InSelf, InArgs, InKwds);
			if (SuperResult != 0)
			{
				return SuperResult;
			}

			return FPyWrapperStruct::SetPropertyValues(InSelf, InArgs, InKwds);
		}
	};

	// Already processed? Nothing more to do
	if (PyTypeObject* ExistingPyType = PythonWrappedStructs.FindRef(InStruct->GetFName()))
	{
		return ExistingPyType;
	}

	// UFunction derives from UStruct to define its arguments, but we should never process a UFunction as a UStruct for Python
	if (InStruct->IsA<UFunction>())
	{
		return nullptr;
	}

	// todo: allow generation of Blueprint generated structs
	if (PyGenUtil::IsBlueprintGeneratedStruct(InStruct))
	{
		return nullptr;
	}

	// Should this type be exported?
	if (!bForceGenerate && !PyGenUtil::ShouldExportStruct(InStruct))
	{
		return nullptr;
	}

	// Make sure the parent struct is also wrapped
	PyTypeObject* SuperPyType = nullptr;
	if (const UStruct* SuperStruct = InStruct->GetSuperStruct())
	{
		SuperPyType = GenerateWrappedStructType(SuperStruct, OutGeneratedWrappedTypeReferences, OutDirtyModules, true);
	}

	check(!GeneratedWrappedTypes.Contains(InStruct->GetFName()));
	TSharedPtr<PyGenUtil::FGeneratedWrappedType> GeneratedWrappedType = GeneratedWrappedTypes.Add(InStruct->GetFName(), MakeShared<PyGenUtil::FGeneratedWrappedType>());

	TMap<FName, FName> PythonProperties;

	auto GenerateWrappedProperty = [this, &PythonProperties, &GeneratedWrappedType, &OutGeneratedWrappedTypeReferences](const UProperty* InProp)
	{
		const bool bExportPropertyToScript = PyGenUtil::ShouldExportProperty(InProp);
		const bool bExportPropertyToEditor = PyGenUtil::ShouldExportEditorOnlyProperty(InProp);

		if (bExportPropertyToScript || bExportPropertyToEditor)
		{
			GatherWrappedTypesForPropertyReferences(InProp, OutGeneratedWrappedTypeReferences);

			const PyGenUtil::FGeneratedWrappedPropertyDoc& GeneratedPropertyDoc = GeneratedWrappedType->TypePropertyDocs[GeneratedWrappedType->TypePropertyDocs.Emplace(InProp)];
			PythonProperties.Add(*GeneratedPropertyDoc.PythonPropName, InProp->GetFName());

			if (bExportPropertyToScript)
			{
				PyGenUtil::FGeneratedWrappedGetSet& GeneratedWrappedGetSet = GeneratedWrappedType->TypeGetSets[GeneratedWrappedType->TypeGetSets.AddDefaulted()];
				GeneratedWrappedGetSet.GetSetName = PyGenUtil::TCHARToUTF8Buffer(*GeneratedPropertyDoc.PythonPropName);
				GeneratedWrappedGetSet.GetSetDoc = PyGenUtil::TCHARToUTF8Buffer(*GeneratedPropertyDoc.DocString);
				GeneratedWrappedGetSet.PropName = InProp->GetFName();
				GeneratedWrappedGetSet.GetCallback = (getter)&FPyWrapperStruct::Getter_Impl;
				GeneratedWrappedGetSet.SetCallback = (setter)&FPyWrapperStruct::Setter_Impl;
			}
		}
	};

	GeneratedWrappedType->TypeName = PyGenUtil::TCHARToUTF8Buffer(*PyGenUtil::GetStructPythonName(InStruct));

	for (TFieldIterator<const UProperty> PropIt(InStruct, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const UProperty* Prop = *PropIt;
		GenerateWrappedProperty(Prop);
	}

	FString TypeDocString = PyGenUtil::PythonizeTooltip(PyGenUtil::GetFieldTooltip(InStruct));
	if (const UStruct* SuperStruct = InStruct->GetSuperStruct())
	{
		TSharedPtr<PyGenUtil::FGeneratedWrappedType> SuperGeneratedWrappedType = GeneratedWrappedTypes.FindRef(SuperStruct->GetFName());
		if (SuperGeneratedWrappedType.IsValid())
		{
			GeneratedWrappedType->TypePropertyDocs.Append(SuperGeneratedWrappedType->TypePropertyDocs);
		}
	}
	GeneratedWrappedType->TypePropertyDocs.Sort(&PyGenUtil::FGeneratedWrappedPropertyDoc::SortPredicate);
	PyGenUtil::FGeneratedWrappedPropertyDoc::AppendDocString(GeneratedWrappedType->TypePropertyDocs, TypeDocString, /*bEditorVariant*/true);
	GeneratedWrappedType->TypeDoc = PyGenUtil::TCHARToUTF8Buffer(*TypeDocString);

	GeneratedWrappedType->PyType.tp_basicsize = sizeof(FPyWrapperStruct);
	GeneratedWrappedType->PyType.tp_base = SuperPyType ? SuperPyType : &PyWrapperStructType;
	GeneratedWrappedType->PyType.tp_init = (initproc)&FFuncs::Init;
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;

	TSharedPtr<FPyWrapperStructMetaData> StructMetaData = MakeShared<FPyWrapperStructMetaData>();
	StructMetaData->Struct = (UStruct*)InStruct;
	StructMetaData->PythonProperties = MoveTemp(PythonProperties);
	// Build a complete list of init params for this struct (parent struct params + our params)
	if (SuperPyType)
	{
		FPyWrapperStructMetaData* SuperMetaData = FPyWrapperStructMetaData::GetMetaData(SuperPyType);
		if (SuperMetaData)
		{
			StructMetaData->InitParams = SuperMetaData->InitParams;
		}
	}
	for (const PyGenUtil::FGeneratedWrappedGetSet& GeneratedWrappedGetSet : GeneratedWrappedType->TypeGetSets)
	{
		PyGenUtil::FGeneratedWrappedMethodParameter& GeneratedInitParam = StructMetaData->InitParams[StructMetaData->InitParams.AddDefaulted()];
		GeneratedInitParam.ParamName = GeneratedWrappedGetSet.GetSetName;
		GeneratedInitParam.ParamPropName = GeneratedWrappedGetSet.PropName;
		GeneratedInitParam.ParamDefaultValue = FString();
	}
	GeneratedWrappedType->MetaData = StructMetaData;

	if (GeneratedWrappedType->Finalize())
	{
		const FName UnrealModuleName = *PyGenUtil::GetFieldModule(InStruct);
		GeneratedWrappedTypesForModule.Add(UnrealModuleName, InStruct->GetFName());
		OutDirtyModules.Add(UnrealModuleName);

		const FString PyModuleName = PyGenUtil::GetModulePythonName(UnrealModuleName);
		PyObject* PyModule = PyImport_AddModule(TCHAR_TO_UTF8(*PyModuleName));

		Py_INCREF(&GeneratedWrappedType->PyType);
		PyModule_AddObject(PyModule, GeneratedWrappedType->PyType.tp_name, (PyObject*)&GeneratedWrappedType->PyType);

		RegisterWrappedStructType(InStruct->GetFName(), &GeneratedWrappedType->PyType);
		return &GeneratedWrappedType->PyType;
	}

	UE_LOG(LogPython, Fatal, TEXT("Failed to generate Python glue code for struct '%s'!"), *InStruct->GetName());
	return nullptr;
}

void FPyWrapperTypeRegistry::RegisterWrappedStructType(const FName StructName, PyTypeObject* PyType)
{
	PythonWrappedStructs.Add(StructName, PyType);
}

PyTypeObject* FPyWrapperTypeRegistry::GetWrappedStructType(const UStruct* InStruct) const
{
	PyTypeObject* PyType = &PyWrapperStructType;

	for (const UStruct* Struct = InStruct; Struct; Struct = Struct->GetSuperStruct())
	{
		if (PyTypeObject* StructPyType = PythonWrappedStructs.FindRef(Struct->GetFName()))
		{
			PyType = StructPyType;
			break;
		}
	}

	return PyType;
}

PyTypeObject* FPyWrapperTypeRegistry::GenerateWrappedEnumType(const UEnum* InEnum, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate)
{
	// Already processed? Nothing more to do
	if (PyTypeObject* ExistingPyType = PythonWrappedEnums.FindRef(InEnum->GetFName()))
	{
		return ExistingPyType;
	}

	// todo: allow generation of Blueprint generated enums
	if (PyGenUtil::IsBlueprintGeneratedEnum(InEnum))
	{
		return nullptr;
	}

	// Should this type be exported?
	if (!bForceGenerate && !PyGenUtil::ShouldExportEnum(InEnum))
	{
		return nullptr;
	}

	check(!GeneratedWrappedTypes.Contains(InEnum->GetFName()));
	TSharedPtr<PyGenUtil::FGeneratedWrappedType> GeneratedWrappedType = GeneratedWrappedTypes.Add(InEnum->GetFName(), MakeShared<PyGenUtil::FGeneratedWrappedType>());

	GeneratedWrappedType->TypeName = PyGenUtil::TCHARToUTF8Buffer(*PyGenUtil::GetEnumPythonName(InEnum));
	GeneratedWrappedType->TypeDoc = PyGenUtil::TCHARToUTF8Buffer(*PyGenUtil::PythonizeTooltip(PyGenUtil::GetFieldTooltip(InEnum)));

	GeneratedWrappedType->PyType.tp_basicsize = sizeof(FPyWrapperEnum);
	GeneratedWrappedType->PyType.tp_base = &PyWrapperEnumType;
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT;

	TSharedPtr<FPyWrapperEnumMetaData> EnumMetaData = MakeShared<FPyWrapperEnumMetaData>();
	EnumMetaData->Enum = (UEnum*)InEnum;
	GeneratedWrappedType->MetaData = EnumMetaData;

	if (GeneratedWrappedType->Finalize())
	{
		// Register the enum values
		for (int32 EnumEntryIndex = 0; EnumEntryIndex < InEnum->NumEnums() - 1; ++EnumEntryIndex)
		{
			if (PyGenUtil::ShouldExportEnumEntry(InEnum, EnumEntryIndex))
			{
				const int64 EnumEntryValue = InEnum->GetValueByIndex(EnumEntryIndex);
				const FString EnumEntryShortName = InEnum->GetNameStringByIndex(EnumEntryIndex);
				const FString EnumEntryShortPythonName = PyGenUtil::PythonizeName(EnumEntryShortName, PyGenUtil::EPythonizeNameCase::Upper);
				const FString EnumEntryDoc = PyGenUtil::PythonizeTooltip(PyGenUtil::GetEnumEntryTooltip(InEnum, EnumEntryIndex));

				FPyWrapperEnum::SetEnumEntryValue(&GeneratedWrappedType->PyType, EnumEntryValue, TCHAR_TO_UTF8(*EnumEntryShortPythonName), TCHAR_TO_UTF8(*EnumEntryDoc));
			}
		}

		const FName UnrealModuleName = *PyGenUtil::GetFieldModule(InEnum);
		GeneratedWrappedTypesForModule.Add(UnrealModuleName, InEnum->GetFName());
		OutDirtyModules.Add(UnrealModuleName);

		const FString PyModuleName = PyGenUtil::GetModulePythonName(UnrealModuleName);
		PyObject* PyModule = PyImport_AddModule(TCHAR_TO_UTF8(*PyModuleName));

		Py_INCREF(&GeneratedWrappedType->PyType);
		PyModule_AddObject(PyModule, GeneratedWrappedType->PyType.tp_name, (PyObject*)&GeneratedWrappedType->PyType);

		RegisterWrappedEnumType(InEnum->GetFName(), &GeneratedWrappedType->PyType);
		return &GeneratedWrappedType->PyType;
	}

	UE_LOG(LogPython, Fatal, TEXT("Failed to generate Python glue code for enum '%s'!"), *InEnum->GetName());
	return nullptr;
}

void FPyWrapperTypeRegistry::RegisterWrappedEnumType(const FName EnumName, PyTypeObject* PyType)
{
	PythonWrappedEnums.Add(EnumName, PyType);
}

PyTypeObject* FPyWrapperTypeRegistry::GetWrappedEnumType(const UEnum* InEnum) const
{
	PyTypeObject* PyType = &PyWrapperEnumType;

	if (PyTypeObject* EnumPyType = PythonWrappedEnums.FindRef(InEnum->GetFName()))
	{
		PyType = EnumPyType;
	}

	return PyType;
}

PyTypeObject* FPyWrapperTypeRegistry::GenerateWrappedDelegateType(const UFunction* InDelegateSignature, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences, TSet<FName>& OutDirtyModules, const bool bForceGenerate)
{
	// Already processed? Nothing more to do
	if (PyTypeObject* ExistingPyType = PythonWrappedDelegates.FindRef(InDelegateSignature->GetFName()))
	{
		return ExistingPyType;
	}

	// Is this actually a delegate signature?
	if (!InDelegateSignature->HasAnyFunctionFlags(FUNC_Delegate))
	{
		return nullptr;
	}

	check(!GeneratedWrappedTypes.Contains(InDelegateSignature->GetFName()));
	TSharedPtr<PyGenUtil::FGeneratedWrappedType> GeneratedWrappedType = GeneratedWrappedTypes.Add(InDelegateSignature->GetFName(), MakeShared<PyGenUtil::FGeneratedWrappedType>());

	for (TFieldIterator<const UProperty> ParamIt(InDelegateSignature); ParamIt; ++ParamIt)
	{
		const UProperty* Param = *ParamIt;
		GatherWrappedTypesForPropertyReferences(Param, OutGeneratedWrappedTypeReferences);
	}

	const FString DelegateBaseTypename = PyGenUtil::GetDelegatePythonName(InDelegateSignature);
	GeneratedWrappedType->TypeName = PyGenUtil::TCHARToUTF8Buffer(*DelegateBaseTypename);
	GeneratedWrappedType->TypeDoc = PyGenUtil::TCHARToUTF8Buffer(*PyGenUtil::PythonizeFunctionTooltip(PyGenUtil::GetFieldTooltip(InDelegateSignature), InDelegateSignature));

	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT;

	if (InDelegateSignature->HasAnyFunctionFlags(FUNC_MulticastDelegate))
	{
		GeneratedWrappedType->PyType.tp_basicsize = sizeof(FPyWrapperMulticastDelegate);
		GeneratedWrappedType->PyType.tp_base = &PyWrapperMulticastDelegateType;

		TSharedPtr<FPyWrapperMulticastDelegateMetaData> DelegateMetaData = MakeShared<FPyWrapperMulticastDelegateMetaData>();
		DelegateMetaData->DelegateSignature = InDelegateSignature;
		GeneratedWrappedType->MetaData = DelegateMetaData;
	}
	else
	{
		GeneratedWrappedType->PyType.tp_basicsize = sizeof(FPyWrapperDelegate);
		GeneratedWrappedType->PyType.tp_base = &PyWrapperDelegateType;

		TSharedPtr<FPyWrapperDelegateMetaData> DelegateMetaData = MakeShared<FPyWrapperDelegateMetaData>();
		DelegateMetaData->DelegateSignature = InDelegateSignature;
		GeneratedWrappedType->MetaData = DelegateMetaData;
	}

	if (GeneratedWrappedType->Finalize())
	{
		const FName UnrealModuleName = *PyGenUtil::GetFieldModule(InDelegateSignature);
		GeneratedWrappedTypesForModule.Add(UnrealModuleName, InDelegateSignature->GetFName());
		OutDirtyModules.Add(UnrealModuleName);

		const FString PyModuleName = PyGenUtil::GetModulePythonName(UnrealModuleName);
		PyObject* PyModule = PyImport_AddModule(TCHAR_TO_UTF8(*PyModuleName));

		Py_INCREF(&GeneratedWrappedType->PyType);
		PyModule_AddObject(PyModule, GeneratedWrappedType->PyType.tp_name, (PyObject*)&GeneratedWrappedType->PyType);

		RegisterWrappedDelegateType(InDelegateSignature->GetFName(), &GeneratedWrappedType->PyType);
		return &GeneratedWrappedType->PyType;
	}

	UE_LOG(LogPython, Fatal, TEXT("Failed to generate Python glue code for delegate '%s'!"), *InDelegateSignature->GetName());
	return nullptr;
}

void FPyWrapperTypeRegistry::RegisterWrappedDelegateType(const FName DelegateName, PyTypeObject* PyType)
{
	PythonWrappedDelegates.Add(DelegateName, PyType);
}

PyTypeObject* FPyWrapperTypeRegistry::GetWrappedDelegateType(const UFunction* InDelegateSignature) const
{
	PyTypeObject* PyType = InDelegateSignature->HasAnyFunctionFlags(FUNC_MulticastDelegate) ? &PyWrapperMulticastDelegateType : &PyWrapperDelegateType;

	if (PyTypeObject* DelegatePyType = PythonWrappedDelegates.FindRef(InDelegateSignature->GetFName()))
	{
		PyType = DelegatePyType;
	}

	return PyType;
}

void FPyWrapperTypeRegistry::GatherWrappedTypesForPropertyReferences(const UProperty* InProp, FGeneratedWrappedTypeReferences& OutGeneratedWrappedTypeReferences)
{
	if (const UObjectProperty* ObjProp = Cast<const UObjectProperty>(InProp))
	{
		if (ObjProp->PropertyClass && !PythonWrappedClasses.Contains(ObjProp->PropertyClass->GetFName()))
		{
			OutGeneratedWrappedTypeReferences.ClassReferences.Add(ObjProp->PropertyClass);
		}
		return;
	}

	if (const UStructProperty* StructProp = Cast<const UStructProperty>(InProp))
	{
		if (!PythonWrappedStructs.Contains(StructProp->Struct->GetFName()))
		{
			OutGeneratedWrappedTypeReferences.StructReferences.Add(StructProp->Struct);
		}
		return;
	}

	if (const UEnumProperty* EnumProp = Cast<const UEnumProperty>(InProp))
	{
		if (!PythonWrappedStructs.Contains(EnumProp->GetEnum()->GetFName()))
		{
			OutGeneratedWrappedTypeReferences.EnumReferences.Add(EnumProp->GetEnum());
		}
		return;
	}

	if (const UDelegateProperty* DelegateProp = Cast<const UDelegateProperty>(InProp))
	{
		if (!PythonWrappedStructs.Contains(DelegateProp->SignatureFunction->GetFName()))
		{
			OutGeneratedWrappedTypeReferences.DelegateReferences.Add(DelegateProp->SignatureFunction);
		}
		return;
	}

	if (const UMulticastDelegateProperty* DelegateProp = Cast<const UMulticastDelegateProperty>(InProp))
	{
		if (!PythonWrappedStructs.Contains(DelegateProp->SignatureFunction->GetFName()))
		{
			OutGeneratedWrappedTypeReferences.DelegateReferences.Add(DelegateProp->SignatureFunction);
		}
		return;
	}

	if (const UArrayProperty* ArrayProp = Cast<const UArrayProperty>(InProp))
	{
		GatherWrappedTypesForPropertyReferences(ArrayProp->Inner, OutGeneratedWrappedTypeReferences);
		return;
	}

	if (const USetProperty* SetProp = Cast<const USetProperty>(InProp))
	{
		GatherWrappedTypesForPropertyReferences(SetProp->ElementProp, OutGeneratedWrappedTypeReferences);
		return;
	}

	if (const UMapProperty* MapProp = Cast<const UMapProperty>(InProp))
	{
		GatherWrappedTypesForPropertyReferences(MapProp->KeyProp, OutGeneratedWrappedTypeReferences);
		GatherWrappedTypesForPropertyReferences(MapProp->ValueProp, OutGeneratedWrappedTypeReferences);
		return;
	}
}

#endif	// WITH_PYTHON

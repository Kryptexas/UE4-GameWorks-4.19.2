// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "BlueprintCompilerCppBackendUtils.h"
#include "Animation/AnimClassData.h"
#include "Animation/AnimNodeBase.h"

void FBackendHelperAnim::AddHeaders(FEmitterLocalContext& Context)
{
	if (Cast<UAnimBlueprintGeneratedClass>(Context.GetCurrentlyGeneratedClass()))
	{
		Context.Header.AddLine(TEXT("#include \"Animation/AnimClassData.h\""));
		Context.Body.AddLine(TEXT("#include \"Animation/BlendProfile.h\""));
	}
}

void FBackendHelperAnim::CreateAnimClassData(FEmitterLocalContext& Context)
{
	if (auto AnimClass = Cast<UAnimBlueprintGeneratedClass>(Context.GetCurrentlyGeneratedClass()))
	{
		const FString LocalNativeName = Context.GenerateUniqueLocalName();
		Context.AddLine(FString::Printf(TEXT("auto %s = NewObject<UAnimClassData>(InDynamicClass, TEXT(\"AnimClassData\"));"), *LocalNativeName));

		auto AnimClassData = NewObject<UAnimClassData>(GetTransientPackage(), TEXT("AnimClassData"));
		AnimClassData->CopyFrom(AnimClass);

		auto ObjectArchetype = AnimClassData->GetArchetype();
		for (auto Property : TFieldRange<const UProperty>(UAnimClassData::StaticClass()))
		{
			FEmitDefaultValueHelper::OuterGenerate(Context, Property, LocalNativeName
				, reinterpret_cast<const uint8*>(AnimClassData)
				, reinterpret_cast<const uint8*>(ObjectArchetype)
				, FEmitDefaultValueHelper::EPropertyAccessOperator::Pointer);
		}

		Context.AddLine(FString::Printf(TEXT("InDynamicClass->%s = %s;"), GET_MEMBER_NAME_STRING_CHECKED(UDynamicClass, AnimClassImplementation), *LocalNativeName));
	}
}

bool FBackendHelperAnim::ShouldAddAnimNodeInitializationFunctionCall(FEmitterLocalContext& Context, const UProperty* InProperty)
{
	if(const UStructProperty* StructProperty = Cast<const UStructProperty>(InProperty))
	{
		if(StructProperty->Struct->IsChildOf(FAnimNode_Base::StaticStruct()))
		{
			return true;
		}
	}

	return false;
}

void FBackendHelperAnim::AddAnimNodeInitializationFunctionCall(FEmitterLocalContext& Context, const UProperty* InProperty)
{
	if(const UStructProperty* StructProperty = Cast<const UStructProperty>(InProperty))
	{
		if(StructProperty->Struct->IsChildOf(FAnimNode_Base::StaticStruct()))
		{
			Context.Body.AddLine(FString::Printf(TEXT("__InitAnimNode__%s();"), *StructProperty->GetName()));
		}
	}
}

void FBackendHelperAnim::AddAnimNodeInitializationFunction(FEmitterLocalContext& Context, const FString& InCppClassName, const UProperty* InProperty, bool bInNewProperty, UObject* InCDO, UObject* InParentCDO)
{
	if(const UStructProperty* StructProperty = Cast<const UStructProperty>(InProperty))
	{
		if(StructProperty->Struct->IsChildOf(FAnimNode_Base::StaticStruct()))
		{
			Context.Header.AddLine(FString::Printf(TEXT("void __InitAnimNode__%s();"), *StructProperty->GetName()));

			Context.Body.AddLine(FString::Printf(TEXT("void %s::__InitAnimNode__%s()"), *InCppClassName, *StructProperty->GetName()));
			Context.Body.AddLine(TEXT("{"));
			Context.Body.IncreaseIndent();

			FEmitDefaultValueHelper::OuterGenerate(Context, InProperty, TEXT(""), reinterpret_cast<const uint8*>(InCDO), bInNewProperty ? nullptr : reinterpret_cast<const uint8*>(InParentCDO), FEmitDefaultValueHelper::EPropertyAccessOperator::None, true);

			Context.Body.DecreaseIndent();
			Context.Body.AddLine(TEXT("}"));
		}
	}
}

void FBackendHelperAnim::AddAllAnimNodesInitializationFunction(FEmitterLocalContext& Context, const FString& InCppClassName, const TArray<const UProperty*>& InAnimProperties)
{
	Context.Header.AddLine(TEXT("void __InitAllAnimNodes();"));

	Context.Body.AddLine(FString::Printf(TEXT("void %s::__InitAllAnimNodes()"), *InCppClassName));
	Context.Body.AddLine(TEXT("{"));
	Context.Body.IncreaseIndent();

	for(const UProperty* Property : InAnimProperties)
	{
		Context.Body.AddLine(FString::Printf(TEXT("__InitAnimNode__%s();"), *Property->GetName()));
	}

	Context.Body.DecreaseIndent();
	Context.Body.AddLine(TEXT("}"));
}

void FBackendHelperAnim::AddAllAnimNodesInitializationFunctionCall(FEmitterLocalContext& Context)
{
	Context.Body.AddLine(TEXT("__InitAllAnimNodes();"));
}
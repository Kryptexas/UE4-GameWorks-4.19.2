// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AbilitySystemEditorPrivatePCH.h"
#include "AbilityTask.h"
#include "K2Node_LatentAbilityCall.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_LatentAbilityCall::UK2Node_LatentAbilityCall(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyActivateFunctionName = GET_FUNCTION_NAME_CHECKED(UAbilityTask, Activate);
}

void UK2Node_LatentAbilityCall::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	EGraphType GraphType = K2Schema->GetGraphType(ContextMenuBuilder.CurrentGraph);
	const bool bAllowLatentFuncs = (GraphType == GT_Ubergraph || GraphType == GT_Macro);

	if (bAllowLatentFuncs)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* TestClass = *ClassIt;
			if (TestClass->IsChildOf(UAbilityTask::StaticClass()) && !TestClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
			{
				for (TFieldIterator<UFunction> FuncIt(TestClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
				{
					// See if the function is a static factory method for online proxies
					UFunction* CandidateFunction = *FuncIt;

					UObjectProperty* ReturnProperty = Cast<UObjectProperty>(CandidateFunction->GetReturnProperty());
					const bool bValidReturnType = (ReturnProperty != nullptr) && (ReturnProperty->PropertyClass != nullptr) && (ReturnProperty->PropertyClass->IsChildOf(UAbilityTask::StaticClass()));

					if (CandidateFunction->HasAllFunctionFlags(FUNC_Static) && bValidReturnType)
					{
						// Create a node template for this factory method
						UK2Node_LatentAbilityCall* NodeTemplate = NewObject<UK2Node_LatentAbilityCall>(ContextMenuBuilder.OwnerOfTemporaries);
						NodeTemplate->ProxyFactoryFunctionName = CandidateFunction->GetFName();
						NodeTemplate->ProxyFactoryClass = TestClass;
						NodeTemplate->ProxyClass = ReturnProperty->PropertyClass;

						CreateDefaultMenuEntry(NodeTemplate, ContextMenuBuilder);
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE



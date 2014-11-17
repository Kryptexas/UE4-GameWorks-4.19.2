// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "OnlineBlueprintCallProxyBase.h"
#include "K2Node_LatentOnlineCall.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_LatentOnlineCall::UK2Node_LatentOnlineCall(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyActivateFunctionName = GET_FUNCTION_NAME_CHECKED(UOnlineBlueprintCallProxyBase, Activate);
}

void UK2Node_LatentOnlineCall::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	const bool bAllowLatentFuncs = (K2Schema->GetGraphType(ContextMenuBuilder.CurrentGraph) == GT_Ubergraph);


	if (bAllowLatentFuncs)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* TestClass = *ClassIt;
			if (TestClass->IsChildOf(UOnlineBlueprintCallProxyBase::StaticClass()) && !TestClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
			{
				for (TFieldIterator<UFunction> FuncIt(TestClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
				{
					// See if the function is a static factory method for online proxies
					UFunction* CandidateFunction = *FuncIt;

					UObjectProperty* ReturnProperty = Cast<UObjectProperty>(CandidateFunction->GetReturnProperty());
					const bool bValidReturnType = (ReturnProperty != nullptr) && (ReturnProperty->PropertyClass != nullptr) && (ReturnProperty->PropertyClass->IsChildOf(UOnlineBlueprintCallProxyBase::StaticClass()));

					if (CandidateFunction->HasAllFunctionFlags(FUNC_Static) && bValidReturnType)
					{
						// Create a node template for this factory method
						UK2Node_LatentOnlineCall* NodeTemplate = NewObject<UK2Node_LatentOnlineCall>(ContextMenuBuilder.OwnerOfTemporaries);
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

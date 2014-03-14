// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "BlueprintGraphPrivatePCH.h"

#include "K2Node_BaseMCDelegate.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "CompilerResultsLog.h"
#include "DelegateNodeHandlers.h"

struct FK2Node_BaseMCDelegateHelper
{
	static FString DelegatePinName;
};
FString FK2Node_BaseMCDelegateHelper::DelegatePinName(TEXT("Delegate"));

/////// UK2Node_BaseMCDelegate ///////////

UK2Node_BaseMCDelegate::UK2Node_BaseMCDelegate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_BaseMCDelegate::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);
	if(UProperty* Property = GetProperty())
	{
		if(!Property->HasAllPropertyFlags(CPF_BlueprintAssignable))
		{
			MessageLog.Error(*FString::Printf(*NSLOCTEXT("K2Node", "BaseMCDelegateNotAssignable", "Event Dispatcher is not 'BlueprintAssignable' @@").ToString()), this);
		}
	}
}

UK2Node::ERedirectType UK2Node_BaseMCDelegate::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	ERedirectType OrginalResult = Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if ((ERedirectType::ERedirectType_None == OrginalResult) && K2Schema && NewPin && OldPin)
	{
		if ((OldPin->PinType.PinCategory == K2Schema->PC_Delegate) &&
			(NewPin->PinType.PinCategory == K2Schema->PC_Delegate) &&
			(FCString::Stricmp(*(NewPin->PinName), *(OldPin->PinName)) == 0))
		{
			return ERedirectType_Name;
		}
	}
	return OrginalResult;
}

void UK2Node_BaseMCDelegate::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	UEdGraphPin* SelfPin = NULL;
	if (DelegateReference.IsSelfContext())
	{
		SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, K2Schema->PSC_Self, NULL, false, false, K2Schema->PN_Self);
	}
	else
	{
		// Allow redirects on the target node if necessary.
		DelegateReference.ResolveMember<UMulticastDelegateProperty>((UClass*)NULL);
		SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), DelegateReference.GetMemberParentClass(this), false, false, K2Schema->PN_Self);
	}

	if(SelfPin)
	{
		SelfPin->PinFriendlyName = NSLOCTEXT("K2Node", "BaseMCDelegateSelfPinName", "Target").ToString();
	}
}

UFunction* UK2Node_BaseMCDelegate::GetDelegateSignature(bool bForceNotFromSkelClass) const
{
	FMemberReference ReferenceToUse;

	if(!bForceNotFromSkelClass)
	{
		ReferenceToUse = DelegateReference;
	}
	else
	{
		UClass* OwnerClass = DelegateReference.GetMemberParentClass(this);
		FGuid DelegateGuid;
		if (OwnerClass)
		{
			UBlueprint::GetGuidFromClassByFieldName<UFunction>(OwnerClass, DelegateReference.GetMemberName(), DelegateGuid);
		}
		ReferenceToUse.SetDirect(DelegateReference.GetMemberName(), DelegateGuid, OwnerClass ? OwnerClass->GetAuthoritativeClass() : NULL, false);
	}

	UMulticastDelegateProperty* DelegateProperty = ReferenceToUse.ResolveMember<UMulticastDelegateProperty>(this);
	return (DelegateProperty != NULL) ? DelegateProperty->SignatureFunction : NULL;
}

UEdGraphPin* UK2Node_BaseMCDelegate::GetDelegatePin() const
{
	return FindPin(FK2Node_BaseMCDelegateHelper::DelegatePinName);
}

FString UK2Node_BaseMCDelegate::GetDocumentationLink() const
{
	UClass* ParentClass = NULL;
	if (DelegateReference.IsSelfContext())
	{
		if (HasValidBlueprint())
		{
			UField* Delegate = FindField<UField>(GetBlueprint()->GeneratedClass, DelegateReference.GetMemberName());
			if (Delegate != NULL)
			{
				ParentClass = Delegate->GetOwnerClass();
			}
		}		
	}
	else 
	{
		ParentClass = DelegateReference.GetMemberParentClass(this);
	}

	if ( ParentClass != NULL )
	{
		return FString( TEXT("Shared/") ) + ParentClass->GetName();
	}

	return TEXT("");
}

FString UK2Node_BaseMCDelegate::GetDocumentationExcerptName() const
{
	return DelegateReference.GetMemberName().ToString();
}

void UK2Node_BaseMCDelegate::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const bool bAllowMultipleSelfs = AllowMultipleSelfs(true);
	if(bAllowMultipleSelfs && CompilerContext.bIsFullCompile)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(Schema);
		UEdGraphPin* MultiSelf = Schema->FindSelfPin(*this, EEdGraphPinDirection::EGPD_Input);
		check(MultiSelf);

		const bool bProperInputToExpandForEach = 
			(MultiSelf->LinkedTo.Num()) && 
			(NULL != MultiSelf->LinkedTo[0]) && 
			(MultiSelf->LinkedTo[0]->PinType.bIsArray);
		if(bProperInputToExpandForEach)
		{
			if(MultiSelf->LinkedTo.Num() > 1)
			{
				CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("K2Node", "BaseMCDelegateMultiArray", "Event Dispatcher does not accept multi-array-self @@").ToString()), this);
			}
			else
			{
				UK2Node_CallFunction::CallForEachElementInArrayExpansion(this, MultiSelf, CompilerContext, SourceGraph);
			}
		}
	}
}

bool UK2Node_BaseMCDelegate::IsAuthorityOnly() const
{
	const UMulticastDelegateProperty* DelegateProperty = DelegateReference.ResolveMember<UMulticastDelegateProperty>(this);
	return DelegateProperty && DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAuthorityOnly);

}

/////// UK2Node_AddDelegate ///////////

UK2Node_AddDelegate::UK2Node_AddDelegate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_AddDelegate::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	if(UEdGraphPin* DelegatePin = CreatePin(EGPD_Input, K2Schema->PC_Delegate, TEXT(""), GetDelegateSignature(), false, false, FK2Node_BaseMCDelegateHelper::DelegatePinName))
	{
		DelegatePin->PinFriendlyName = *NSLOCTEXT("K2Node", "PinFriendlyDelegatetName", "Event").ToString();
	}
}

FString UK2Node_AddDelegate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "AddDelegate", "Bind Event to %s").ToString(), *GetPropertyName().ToString());
}

FNodeHandlingFunctor* UK2Node_AddDelegate::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_AddRemoveDelegate(CompilerContext, KCST_AddMulticastDelegate);
}

/////// UK2Node_ClearDelegate ///////////

UK2Node_ClearDelegate::UK2Node_ClearDelegate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UK2Node_ClearDelegate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "ClearDelegate", "Unbind all Events from %s").ToString(), *GetPropertyName().ToString());
}

FNodeHandlingFunctor* UK2Node_ClearDelegate::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_ClearDelegate(CompilerContext);
}

/////// UK2Node_RemoveDelegate ///////////

UK2Node_RemoveDelegate::UK2Node_RemoveDelegate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_RemoveDelegate::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	if(UEdGraphPin* DelegatePin = CreatePin(EGPD_Input, K2Schema->PC_Delegate, TEXT(""), GetDelegateSignature(), false, false, FK2Node_BaseMCDelegateHelper::DelegatePinName))
	{
		DelegatePin->PinFriendlyName = *NSLOCTEXT("K2Node", "PinFriendlyDelegatetName", "Event").ToString();
	}
}

FString UK2Node_RemoveDelegate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "RemoveDelegate", "Unbind Event from %s").ToString(), *GetPropertyName().ToString());
}

FNodeHandlingFunctor* UK2Node_RemoveDelegate::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_AddRemoveDelegate(CompilerContext, KCST_RemoveMulticastDelegate);
}

/////// UK2Node_CallDelegate ///////////

UK2Node_CallDelegate::UK2Node_CallDelegate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UK2Node_CallDelegate::CreatePinsForFunctionInputs(const UFunction* Function)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create the inputs and outputs
	bool bAllPinsGood = true;
	for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* Param = *PropIt;
		const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
		if (bIsFunctionInput)
		{
			UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Param->GetName());
			const bool bPinGood = K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);

			bAllPinsGood = bAllPinsGood && bPinGood;
		}
	}

	return bAllPinsGood;
}

void UK2Node_CallDelegate::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePinsForFunctionInputs(GetDelegateSignature());
}

FString UK2Node_CallDelegate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "CallDelegate", "Call %s").ToString(), *GetPropertyName().ToString());
}

void UK2Node_CallDelegate::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	UK2Node::ValidateNodeDuringCompilation(MessageLog);
	if(UProperty* Property = GetProperty())
	{
		if(!Property->HasAllPropertyFlags(CPF_BlueprintCallable))
		{
			MessageLog.Error(*FString::Printf(*NSLOCTEXT("K2Node", "BaseMCDelegateNotCallable", "Event Dispatcher is not 'BlueprintCallable' @@").ToString()), this);
		}
	}
}

FNodeHandlingFunctor* UK2Node_CallDelegate::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_CallDelegate(CompilerContext);
}

FName UK2Node_CallDelegate::GetCornerIcon() const
{
	return TEXT("Graph.Message.MessageIcon");
}
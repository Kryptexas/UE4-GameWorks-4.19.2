// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace FindInBlueprintsUtil
{
	/* Get the path for a event */
	inline FString GetNodeTypePath(UK2Node_Event* Node)
	{
		return Node->EventSignatureClass->GetPathName();
	}
	/* Get the path for a function */
	inline FString GetNodeTypePath(UK2Node_CallFunction* Node)
	{
		return Node->FunctionReference.GetMemberParentClass(Node)->GetPathName();
	}
	/* Get the path for a macro instance */
	inline FString GetNodeTypePath(UK2Node_MacroInstance* Node)
	{
		UEdGraph* MacroGraph = Node->GetMacroGraph();
		if(MacroGraph)
		{
			if(auto Outer = MacroGraph->GetOuter())
			{
				return Outer->GetPathName();
			}
		}
		return FString();
	}
	/* Get the path for a Variable  */
	inline FString GetNodeTypePath(UK2Node_Variable* Node)
	{
		FString Result;
		if(!Node->VariableReference.IsSelfContext())
		{
			Result = Node->VariableReference.GetMemberParentClass(Node)->GetPathName();
		}
		if(UProperty* Property = Node->GetPropertyForVariable())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FEdGraphPinType Type;
			K2Schema->ConvertPropertyToPinType(Property, Type);
			FString Category = Type.PinCategory;
			if(UObject* SubCategoryObject = Type.PinSubCategoryObject.Get()) 
			{
				Category += FString(" '") + SubCategoryObject->GetName() + "'";
			}
			return  FString("[") + Category + "]" + Result;
		}
		return Result;
	}
	/* Get the path for a delegate */
	inline FString GetNodeTypePath(UK2Node_BaseMCDelegate* Node)
	{
		return FName().ToString();
	}
	inline FString GetNodeTypePath(UK2Node_DelegateSet* Node)
	{
		return FName().ToString();
	}
	/* Get the path for a node comment */
	inline FString GetNodeTypePath(UEdGraphNode* Node)
	{
		return Node->NodeComment;
	}
}

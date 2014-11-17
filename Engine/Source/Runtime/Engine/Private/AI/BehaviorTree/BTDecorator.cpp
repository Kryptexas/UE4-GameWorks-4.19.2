// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator::UBTDecorator(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "UnknownDecorator";

	FlowAbortMode = EBTFlowAbortMode::None;
	bAllowAbortNone = true;
	bAllowAbortLowerPri = true;
	bAllowAbortChildNodes = true;
	bNotifyActivation = false;
	bNotifyDeactivation = false;
	bNotifyProcessed = false;

	bShowInverseConditionDesc = true;
	bInverseCondition = false;
}

void UBTDecorator::InitializeDecorator(uint8 MyChildIndex)
{
	ChildIndex = MyChildIndex;
}

bool UBTDecorator::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return true;
}

void UBTDecorator::SetIsInversed(bool bShouldBeInversed)
{
	bInverseCondition = bShouldBeInversed;
}

void UBTDecorator::OnNodeActivation(struct FBehaviorTreeSearchData& SearchData)
{
}

void UBTDecorator::OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult)
{
}

void UBTDecorator::OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult)
{
}

bool UBTDecorator::WrappedCanExecute(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBTDecorator* NodeOb = bCreateNodeInstance ? (const UBTDecorator*)GetNodeInstance(OwnerComp, NodeMemory) : this;
	return NodeOb ? (IsInversed() != NodeOb->CalculateRawConditionValue(OwnerComp, NodeMemory)) : false;
}

void UBTDecorator::WrappedOnNodeActivation(struct FBehaviorTreeSearchData& SearchData) const
{
	if (bNotifyActivation)
	{
		const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(SearchData) : this;
		if (NodeOb)
		{
			((UBTDecorator*)NodeOb)->OnNodeActivation(SearchData);
		}		
	}
};

void UBTDecorator::WrappedOnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) const
{
	if (bNotifyDeactivation)
	{
		const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(SearchData) : this;
		if (NodeOb)
		{
			((UBTDecorator*)NodeOb)->OnNodeDeactivation(SearchData, NodeResult);
		}		
	}
}

void UBTDecorator::WrappedOnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const
{
	if (bNotifyProcessed)
	{
		const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(SearchData) : this;
		if (NodeOb)
		{
			((UBTDecorator*)NodeOb)->OnNodeProcessed(SearchData, NodeResult);
		}		
	}
}

const UBTNode* UBTDecorator::GetMyNode() const
{
	return GetParentNode() ? GetParentNode()->GetChildNode(ChildIndex) : NULL;
}

FString UBTDecorator::GetStaticDescription() const
{
	FString FlowAbortDesc;
	if (FlowAbortMode != EBTFlowAbortMode::None)
	{
		FlowAbortDesc = FString::Printf(TEXT("aborts %s"), *UBehaviorTreeTypes::DescribeFlowAbortMode(FlowAbortMode).ToLower());
	}

	FString InversedDesc;
	if (bShowInverseConditionDesc && IsInversed())
	{
		InversedDesc = TEXT("inversed");
	}

	FString AdditionalDesc;
	if (FlowAbortDesc.Len() || InversedDesc.Len())
	{
		AdditionalDesc = FString::Printf(TEXT("( %s%s%s )\n"), *FlowAbortDesc, 
			(FlowAbortDesc.Len() > 0) && (InversedDesc.Len() > 0) ? TEXT(", ") : TEXT(""),
			*InversedDesc);
	}

	return FString::Printf(TEXT("%s%s"), *AdditionalDesc, *UBehaviorTreeTypes::GetShortTypeName(this));
}

void UBTDecorator::ValidateFlowAbortMode()
{
#if WITH_EDITOR
	if (GetParentNode() == NULL)
	{
		FlowAbortMode = EBTFlowAbortMode::None;
		return;
	}

	if (GetParentNode()->CanAbortLowerPriority() == false)
	{
		if (FlowAbortMode == EBTFlowAbortMode::Both)
		{
			FlowAbortMode = GetParentNode()->CanAbortSelf() ? EBTFlowAbortMode::Self : EBTFlowAbortMode::None;
		}
		else if (FlowAbortMode == EBTFlowAbortMode::LowerPriority)
		{
			FlowAbortMode = EBTFlowAbortMode::None;
		}
	}

	if (GetParentNode()->CanAbortSelf() == false)
	{
		if (FlowAbortMode == EBTFlowAbortMode::Both)
		{
			FlowAbortMode = GetParentNode()->CanAbortLowerPriority() ? EBTFlowAbortMode::LowerPriority : EBTFlowAbortMode::None;
		}
		else if (FlowAbortMode == EBTFlowAbortMode::Self)
		{
			FlowAbortMode = EBTFlowAbortMode::None;
		}
	}
#endif
}

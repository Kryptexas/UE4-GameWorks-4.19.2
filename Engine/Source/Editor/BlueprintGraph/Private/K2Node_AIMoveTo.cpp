// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "EngineKismetLibraryClasses.h"
#include "CompilerResultsLog.h"

UK2Node_AIMoveTo::UK2Node_AIMoveTo(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetAIHelperLibrary, CreateMoveToProxyObject);
	ProxyFactoryClass = UKismetAIHelperLibrary::StaticClass();
	ProxyClass = UKismetAIAsyncTaskProxy::StaticClass();
}

#if WITH_EDITOR
FString UK2Node_AIMoveTo::GetCategoryName()
{
	return TEXT("AI");
}

FString UK2Node_AIMoveTo::GetTooltip() const
{
	return TEXT("Simple order for Pawn with AIController to move to a specific location");
}

FString UK2Node_AIMoveTo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return "AI MoveTo";
}
#endif // WITH_EDITOR


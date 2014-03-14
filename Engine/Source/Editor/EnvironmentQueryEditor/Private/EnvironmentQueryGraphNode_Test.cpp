// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnvironmentQueryEditorPrivatePCH.h"

UEnvironmentQueryGraphNode_Test::UEnvironmentQueryGraphNode_Test(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	bTestEnabled = true;
	bHasNamedWeight = false;
	TestWeightPct = -1.0f;
}

void UEnvironmentQueryGraphNode_Test::PostPlacedNewNode()
{
	if (EnvQueryNodeClass != NULL)
	{
		UEnvQuery* Query = Cast<UEnvQuery>(GetEnvironmentQueryGraph()->GetOuter());
		NodeInstance = ConstructObject<UEnvQueryTest>(EnvQueryNodeClass, Query);
		NodeInstance->SetFlags(RF_Transactional);
	}

	if (ParentNode)
	{
		ParentNode->CalculateWeights();
	}
}

void UEnvironmentQueryGraphNode_Test::DestroyNode()
{
	if (ParentNode)
	{
		ParentNode->Modify();
		ParentNode->Tests.Remove(this);
	}

	Super::DestroyNode();
}

FString UEnvironmentQueryGraphNode_Test::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEnvQueryTest* TestInstance = Cast<UEnvQueryTest>(NodeInstance);
	return TestInstance ? TestInstance->GetDescriptionTitle() : FString();
}

FString UEnvironmentQueryGraphNode_Test::GetDescription() const
{
	UEnvQueryTest* TestInstance = Cast<UEnvQueryTest>(NodeInstance);
	return TestInstance ? TestInstance->GetDescriptionDetails() : FString();
}

void UEnvironmentQueryGraphNode_Test::SetDisplayedWeight(float Pct, bool bNamed)
{
	if (TestWeightPct != Pct || bHasNamedWeight != bNamed)
	{
		Modify();
	}

	TestWeightPct = Pct;
	bHasNamedWeight = bNamed;
}

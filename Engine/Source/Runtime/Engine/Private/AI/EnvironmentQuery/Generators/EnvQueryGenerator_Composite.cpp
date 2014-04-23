// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryGenerator_Composite::UEnvQueryGenerator_Composite(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	GenerateDelegate.BindUObject(this, &UEnvQueryGenerator_Composite::GenerateItems);
}

void UEnvQueryGenerator_Composite::GenerateItems(struct FEnvQueryInstance& QueryInstance)
{
	for (int32 Idx = 0; Idx < Generators.Num(); Idx++)
	{
		if (Generators[Idx] && Generators[Idx]->GenerateDelegate.IsBound())
		{
			Generators[Idx]->GenerateDelegate.Execute(QueryInstance);
		}
	}
}

FText UEnvQueryGenerator_Composite::GetDescriptionTitle() const
{
	FText Desc = Super::GetDescriptionTitle();
	for (int32 Idx = 0; Idx < Generators.Num(); Idx++)
	{
		if (Generators[Idx])
		{
			Desc = FText::Format(LOCTEXT("DescTitleExtention", "{0}\n  {1}"), Desc, Generators[Idx]->GetDescriptionTitle());
		}
	}

	return Desc;
};

#undef LOCTEXT_NAMESPACE
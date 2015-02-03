// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "ProceduralFoliageFactory.h"
#include "IAssetTypeActions.h"
#include "ProceduralFoliage.h"

UProceduralFoliageFactory::UProceduralFoliageFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UProceduralFoliage::StaticClass();
}

UObject* UProceduralFoliageFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	auto NewProceduralFoliage = NewObject<UProceduralFoliage>(InParent, Class, Name, Flags | RF_Transactional);

	return NewProceduralFoliage;
}

uint32 UProceduralFoliageFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}
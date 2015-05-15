// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GatherableTextData.h"

COREUOBJECT_API extern void GatherLocalizationDataFromPropertiesOfDataStructure(UStruct* const Structure, void* const Data, TArray<FGatherableTextData>& GatherableTextDataArray);
COREUOBJECT_API extern void GatherLocalizationDataFromChildTextProperies(const FString& PathToParent, UProperty* const Property, void* const ValueAddress, TArray<FGatherableTextData>& GatherableTextDataArray, const bool ForceIsEditorOnly);
COREUOBJECT_API extern void GatherLocalizationDataFromTextProperty(const FString& PathToParent, UTextProperty* const TextProperty, void* const ValueAddress, TArray<FGatherableTextData>& GatherableTextDataArray, const bool ForceIsEditorOnly);
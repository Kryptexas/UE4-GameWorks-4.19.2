// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BlueprintFunctionLibrary.generated.h"

// This class is a base class for any function libraries exposed to blueprints.
// Methods in subclasses are expected to be static, and no methods should be added to this base class.
UCLASS(Abstract, MinimalAPI)
class UBlueprintFunctionLibrary : public UObject
{
	GENERATED_BODY()
public:
	ENGINE_API UBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UObject interface
	ENGINE_API virtual int32 GetFunctionCallspace(UFunction* Function, void* Parms, FFrame* Stack) override;
	// End of UObject interface

protected:

	//bool	CheckAuthority(UObject* 

};

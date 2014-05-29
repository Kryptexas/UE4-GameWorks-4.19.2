// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TableViewBase.generated.h"

UCLASS(Abstract, meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UTableViewBase : public UWidget
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FOnGenerateRowUObject, UObject*, Item);
	
};

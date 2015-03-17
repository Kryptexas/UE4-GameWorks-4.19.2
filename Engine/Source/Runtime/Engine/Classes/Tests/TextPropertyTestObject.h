// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.






#pragma once
#include "TextPropertyTestObject.generated.h"
UCLASS()
class UTextPropertyTestObject : public UObject
{
	GENERATED_BODY()
public:
	UTextPropertyTestObject(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	UPROPERTY()
	FText DefaultedText;
	UPROPERTY()
	FText UndefaultedText;
	UPROPERTY()
	FText TransientText;
};

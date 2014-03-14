// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DistributionFloatParameterBase.generated.h"

UENUM()
enum DistributionParamMode
{
	DPM_Normal,
	DPM_Abs,
	DPM_Direct,
	DPM_MAX,
};

UCLASS(abstract, collapsecategories, hidecategories=Object, editinlinenew)
class UDistributionFloatParameterBase : public UDistributionFloatConstant
{
	GENERATED_UCLASS_BODY()

	/** todo document */
	UPROPERTY(EditAnywhere, Category=DistributionFloatParameterBase)
	FName ParameterName;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=DistributionFloatParameterBase)
	float MinInput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=DistributionFloatParameterBase)
	float MaxInput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=DistributionFloatParameterBase)
	float MinOutput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=DistributionFloatParameterBase)
	float MaxOutput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=DistributionFloatParameterBase)
	TEnumAsByte<enum DistributionParamMode> ParamMode;


	// Begin UDistributionFloat Interface
	virtual float GetValue( float F = 0.f, UObject* Data = NULL, class FRandomStream* InRandomStream = NULL ) const OVERRIDE;
	// End UDistributionFloat Interface
	
	
	/** todo document */
	virtual bool GetParamValue(UObject* Data, FName ParamName, float& OutFloat) const { return false; }

	
	// Begin Distribution Float
	virtual bool CanBeBaked() const OVERRIDE { return false; }
};




// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraCommon.h"
#include "VectorVM.h"
#include "NiagaraDataInterface.generated.h"

class INiagaraCompiler;
class UCurveVector;
class UCurveLinearColor;
class UCurveFloat;

/** Base class for all Niagara data interfaces. */
UCLASS(abstract, EditInlineNew)
class NIAGARA_API UNiagaraDataInterface : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Gets all the available functions for this data interface. */
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) {}

	/** Returns the delegate for the passed function signature. */
	virtual FVMExternalFunction GetVMExternalFunction(const FNiagaraFunctionSignature& Signature) { return FVMExternalFunction(); };
};

/** Data Interface allowing sampling of float curves. */
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Float Curve"))
class NIAGARA_API UNiagaraDataInterfaceCurve : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Curve")
	UCurveFloat* Curve;

	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FNiagaraFunctionSignature& Signature)override;
	
	void SampleCurve(FVMExternalFuncInputs& Inputs, FVMExternalFuncOutputs& Outputs, uint32 NumInstances);
};

/** Data Interface allowing sampling of vector curves. */
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Vector Curve"))
class NIAGARA_API UNiagaraDataInterfaceVectorCurve : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Curve")
	UCurveVector* Curve;

	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FNiagaraFunctionSignature& Signature)override;

	void SampleCurve(FVMExternalFuncInputs& Inputs, FVMExternalFuncOutputs& Outputs, uint32 NumInstances);
};

/** Data Interface allowing sampling of color curves. */
UCLASS(EditInlineNew, Category="Curves", meta = (DisplayName = "Color Curve"))
class NIAGARA_API UNiagaraDataInterfaceColorCurve : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Curve")
	UCurveLinearColor* Curve;

	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FNiagaraFunctionSignature& Signature)override;

	void SampleCurve(FVMExternalFuncInputs& Inputs, FVMExternalFuncOutputs& Outputs, uint32 NumInstances);
};

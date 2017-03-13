// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterface.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveFloat.h"

UNiagaraDataInterface::UNiagaraDataInterface(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

//////////////////////////////////////////////////////////////////////////
//Float Curve

UNiagaraDataInterfaceCurve::UNiagaraDataInterfaceCurve(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UNiagaraDataInterfaceCurve::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false);
	}
}

void UNiagaraDataInterfaceCurve::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	FNiagaraFunctionSignature Sig;
	Sig.Name = TEXT("SampleCurve");
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Curve")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("X")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Value")));
	Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

FVMExternalFunction UNiagaraDataInterfaceCurve::GetVMExternalFunction(const FNiagaraFunctionSignature& Signature)
{
	check(Signature.GetName() == FString(TEXT("SampleCurve")));
	return FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceCurve::SampleCurve);
}

void UNiagaraDataInterfaceCurve::SampleCurve(FVMExternalFuncInputs& Inputs, FVMExternalFuncOutputs& Outputs, uint32 NumInstances)
{
	//TODO: Create some SIMDable optimized representation of the curve to do this faster.
	check(Inputs.Num() == 1);
	check(Outputs.Num() == 1);

	FVMExternalFunctionInput& X = Inputs[0];
	check(X.ComponentPtrs.Num() == 1);
	float* XPtr = (float*)X.ComponentPtrs[0];
	FVMExternalFunctionOutput& Sample = Outputs[0];
	check(Sample.ComponentPtrs.Num() == 1);
	float* SamplePtr = (float*)Sample.ComponentPtrs[0];

	if (Curve)
	{
		for (uint32 i = 0; i < NumInstances; ++i)
		{
			SamplePtr[i] = Curve->GetFloatValue(*XPtr);
			XPtr += X.bIsConstant ? 0 : 1;//Don't really like this handling of constants vs registers. Can we have the data interface require one or the other?
		}
	}
	else
	{
		FMemory::Memset(SamplePtr, 0, sizeof(float) * NumInstances);
	}
}

//////////////////////////////////////////////////////////////////////////
//Vector Curve

UNiagaraDataInterfaceVectorCurve::UNiagaraDataInterfaceVectorCurve(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UNiagaraDataInterfaceVectorCurve::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false);
	}
}

void UNiagaraDataInterfaceVectorCurve::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	FNiagaraFunctionSignature Sig;
	Sig.Name = TEXT("SampleVectorCurve");
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("VectorCurve")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("X")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Value")));
	Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

FVMExternalFunction UNiagaraDataInterfaceVectorCurve::GetVMExternalFunction(const FNiagaraFunctionSignature& Signature)
{
	check(Signature.GetName() == FString(TEXT("SampleVectorCurve")));
	return FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceVectorCurve::SampleCurve);
}

void UNiagaraDataInterfaceVectorCurve::SampleCurve(FVMExternalFuncInputs& Inputs, FVMExternalFuncOutputs& Outputs, uint32 NumInstances)
{
	//TODO: Create some SIMDable optimized representation of the curve to do this faster.
	check(Inputs.Num() == 1);
	check(Outputs.Num() == 1);

	FVMExternalFunctionInput& X = Inputs[0];
	check(X.ComponentPtrs.Num() == 1);
	float* XPtr = (float*)X.ComponentPtrs[0];
	FVMExternalFunctionOutput& Sample = Outputs[0];
	check(Sample.ComponentPtrs.Num() == 3);
	float* SamplePtrX = (float*)Sample.ComponentPtrs[0];
	float* SamplePtrY = (float*)Sample.ComponentPtrs[1];
	float* SamplePtrZ = (float*)Sample.ComponentPtrs[2];

	if (Curve)
	{
		for (uint32 i = 0; i < NumInstances; ++i)
		{
			FVector V = Curve->GetVectorValue(*XPtr);
			SamplePtrX[i] = V.X;
			SamplePtrY[i] = V.Y;
			SamplePtrZ[i] = V.Z;
			XPtr += X.bIsConstant ? 0 : 1;//Don't really like this handling of constants vs registers. Can we have the data interface require one or the other?
		}
	}
	else
	{
		FMemory::Memset(SamplePtrX, 0, sizeof(float) * NumInstances);
		FMemory::Memset(SamplePtrY, 0, sizeof(float) * NumInstances);
		FMemory::Memset(SamplePtrZ, 0, sizeof(float) * NumInstances);
	}
}

//////////////////////////////////////////////////////////////////////////
//Color Curve

UNiagaraDataInterfaceColorCurve::UNiagaraDataInterfaceColorCurve(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UNiagaraDataInterfaceColorCurve::PostInitProperties()
{
	Super::PostInitProperties();

	//Can we regitser data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false);
	}
}

void UNiagaraDataInterfaceColorCurve::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	FNiagaraFunctionSignature Sig;
	Sig.Name = TEXT("SampleColorCurve");
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("ColorCurve")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("X")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Value")));
	Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

FVMExternalFunction UNiagaraDataInterfaceColorCurve::GetVMExternalFunction(const FNiagaraFunctionSignature& Signature)
{
	check(Signature.GetName() == FString(TEXT("SampleColorCurve")));
	return FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceColorCurve::SampleCurve);
}

void UNiagaraDataInterfaceColorCurve::SampleCurve(FVMExternalFuncInputs& Inputs, FVMExternalFuncOutputs& Outputs, uint32 NumInstances)
{
	//TODO: Create some SIMDable optimized representation of the curve to do this faster.
	check(Inputs.Num() == 1);
	check(Outputs.Num() == 1);

	FVMExternalFunctionInput& X = Inputs[0];
	check(X.ComponentPtrs.Num() == 1);
	float* XPtr = (float*)X.ComponentPtrs[0];
	FVMExternalFunctionOutput& Sample = Outputs[0];
	check(Sample.ComponentPtrs.Num() == 4);
	float* SamplePtrR = (float*)Sample.ComponentPtrs[0];
	float* SamplePtrG = (float*)Sample.ComponentPtrs[1];
	float* SamplePtrB = (float*)Sample.ComponentPtrs[2];
	float* SamplePtrA = (float*)Sample.ComponentPtrs[3];

	if (Curve)
	{
		for (uint32 i = 0; i < NumInstances; ++i)
		{
			FLinearColor C = Curve->GetLinearColorValue(*XPtr);
			SamplePtrR[i] = C.R;
			SamplePtrG[i] = C.G;
			SamplePtrB[i] = C.B;
			SamplePtrA[i] = C.A;
			XPtr += X.bIsConstant ? 0 : 1;//Don't really like this handling of constants vs registers. Can we have the data interface require one or the other?
		}
	}
	else
	{
		FMemory::Memset(SamplePtrR, 0, sizeof(float) * NumInstances);
		FMemory::Memset(SamplePtrG, 0, sizeof(float) * NumInstances);
		FMemory::Memset(SamplePtrB, 0, sizeof(float) * NumInstances);
		FMemory::Memset(SamplePtrA, 0, sizeof(float) * NumInstances);
	}
}
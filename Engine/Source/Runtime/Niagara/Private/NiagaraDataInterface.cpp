// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterface.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveFloat.h"
#include "NiagaraTypes.h"

UNiagaraDataInterface::UNiagaraDataInterface(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UNiagaraDataInterface::CopyTo(UNiagaraDataInterface* Destination) 
{
	if (Destination == nullptr || Destination->GetClass() != GetClass())
	{
		return false;
	}
	return true;
}

bool UNiagaraDataInterface::Equals(UNiagaraDataInterface* Other)
{
	if (Other == nullptr || Other->GetClass() != GetClass())
	{
		return false;
	}
	return true;
}

bool UNiagaraDataInterface::IsDataInterfaceType(const FNiagaraTypeDefinition& TypeDef)
{
	const UClass* Class = TypeDef.GetClass();
	if (Class && Class->IsChildOf(UNiagaraDataInterface::StaticClass()))
	{
		return true;
	}
	return false;
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
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

bool UNiagaraDataInterfaceCurve::CopyTo(UNiagaraDataInterface* Destination) 
{
	if (!Super::CopyTo(Destination))
	{
		return false;
	}
	CastChecked<UNiagaraDataInterfaceCurve>(Destination)->Curve = Curve;
	return true;
}

bool UNiagaraDataInterfaceCurve::Equals(UNiagaraDataInterface* Other)
{
	if (!Super::Equals(Other))
	{
		return false;
	}

	return CastChecked<UNiagaraDataInterfaceCurve>(Other)->Curve == Curve;
}

void UNiagaraDataInterfaceCurve::GetCurveData(TArray<FCurveData>& OutCurveData)
{
	OutCurveData.Add(FCurveData(&Curve, NAME_None, FLinearColor::Red));
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
//	Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceCurve, SampleCurve);
FVMExternalFunction UNiagaraDataInterfaceCurve::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)
{
	check(BindingInfo.Name == TEXT("SampleCurve"));
	check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 1);
	return TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceCurve, SampleCurve)>::Bind(this, BindingInfo);
}

template<typename XParamType>
void UNiagaraDataInterfaceCurve::SampleCurve(FVectorVMContext& Context)
{
	XParamType XParam(Context);
	FRegisterHandler<float> OutSample(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		*OutSample.GetDest() = Curve.Eval(XParam.Get());
		XParam.Advance();
		OutSample.Advance();
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
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

bool UNiagaraDataInterfaceVectorCurve::CopyTo(UNiagaraDataInterface* Destination) 
{
	if (!Super::CopyTo(Destination))
	{
		return false;
	}
	UNiagaraDataInterfaceVectorCurve* DestinationVectorCurve = CastChecked<UNiagaraDataInterfaceVectorCurve>(Destination);
	DestinationVectorCurve->XCurve = XCurve;
	DestinationVectorCurve->YCurve = YCurve;
	DestinationVectorCurve->ZCurve = ZCurve;
	return true;
}

bool UNiagaraDataInterfaceVectorCurve::Equals(UNiagaraDataInterface* Other)
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	UNiagaraDataInterfaceVectorCurve* OtherVectorCurve = CastChecked<UNiagaraDataInterfaceVectorCurve>(Other);
	return OtherVectorCurve->XCurve == XCurve &&
		OtherVectorCurve->YCurve == YCurve &&
		OtherVectorCurve->ZCurve == ZCurve;
}

void UNiagaraDataInterfaceVectorCurve::GetCurveData(TArray<FCurveData>& OutCurveData)
{
	OutCurveData.Add(FCurveData(&XCurve, TEXT("X"), FLinearColor::Red));
	OutCurveData.Add(FCurveData(&YCurve, TEXT("Y"), FLinearColor::Green));
	OutCurveData.Add(FCurveData(&ZCurve, TEXT("Z"), FLinearColor::Blue));
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
	//Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceVectorCurve, SampleCurve);
FVMExternalFunction UNiagaraDataInterfaceVectorCurve::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)
{
	check(BindingInfo.Name == TEXT("SampleVectorCurve"));
	check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
	return TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceVectorCurve, SampleCurve)>::Bind(this, BindingInfo);
}

template<typename XParamType>
void UNiagaraDataInterfaceVectorCurve::SampleCurve(FVectorVMContext& Context)
{
	//TODO: Create some SIMDable optimized representation of the curve to do this faster.
	XParamType XParam(Context);
	FRegisterHandler<float> OutSampleX(Context);
	FRegisterHandler<float> OutSampleY(Context);
	FRegisterHandler<float> OutSampleZ(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float X = XParam.Get();
		FVector V(XCurve.Eval(X), YCurve.Eval(X), ZCurve.Eval(X));
		*OutSampleX.GetDest() = V.X;
		*OutSampleY.GetDest() = V.Y;
		*OutSampleZ.GetDest() = V.Z;
		XParam.Advance();
		OutSampleX.Advance();
		OutSampleY.Advance();
		OutSampleZ.Advance();
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
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

bool UNiagaraDataInterfaceColorCurve::CopyTo(UNiagaraDataInterface* Destination) 
{
	if (!Super::CopyTo(Destination))
	{
		return false;
	}
	UNiagaraDataInterfaceColorCurve* DestinationColorCurve = CastChecked<UNiagaraDataInterfaceColorCurve>(Destination);
	DestinationColorCurve->RedCurve = RedCurve;
	DestinationColorCurve->GreenCurve = GreenCurve;
	DestinationColorCurve->BlueCurve = BlueCurve;
	DestinationColorCurve->AlphaCurve = AlphaCurve;
	return true;
}

bool UNiagaraDataInterfaceColorCurve::Equals(UNiagaraDataInterface* Other)
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	UNiagaraDataInterfaceColorCurve* OtherColoRedCurve = CastChecked<UNiagaraDataInterfaceColorCurve>(Other);
	return OtherColoRedCurve->RedCurve == RedCurve &&
		OtherColoRedCurve->GreenCurve == GreenCurve &&
		OtherColoRedCurve->BlueCurve == BlueCurve &&
		OtherColoRedCurve->AlphaCurve == AlphaCurve;
}

void UNiagaraDataInterfaceColorCurve::GetCurveData(TArray<FCurveData>& OutCurveData)
{
	OutCurveData.Add(FCurveData(&RedCurve, TEXT("Red"), FLinearColor::Red));
	OutCurveData.Add(FCurveData(&GreenCurve, TEXT("Green"), FLinearColor::Green));
	OutCurveData.Add(FCurveData(&BlueCurve, TEXT("Blue"), FLinearColor::Blue));
	OutCurveData.Add(FCurveData(&AlphaCurve, TEXT("Alpha"), FLinearColor::White));
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
	//Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceColorCurve, SampleCurve);
FVMExternalFunction UNiagaraDataInterfaceColorCurve::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)
{
	check(BindingInfo.Name == TEXT("SampleColorCurve"));
	check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 4);
	return TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceColorCurve, SampleCurve)>::Bind(this, BindingInfo);
}

template<typename XParamType>
void UNiagaraDataInterfaceColorCurve::SampleCurve(FVectorVMContext& Context)
{
	//TODO: Create some SIMDable optimized representation of the curve to do this faster.
	XParamType XParam(Context);
	FRegisterHandler<float> SamplePtrR(Context);
	FRegisterHandler<float> SamplePtrG(Context);
	FRegisterHandler<float> SamplePtrB(Context);
	FRegisterHandler<float> SamplePtrA(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float X = XParam.Get();
		FLinearColor C(RedCurve.Eval(X), GreenCurve.Eval(X), BlueCurve.Eval(X), AlphaCurve.Eval(X));
		*SamplePtrR.GetDest() = C.R;
		*SamplePtrG.GetDest() = C.G;
		*SamplePtrB.GetDest() = C.B;
		*SamplePtrA.GetDest() = C.A;
		XParam.Advance();
		SamplePtrR.Advance();
		SamplePtrG.Advance();
		SamplePtrB.Advance();
		SamplePtrA.Advance();
	}
}

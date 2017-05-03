// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceSpline.h"
#include "NiagaraSimulation.h"
#include "NiagaraComponent.h"
#include "NiagaraEffectInstance.h"
#include "Internationalization.h"

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceSpline"

namespace SplineUtils
{
	struct FTransformHandlerNoop
	{
		FORCEINLINE void Transform(FVector& V, FMatrix& M) {  }
	};

	struct FTransformHandlerPosition
	{
		FORCEINLINE void Transform(FVector& P, FMatrix& M) { P = M.TransformPosition(P); }
	};

	struct FTransformHandlerUnitVector
	{
		FORCEINLINE void Transform(FVector& V, FMatrix& M) { V = M.TransformVector(V).GetUnsafeNormal3(); }
	};

	struct FTransformHandlerTangent
	{
		FORCEINLINE void Transform(FVector& V, FMatrix& M) { V = M.TransformVector(V); }
	};
};

UNiagaraDataInterfaceSpline::UNiagaraDataInterfaceSpline(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, Source(nullptr)
	, Component(nullptr)
	, Transform(FMatrix::Identity)
	, TransformInverseTransposed(FMatrix::Identity)
{

}

void UNiagaraDataInterfaceSpline::PostInitProperties()
{
	Super::PostInitProperties();

	//Can we regitser data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

static const FName SampleSplinePositionByUnitDistanceName("SampleSplinePositionByUnitDistance");
static const FName SampleSplinePositionByUnitDistanceWSName("SampleSplinePositionByUnitDistanceWS");

static const FName SampleSplineUpVectorByUnitDistanceName("SampleSplineUpVectorByUnitDistance");
static const FName SampleSplineUpVectorByUnitDistanceWSName("SampleSplineUpVectorByUnitDistanceWS");

static const FName SampleSplineDirectionByUnitDistanceName("SampleSplineDirectionByUnitDistance");
static const FName SampleSplineDirectionByUnitDistanceWSName("SampleSplineDirectionByUnitDistanceWS");

static const FName SampleSplineRightVectorByUnitDistanceName("SampleSplineRightVectorByUnitDistance");
static const FName SampleSplineRightVectorByUnitDistanceWSName("SampleSplineRightVectorByUnitDistanceWS");

static const FName SampleSplineTangentByUnitDistanceName("SampleSplineTangentByUnitDistance");
static const FName SampleSplineTangentByUnitDistanceWSName("SampleSplineTangentByUnitDistanceWS");


static const FName FindClosestUnitDistanceFromPositionWSName("FindClosestUnitDistanceFromPositionWS");


/** Temporary solution for exposing the transform of a mesh. Ideally this would be done by allowing interfaces to add to the uniform set for a simulation. */
static const FName GetSplineLocalToWorldName("GetSplineLocalToWorld");
static const FName GetSplineLocalToWorldInverseTransposedName("GetSplineLocalToWorldInverseTransposed");

void UNiagaraDataInterfaceSpline::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplinePositionByUnitDistanceName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplinePositionByUnitDistance", "Sample the spline Position where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the local space of the referenced USplineComponent."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplinePositionByUnitDistanceWSName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplinePositionByUnitDistanceWS", "Sample the spline Position where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the world space of the level."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineDirectionByUnitDistanceName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Direction")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineDirectionByUnitDistance", "Sample the spline direction vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the local space of the referenced USplineComponent."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineDirectionByUnitDistanceWSName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Direction")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineDirectionByUnitDistanceWS", "Sample the spline direction vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the world space of the level."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineUpVectorByUnitDistanceName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("UpVector")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineUpVectorByUnitDistance", "Sample the spline up vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the local space of the referenced USplineComponent."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineUpVectorByUnitDistanceWSName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineUpVectorByUnitDistanceWS", "Sample the spline up vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the world space of the level."));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("UpVector")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineRightVectorByUnitDistanceName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineRightVectorByUnitDistance", "Sample the spline right vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the local space of the referenced USplineComponent."));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("RightVector")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineRightVectorByUnitDistanceWSName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineRightVectorByUnitDistanceWS", "Sample the spline right vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the world space of the level."));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("RightVector")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineTangentByUnitDistanceName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Tangent")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineTangentVectorByUnitDistance", "Sample the spline tangent vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the local space of the referenced USplineComponent."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleSplineTangentByUnitDistanceWSName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Tangent")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_SampleSplineTangentVectorByUnitDistanceWS", "Sample the spline tangent vector where U is a 0 to 1 value representing the start and normalized length of the spline.\nThis is in the world space of the level."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = GetSplineLocalToWorldName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Transform")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_GetSplineLocalToWorld", "Get the transform from the USplineComponent's local space to world space."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = GetSplineLocalToWorldInverseTransposedName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Transform")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_GetSplineLocalToWorldInverseTransposed", "Get the transform from the world space to the USplineComponent's local space."));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = FindClosestUnitDistanceFromPositionWSName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Spline")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("PositionWS")));

		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("U")));
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		//Sig.Owner = *GetName();
		Sig.SetDescription(LOCTEXT("DataInterfaceSpline_FindClosestUnitDistanceFromPositionWS", "Given a world space position, find the closest value 'U' on the USplineComponent to that point."));
		OutFunctions.Add(Sig);
	}
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplinePositionByUnitDistance);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineUpVectorByUnitDistance);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineRightVectorByUnitDistance);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineDirectionByUnitDistance);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineTangentByUnitDistance);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, FindClosestUnitDistanceFromPositionWS);

FVMExternalFunction UNiagaraDataInterfaceSpline::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)
{
	using namespace SplineUtils;
	if (BindingInfo.Name == SampleSplinePositionByUnitDistanceName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerNoop, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplinePositionByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplinePositionByUnitDistanceWSName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerPosition, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplinePositionByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineUpVectorByUnitDistanceName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerNoop, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineUpVectorByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineUpVectorByUnitDistanceWSName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerUnitVector, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineUpVectorByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineDirectionByUnitDistanceName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerNoop, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineDirectionByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineDirectionByUnitDistanceWSName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerUnitVector, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineDirectionByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineRightVectorByUnitDistanceName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerNoop, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineRightVectorByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineRightVectorByUnitDistanceWSName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerUnitVector, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineRightVectorByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineTangentByUnitDistanceName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerNoop, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineTangentByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == SampleSplineTangentByUnitDistanceWSName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 3);
		return TNDIExplicitBinder<FTransformHandlerTangent, TNDIParamBinder<0, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, SampleSplineTangentByUnitDistance)>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == FindClosestUnitDistanceFromPositionWSName)
	{
		check(BindingInfo.GetNumInputs() == 3 && BindingInfo.GetNumOutputs() == 1);
		return TNDIParamBinder<0, float, TNDIParamBinder<1, float, TNDIParamBinder<2, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceSpline, FindClosestUnitDistanceFromPositionWS)>>>::Bind(this, BindingInfo);
	}
	else if (BindingInfo.Name == GetSplineLocalToWorldName)
	{
		check(BindingInfo.GetNumInputs() == 0 && BindingInfo.GetNumOutputs() == 16);
		return FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceSpline::GetLocalToWorld);
	}
	else if (BindingInfo.Name == GetSplineLocalToWorldInverseTransposedName)
	{
		check(BindingInfo.GetNumInputs() == 0 && BindingInfo.GetNumOutputs() == 16);
		return FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceSpline::GetLocalToWorldInverseTransposed);
	}

	//check(0);
	return FVMExternalFunction();
}

bool UNiagaraDataInterfaceSpline::CopyTo(UNiagaraDataInterface* Destination)
{
	if (!Super::CopyTo(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceSpline* OtherTyped = CastChecked<UNiagaraDataInterfaceSpline>(Destination);
	OtherTyped->Source = Source;
	return true;
}

bool UNiagaraDataInterfaceSpline::Equals(UNiagaraDataInterface* Other)
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	UNiagaraDataInterfaceSpline* OtherTyped = CastChecked<UNiagaraDataInterfaceSpline>(Other);
	return OtherTyped->Source == Source;
}

bool UNiagaraDataInterfaceSpline::HasPreSimulationTick()const
{
	return true;
}

bool UNiagaraDataInterfaceSpline::PreSimulationTick(FNiagaraEffectInstance* EffectInstance, float DeltaSeconds)
{
	check(EffectInstance);
	Component = nullptr;
	Transform = FMatrix::Identity;
	TransformInverseTransposed = FMatrix::Identity;
	if (Source)
	{
		USplineComponent* SourceComp = Source->FindComponentByClass<USplineComponent>();

		if (SourceComp)
		{
			Component = SourceComp;
		}
	}
	else
	{
		if (UNiagaraComponent* SimComp = EffectInstance->GetComponent())
		{
			if (AActor* Owner = SimComp->GetAttachmentRootActor())
			{
				USplineComponent* SourceComp = Owner->FindComponentByClass<USplineComponent>();

				if (SourceComp)
				{
					Component = SourceComp;
				}
			}
		}
	}
	

	//Re-evaluate source in case it's changed?
	if (Component)
	{
		Transform = Component->GetComponentToWorld().ToMatrixWithScale();
		TransformInverseTransposed = Transform.InverseFast().GetTransposed();
	}

	//Any situations requiring a rebind?
	return false;
}

template<typename TransformHandlerType, typename SplineSampleType>
void UNiagaraDataInterfaceSpline::SampleSplinePositionByUnitDistance(FVectorVMContext& Context)
{
	checkSlow(IsValid());

	TransformHandlerType TransformHandler;
	SplineSampleType SplineSampleParam(Context);
	FRegisterHandler<float> OutPosX(Context);
	FRegisterHandler<float> OutPosY(Context);
	FRegisterHandler<float> OutPosZ(Context);
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float DistanceUnitDistance = SplineSampleParam.Get();

		FVector Pos = Component->GetLocationAtDistanceAlongSpline(DistanceUnitDistance * Component->GetSplineLength(), ESplineCoordinateSpace::Local);
		TransformHandler.Transform(Pos, Transform);

		*OutPosX.GetDest() = Pos.X;
		*OutPosY.GetDest() = Pos.Y;
		*OutPosZ.GetDest() = Pos.Z;
		SplineSampleParam.Advance();
		OutPosX.Advance();
		OutPosY.Advance();
		OutPosZ.Advance();
	}
}

template<typename TransformHandlerType, typename SplineSampleType>
void UNiagaraDataInterfaceSpline::SampleSplineUpVectorByUnitDistance(FVectorVMContext& Context)
{
	checkSlow(IsValid());

	TransformHandlerType TransformHandler;
	SplineSampleType SplineSampleParam(Context);
	FRegisterHandler<float> OutPosX(Context);
	FRegisterHandler<float> OutPosY(Context);
	FRegisterHandler<float> OutPosZ(Context);
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float DistanceUnitDistance = SplineSampleParam.Get();

		FVector Pos = Component->GetUpVectorAtDistanceAlongSpline(DistanceUnitDistance * Component->GetSplineLength(), ESplineCoordinateSpace::Local);
		TransformHandler.Transform(Pos, Transform);

		*OutPosX.GetDest() = Pos.X;
		*OutPosY.GetDest() = Pos.Y;
		*OutPosZ.GetDest() = Pos.Z;
		SplineSampleParam.Advance();
		OutPosX.Advance();
		OutPosY.Advance();
		OutPosZ.Advance();
	}
}

template<typename TransformHandlerType, typename SplineSampleType>
void UNiagaraDataInterfaceSpline::SampleSplineRightVectorByUnitDistance(FVectorVMContext& Context)
{
	checkSlow(IsValid());

	TransformHandlerType TransformHandler;
	SplineSampleType SplineSampleParam(Context);
	FRegisterHandler<float> OutPosX(Context);
	FRegisterHandler<float> OutPosY(Context);
	FRegisterHandler<float> OutPosZ(Context);
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float DistanceUnitDistance = SplineSampleParam.Get();

		FVector Pos = Component->GetRightVectorAtDistanceAlongSpline(DistanceUnitDistance * Component->GetSplineLength(), ESplineCoordinateSpace::Local);
		TransformHandler.Transform(Pos, Transform);

		*OutPosX.GetDest() = Pos.X;
		*OutPosY.GetDest() = Pos.Y;
		*OutPosZ.GetDest() = Pos.Z;
		SplineSampleParam.Advance();
		OutPosX.Advance();
		OutPosY.Advance();
		OutPosZ.Advance();
	}
}

template<typename TransformHandlerType, typename SplineSampleType>
void UNiagaraDataInterfaceSpline::SampleSplineTangentByUnitDistance(FVectorVMContext& Context)
{
	checkSlow(IsValid());

	TransformHandlerType TransformHandler;
	SplineSampleType SplineSampleParam(Context);
	FRegisterHandler<float> OutPosX(Context);
	FRegisterHandler<float> OutPosY(Context);
	FRegisterHandler<float> OutPosZ(Context);
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float DistanceUnitDistance = SplineSampleParam.Get();

		FVector Pos = Component->GetTangentAtDistanceAlongSpline(DistanceUnitDistance * Component->GetSplineLength(), ESplineCoordinateSpace::Local);
		TransformHandler.Transform(Pos, Transform);

		*OutPosX.GetDest() = Pos.X;
		*OutPosY.GetDest() = Pos.Y;
		*OutPosZ.GetDest() = Pos.Z;
		SplineSampleParam.Advance();
		OutPosX.Advance();
		OutPosY.Advance();
		OutPosZ.Advance();
	}
}

template<typename TransformHandlerType, typename SplineSampleType>
void UNiagaraDataInterfaceSpline::SampleSplineDirectionByUnitDistance(FVectorVMContext& Context)
{
	checkSlow(IsValid());

	TransformHandlerType TransformHandler;
	SplineSampleType SplineSampleParam(Context);
	FRegisterHandler<float> OutPosX(Context);
	FRegisterHandler<float> OutPosY(Context);
	FRegisterHandler<float> OutPosZ(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float DistanceUnitDistance = SplineSampleParam.Get();

		FVector Pos = Component->GetDirectionAtDistanceAlongSpline(DistanceUnitDistance * Component->GetSplineLength(), ESplineCoordinateSpace::Local);
		TransformHandler.Transform(Pos, Transform);

		*OutPosX.GetDest() = Pos.X;
		*OutPosY.GetDest() = Pos.Y;
		*OutPosZ.GetDest() = Pos.Z;
		SplineSampleParam.Advance();
		OutPosX.Advance();
		OutPosY.Advance();
		OutPosZ.Advance();
	}
}

void UNiagaraDataInterfaceSpline::WriteTransform(const FMatrix& ToWrite, FVectorVMContext& Context)
{
	FRegisterHandler<float> Out00(Context);
	FRegisterHandler<float> Out01(Context);
	FRegisterHandler<float> Out02(Context);
	FRegisterHandler<float> Out03(Context);
	FRegisterHandler<float> Out04(Context);
	FRegisterHandler<float> Out05(Context);
	FRegisterHandler<float> Out06(Context);
	FRegisterHandler<float> Out07(Context);
	FRegisterHandler<float> Out08(Context);
	FRegisterHandler<float> Out09(Context);
	FRegisterHandler<float> Out10(Context);
	FRegisterHandler<float> Out11(Context);
	FRegisterHandler<float> Out12(Context);
	FRegisterHandler<float> Out13(Context);
	FRegisterHandler<float> Out14(Context);
	FRegisterHandler<float> Out15(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		*Out00.GetDest() = ToWrite.M[0][0]; Out00.Advance();
		*Out01.GetDest() = ToWrite.M[0][0]; Out01.Advance();
		*Out02.GetDest() = ToWrite.M[0][0]; Out02.Advance();
		*Out03.GetDest() = ToWrite.M[0][0]; Out03.Advance();
		*Out04.GetDest() = ToWrite.M[0][0]; Out04.Advance();
		*Out05.GetDest() = ToWrite.M[0][0]; Out05.Advance();
		*Out06.GetDest() = ToWrite.M[0][0]; Out06.Advance();
		*Out07.GetDest() = ToWrite.M[0][0]; Out07.Advance();
		*Out08.GetDest() = ToWrite.M[0][0]; Out08.Advance();
		*Out09.GetDest() = ToWrite.M[0][0]; Out09.Advance();
		*Out10.GetDest() = ToWrite.M[0][0]; Out10.Advance();
		*Out11.GetDest() = ToWrite.M[0][0]; Out11.Advance();
		*Out12.GetDest() = ToWrite.M[0][0]; Out12.Advance();
		*Out13.GetDest() = ToWrite.M[0][0]; Out13.Advance();
		*Out14.GetDest() = ToWrite.M[0][0]; Out14.Advance();
		*Out15.GetDest() = ToWrite.M[0][0]; Out15.Advance();
	}
}

template<typename PosXType, typename PosYType, typename PosZType>
void UNiagaraDataInterfaceSpline::FindClosestUnitDistanceFromPositionWS(FVectorVMContext& Context)
{
	checkSlow(IsValid());

	PosXType PosXParam(Context);
	PosYType PosYParam(Context);
	PosZType PosZParam(Context);
	FRegisterHandler<float> OutUnitDistance(Context);

	const int32 NumPoints = Component->GetSplinePointsPosition().Points.Num();
	const float FinalKeyTime = Component->GetSplinePointsPosition().Points[NumPoints - 1].InVal;

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		float PosX = PosXParam.Get();
		float PosY = PosYParam.Get();
		float PosZ = PosZParam.Get();

		FVector Pos(PosX, PosY, PosZ);

		// This first call finds the key time, but this is not in 0..1 range for the spline. 
		float KeyTime = Component->FindInputKeyClosestToWorldLocation(Pos);
		// We need to convert into the range by dividing through by the overall duration of the spline according to the keys.
		float UnitDistance = KeyTime / FinalKeyTime;

		*OutUnitDistance.GetDest() = UnitDistance;

		PosXParam.Advance();
		PosYParam.Advance();
		PosZParam.Advance();
		OutUnitDistance.Advance();
	}
}


void UNiagaraDataInterfaceSpline::GetLocalToWorld(FVectorVMContext& Context)
{
	WriteTransform(Transform, Context);
}

void UNiagaraDataInterfaceSpline::GetLocalToWorldInverseTransposed(FVectorVMContext& Context)
{
	WriteTransform(Transform, Context);
}

#undef LOCTEXT_NAMESPACE

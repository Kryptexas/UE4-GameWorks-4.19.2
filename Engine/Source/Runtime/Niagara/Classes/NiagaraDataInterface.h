// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraCommon.h"
#include "VectorVM.h"
#include "StaticMeshResources.h"
#include "Curves/RichCurve.h"
#include "NiagaraDataInterface.generated.h"

class INiagaraCompiler;
class UCurveVector;
class UCurveLinearColor;
class UCurveFloat;
class FNiagaraEffectInstance;

//////////////////////////////////////////////////////////////////////////
// Some helper classes allowing neat, init time binding of templated vm external functions.

// Adds a known type to the parameters
template<typename DirectType, typename NextBinder>
struct TNDIExplicitBinder
{
	template<typename... ParamTypes>
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo)
	{
		return NextBinder::template Bind<ParamTypes..., DirectType>(Interface, BindingInfo);
	}
};

// Binder that tests the location of an operand and adds the correct handler type to the Binding parameters.
template<int32 ParamIdx, typename DataType, typename NextBinder>
struct TNDIParamBinder
{
	template<typename... ParamTypes>
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo)
	{
		if (BindingInfo.InputParamLocations[ParamIdx])
		{
			return NextBinder::template Bind<ParamTypes..., FConstantHandler<DataType>>(Interface, BindingInfo);
		}
		else
		{
			return NextBinder::template Bind<ParamTypes..., FRegisterHandler<DataType>>(Interface, BindingInfo);
		}
	}
};

//Helper macros allowing us to define the final binding structs for each vm external function function more concisely.
#define NDI_FUNC_BINDER(ClassName, FuncName) T##ClassName##_##FuncName##Binder

#define DEFINE_NDI_FUNC_BINDER(ClassName, FuncName)\
struct NDI_FUNC_BINDER(ClassName, FuncName)\
{\
	template<typename ... ParamTypes>\
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo)\
	{\
		return FVMExternalFunction::CreateUObject(CastChecked<ClassName>(Interface), &ClassName::FuncName<ParamTypes...>);\
	}\
};

//////////////////////////////////////////////////////////////////////////

/** Base class for all Niagara data interfaces. */
UCLASS(abstract, EditInlineNew)
class NIAGARA_API UNiagaraDataInterface : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Does any initialization and error checking for this interface. Returns false if there was some error and the simulation should be disabled. */
	virtual bool PrepareForSimulation(FNiagaraEffectInstance* EffectInstance) { return true; }

	/** A tick function called before simulation if HasPreSimulationTick() returns true. Returns true if the bindings for this interface must be refreshed. */
	virtual bool PreSimulationTick(FNiagaraEffectInstance* EffectInstance, float DeltaSeconds) { return false; }

	/** If this returns true, PreSimulate() will be called on the interface prior to simulation. */
	virtual bool HasPreSimulationTick()const { return false; }

	/** Gets all the available functions for this data interface. */
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) {}

	/** Returns the delegate for the passed function signature. */
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo) { return FVMExternalFunction(); };

	/** Copies the contents of this DataInterface to another.*/
	virtual bool CopyTo(UNiagaraDataInterface* Destination);

	/** Determines if this DataInterface is the same as another.*/
	virtual bool Equals(UNiagaraDataInterface* Other);

	/** Determines if this DataInterface requires per-instance data.*/
	virtual bool RequiresPerInstanceData() const { return false; }

	/** Determines if this type definition matches to a known data interface type.*/
	static bool IsDataInterfaceType(const FNiagaraTypeDefinition& TypeDef);
};

/** Base class for curve data interfaces which facilitates handling the curve data in a standardized way. */
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Float Curve"))
class NIAGARA_API UNiagaraDataInterfaceCurveBase : public UNiagaraDataInterface
{
	GENERATED_BODY()

public:
	/** Structure to facilitate getting standardized curve information from a curve data interface. */
	struct FCurveData
	{
		FCurveData(FRichCurve* InCurve, FName InName, FLinearColor InColor)
			: Curve(InCurve)
			, Name(InName)
			, Color(InColor)
		{
		}
		/** A pointer to the curve. */
		FRichCurve* Curve;
		/** The name of the curve, unique within the data interface, which identifies the curve in the UI. */
		FName Name;
		/** The color to use when displaying this curve in the UI. */
		FLinearColor Color;
	};

	/** Gets information for all of the curves owned by this curve data interface. */
	virtual void GetCurveData(TArray<FCurveData>& OutCurveData) { }
};

/** Data Interface allowing sampling of float curves. */
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Float Curve"))
class NIAGARA_API UNiagaraDataInterfaceCurve : public UNiagaraDataInterfaceCurveBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve Curve;

	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)override;

	template<typename XParamType>
	void SampleCurve(FVectorVMContext& Context);

	virtual bool CopyTo(UNiagaraDataInterface* Destination) override;

	virtual bool Equals(UNiagaraDataInterface* Other) override;

	//~ UNiagaraDataInterfaceCurveBase interface
	virtual void GetCurveData(TArray<FCurveData>& OutCurveData) override;
};

/** Data Interface allowing sampling of vector curves. */
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Vector Curve"))
class NIAGARA_API UNiagaraDataInterfaceVectorCurve : public UNiagaraDataInterfaceCurveBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve XCurve;

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve YCurve;

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve ZCurve;

	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)override;

	template<typename XParamType>
	void SampleCurve(FVectorVMContext& Context);

	virtual bool CopyTo(UNiagaraDataInterface* Destination) override;

	virtual bool Equals(UNiagaraDataInterface* Other) override;

	//~ UNiagaraDataInterfaceCurveBase interface
	virtual void GetCurveData(TArray<FCurveData>& OutCurveData) override;
};

/** Data Interface allowing sampling of color curves. */
UCLASS(EditInlineNew, Category="Curves", meta = (DisplayName = "Color Curve"))
class NIAGARA_API UNiagaraDataInterfaceColorCurve : public UNiagaraDataInterfaceCurveBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve RedCurve;

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve GreenCurve;

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve BlueCurve;

	UPROPERTY(EditAnywhere, Category = "Curve")
	FRichCurve AlphaCurve;

	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)override;

	template<typename XParamType>
	void SampleCurve(FVectorVMContext& Context);

	virtual bool CopyTo(UNiagaraDataInterface* Destination) override;

	virtual bool Equals(UNiagaraDataInterface* Other) override;

	//~ UNiagaraDataInterfaceCurveBase interface
	virtual void GetCurveData(TArray<FCurveData>& OutCurveData) override;
};

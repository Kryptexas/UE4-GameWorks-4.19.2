// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorCommon.h"
#include "NiagaraNode.h"
#include "NiagaraNodeInput.generated.h"

class UEdGraphPin;

UENUM()
enum class ENiagaraInputNodeUsage : uint8
{
	Undefined = 0,
	Parameter,
	Attribute,
	SystemConstant
};

USTRUCT()
struct FNiagaraInputExposureOptions
{
	GENERATED_USTRUCT_BODY()
		
	FNiagaraInputExposureOptions()
		: bExposed(1)
		, bRequired(1)
		, bCanAutoBind(0)
		, bHidden(0)
	{}

	/** If this input is exposed to it's caller. */
	UPROPERTY(EditAnywhere, Category = Exposure)
	uint32 bExposed : 1;

	/** If this input is required to be bound. */
	UPROPERTY(EditAnywhere, Category = Exposure, meta = (editcondition = "bExposed"))
	uint32 bRequired : 1;

	/** If this input can auto-bind to system parameters and emitter attributes. Will never auto bind to custom parameters. */
	UPROPERTY(EditAnywhere, Category = Exposure, meta = (editcondition = "bExposed"))
	uint32 bCanAutoBind : 1;

	/** If this input should be hidden in the advanced pin section of it's caller. */
	UPROPERTY(EditAnywhere, Category = Exposure, meta = (editcondition = "bExposed"))
	uint32 bHidden : 1;
};


UCLASS(MinimalAPI)
class UNiagaraNodeInput : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Instanced, Category = Input)
	class UNiagaraDataInterface* DataInterface;

	UPROPERTY(EditAnywhere, Category = Input)
	FNiagaraVariable Input;

	UPROPERTY(VisibleAnywhere, Category = Input)
	ENiagaraInputNodeUsage Usage;

	/** Controls where this input is relative to others in the calling node. */
	UPROPERTY(EditAnywhere, Category = Input)
	int32 CallSortPriority;

	/** Controls this inputs exposure to callers. */
	UPROPERTY(EditAnywhere, Category = Input)
	FNiagaraInputExposureOptions ExposureOptions;

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin)override;
	//~ End EdGraphNode Interface

	/** Notifies the node that the type of it's input has been changed externally. */
	void NotifyInputTypeChanged();

	virtual void Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs)override;
	
	bool IsExposed()const { return ExposureOptions.bExposed; }
	bool IsRequired()const { return ExposureOptions.bExposed && ExposureOptions.bRequired; }
	bool IsHidded()const { return ExposureOptions.bExposed && ExposureOptions.bHidden; }
	bool CanAutoBind()const { return ExposureOptions.bExposed && ExposureOptions.bCanAutoBind; }

	static const FLinearColor TitleColor_Attribute;
	static const FLinearColor TitleColor_Constant;
};


// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorCommon.h"
#include "NiagaraNodeDataSetBase.h"
#include "NiagaraNodeWriteDataSet.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeWriteDataSet : public UNiagaraNodeDataSetBase
{
	GENERATED_UCLASS_BODY()

public:


	//TODO: DIRECT AND APPEND-CONSUME 
// 	UPROPERTY(EditAnywhere, Category = DataSet)
// 	ENiagaraDataSetAccessMode AccessMode;
	

	//~ Begin EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End EdGraphNode Interface

	virtual void Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs)override;
};


// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorCommon.h"
#include "NiagaraNode.h"
#include "NiagaraNodeDataSetBase.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeDataSetBase : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = DataSet)
	FNiagaraDataSetID DataSet;

	UPROPERTY(EditAnywhere, Category = Variables)
	TArray<FNiagaraVariable> Variables;
	
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin EdGraphNode Interface
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End EdGraphNode Interface

	bool InitializeFromStruct(const UStruct* ReferenceStruct);

protected:
	bool InitializeFromStructInternal(const UStruct* PayloadStruct);
};


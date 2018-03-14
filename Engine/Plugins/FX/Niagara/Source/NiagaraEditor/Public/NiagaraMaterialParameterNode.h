// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraMaterialParameterNode.generated.h"

UCLASS(MinimalAPI)
class UNiagaraMaterialParameterNode : public UNiagaraNodeFunctionCall
{
	GENERATED_BODY()

private:

	UPROPERTY()
	class UNiagaraEmitter* Emitter;

public:

	//~ Begin EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	//~ UNiagaraNodeFunctionCall interface
	virtual bool RefreshFromExternalChanges() override;
	virtual bool CanRefreshFromModuleItem() const override { return true; };

	/** Set the owning emitter of this node to get the material. */
	void SetEmitter(class UNiagaraEmitter* InEmitter);

private:		
	void GenerateScript();
	void InitializeScript(class UNiagaraScript* NewScript);
	
	/** Get a the first material of a renderproperty used by the emitter with a material. */
	class UMaterial* GetMaterialFromEmitter(class UNiagaraEmitter* InEmitter);
	class UMaterialExpressionDynamicParameter* GetMaterialExpressionDynamicParameter(class UNiagaraEmitter* InEmitter);
};

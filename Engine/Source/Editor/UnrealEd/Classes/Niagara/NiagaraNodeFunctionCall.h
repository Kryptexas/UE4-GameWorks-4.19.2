#pragma once

#include "NiagaraNodeFunctionCall.generated.h"

UCLASS()
class UNiagaraNodeFunctionCall : public UNiagaraNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Function")
	UNiagaraScript* FunctionScript;


	//Begin UObject interface
	virtual void PostLoad()override;
	//End UObject interface

	// Begin UNiagaraNode interface
	virtual void Compile(class INiagaraCompiler* Compiler, TArray<FNiagaraNodeResult>& Outputs)override;
	//End UNiagaraNode interface

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() override;
// 	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
// 	virtual FText GetTooltipText() const override;
// 	virtual FLinearColor GetNodeTitleColor() const override;
	// End EdGraphNode interface

private:
	
	/** Destroys and reallocates input and output pins. */
	void ReallocatePins();
};


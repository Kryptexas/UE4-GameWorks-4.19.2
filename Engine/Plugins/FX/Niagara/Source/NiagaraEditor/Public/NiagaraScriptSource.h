// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraScriptSourceBase.h"
#include "INiagaraCompiler.h"
#include "NiagaraParameterMapHistory.h"
#include "GraphEditAction.h"
#include "NiagaraScriptSource.generated.h"

UCLASS(MinimalAPI)
class UNiagaraScriptSource : public UNiagaraScriptSourceBase
{
	GENERATED_UCLASS_BODY()

	/** Graph for particle update expression */
	UPROPERTY()
	class UNiagaraGraph*	NodeGraph;
	
	// UObject interface
	virtual void PostLoad() override;

	// UNiagaraScriptSourceBase interface.
	virtual ENiagaraScriptCompileStatus Compile(UNiagaraScript* ScriptOwner, FString& OutGraphLevelErrorMessages) override;
	virtual bool IsSynchronized(const FGuid& InChangeId) override;
	virtual void MarkNotSynchronized() override;

	virtual UNiagaraScriptSourceBase* MakeRecursiveDeepCopy(UObject* DestOuter, TMap<const UObject*, UObject*>& ExistingConversions) const override;

	/** Determine if there are any external dependencies wrt to scripts and ensure that those dependencies are sucked into the existing package.*/
	virtual void SubsumeExternalDependencies(TMap<const UObject*, UObject*>& ExistingConversions) override;

	virtual FGuid GetChangeID();

	virtual bool IsPreCompiled() const override;
	virtual void PreCompile(UNiagaraEmitter* Emitter, bool bClearErrors = true) override;
	virtual bool GatherPreCompiledVariables(const FString& InNamespaceFilter, TArray<FNiagaraVariable>& OutVars) override;
	virtual void PostCompile() override;
	virtual bool PostLoadFromEmitter(UNiagaraEmitter& OwningEmitter) override;

	NIAGARAEDITOR_API virtual bool AddModuleIfMissing(FString ModulePath, ENiagaraScriptUsage Usage, bool& bOutFoundModule)override;

	TArray<FNiagaraParameterMapHistory>& GetPrecomputedHistories() { return PrecompiledHistories; }
	class UNiagaraGraph* GetPrecomputedNodeGraph() { return NodeGraphDeepCopy; }

private:
	void OnGraphChanged(const FEdGraphEditAction &Action);
	void OnGraphDataInterfaceChanged();

private:
	bool bIsPrecompiled;
	TArray<FNiagaraParameterMapHistory> PrecompiledHistories;

	void UpgradeForScriptRapidIterationVariables();

	UPROPERTY()
	class UNiagaraGraph* NodeGraphDeepCopy;
};

// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraNode.h"
#include "NiagaraNodeEmitter.generated.h"

class UNiagaraEffect;

/** A niagara graph node which represents an emitter and it's parameters. */
UCLASS(MinimalAPI)
class UNiagaraNodeEmitter : public UNiagaraNode
{
public:
	GENERATED_BODY()

	/** Gets the effect that owns this emitter node. */
	UNiagaraEffect* GetOwnerEffect() const;

	/** Sets the effect that owns this emitter node. */
	void SetOwnerEffect(UNiagaraEffect* InOwnerEffect);

	/** Gets the id of the emitter handle which this node represents. */
	FGuid GetEmitterHandleId() const;

	/** Sets the id of the emitter handle which this node represents. */
	void SetEmitterHandleId(FGuid InEmitterHandleId);

	//~ EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void NodeConnectionListChanged() override;

	//~ UNiagaraNode Interface
	virtual void RefreshFromExternalChanges() override;

private:
	/** Looks up the name of the emitter and converts it to text. */
	FText GetNameFromEmitter();

private:
	/** The effect that owns the emitter which is represented by this node. */
	UPROPERTY()
	UNiagaraEffect* OwnerEffect;

	/** The id of the emitter handle which points to the emitter represented by this node. */
	UPROPERTY()
	FGuid EmitterHandleId;

	/** The display name for the title bar of this node. */
	UPROPERTY()
	FText DisplayName;
};
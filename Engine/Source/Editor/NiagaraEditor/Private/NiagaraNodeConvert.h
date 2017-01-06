// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraNodeWithDynamicPins.h"

#include "SGraphPin.h"

#include "NiagaraNodeConvert.generated.h"

class UEdGraphPin;

USTRUCT()
struct FNiagaraConvertConnection
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid SourcePinId;

	UPROPERTY()
	TArray<FName> SourcePath;

	UPROPERTY()
	FGuid DestinationPinId;

	UPROPERTY()
	TArray<FName> DestinationPath;

	FNiagaraConvertConnection()
	{
	}

	FNiagaraConvertConnection(FGuid InSourcePinId, const TArray<FName>& InSourcePath, FGuid InDestinationPinId, const TArray<FName>& InDestinationPath)
		: SourcePinId(InSourcePinId)
		, SourcePath(InSourcePath)
		, DestinationPinId(InDestinationPinId)
		, DestinationPath(InDestinationPath)
	{
	}
};


/** A node which allows the user to build a set of arbitrary output types from an arbitrary set of input types by connecting their inner components. */
UCLASS()
class UNiagaraNodeConvert : public UNiagaraNodeWithDynamicPins
{
public:
	GENERATED_BODY()

	//~ UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual void  AutowireNewNode(UEdGraphPin* FromPin)override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	//~ UNiagaraNode interface
	virtual void Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs) override;

	/** Gets the nodes inner connection. */
	TArray<FNiagaraConvertConnection>& GetConnections();

	/** Initializes this node as a swizzle by component string. */
	void InitAsSwizzle(FString Swiz);

	/** Initializes this node as a make node based on an output type. */
	void InitAsMake(FNiagaraTypeDefinition Type);

	/** Initializes this node as a break node based on an input tye. */
	void InitAsBreak(FNiagaraTypeDefinition Type);

	bool InitConversion(UEdGraphPin* FromPin, UEdGraphPin* ToPin);
private:
	//~ EdGraphNode interface
	virtual void OnPinRemoved(UEdGraphPin* Pin) override;

private:

	//A swizzle string set externally to instruct the autowiring code.
	UPROPERTY()
	FString AutowireSwizzle;

	//A type def used when auto wiring up the convert node.
	UPROPERTY()
	FNiagaraTypeDefinition AutowireMakeType;
	UPROPERTY()
	FNiagaraTypeDefinition AutowireBreakType;

	/** The internal connections for this node. */
	UPROPERTY()
	TArray<FNiagaraConvertConnection> Connections;
};

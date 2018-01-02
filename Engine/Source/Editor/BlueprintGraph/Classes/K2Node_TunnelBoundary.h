// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "K2Node.h"
#include "K2Node_TunnelBoundary.generated.h"

class FKismetCompilerContext;
class UK2Node_Tunnel;

UENUM()
enum class ETunnelBoundaryType : uint8
{
	Unknown = 0,
	EntrySite,
	InputSite,
	OutputSite
};

UCLASS()
class BLUEPRINTGRAPH_API UK2Node_TunnelBoundary : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Base Name */
	UPROPERTY(Transient)
	FName BaseName;

	/** Node Type */
	UPROPERTY(Transient)
	ETunnelBoundaryType TunnelBoundaryType;

public:

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface

	//~ Begin K2Node Interface
	virtual class FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;
	virtual int32 GetNodeRefreshPriority() const override { return EBaseNodeRefreshPriority::Low_UsesDependentWildcard; }
	//~ End UK2Node Interface

	/** Returns the type of tunnel boundary this node is */
	ETunnelBoundaryType GetTunnelBoundaryType() const { return TunnelBoundaryType; }

	/** Set node attributes based on the tunnel source node */
	void SetNodeAttributes(const UK2Node_Tunnel* SourceNode);
};
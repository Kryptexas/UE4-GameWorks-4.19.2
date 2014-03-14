// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeMaterialResult : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeMaterialResult){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UMaterialGraphNode_Root* InNode);

	// SGraphNode interface
	virtual void CreatePinWidgets() OVERRIDE;
	// End of SGraphNode interface

	// SNodePanel::SNode interface
	virtual void MoveTo(const FVector2D& NewPosition) OVERRIDE;
	// End of SNodePanel::SNode interface

private:
	UMaterialGraphNode_Root* RootNode;
};

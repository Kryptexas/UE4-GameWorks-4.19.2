// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphNode.h"
#include "SMenuAnchor.h"
#include "UICommandList.h"

/** A graph node widget representing a niagara input node. */
class SNiagaraGraphNodeCustomHlsl : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SNiagaraGraphNodeCustomHlsl) {}
	SLATE_END_ARGS();

	SNiagaraGraphNodeCustomHlsl();

	void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode);

	//~ SGraphNode api
	virtual TSharedRef<SWidget> CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle) override;
	virtual bool IsNameReadOnly() const override;
	virtual void RequestRenameOnSpawn() override;

	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox);
protected:


};
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeOp.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeOp : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

	/** Index of operation */
	UPROPERTY()
	uint8 OpIndex;

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End EdGraphNode interface

	/** Names to use for each input pin. */
	UNREALED_API static const TCHAR* InPinNames[3];
};



#if 0

class SGraphNodeNiagara : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeNiagara){}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, class UNiagaraNode* InNode);

	// SGraphNode interface
	virtual void CreatePinWidgets() override;
	// End of SGraphNode interface

	// SNodePanel::SNode interface
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter) override;
	// End of SNodePanel::SNode interface

	UNiagaraNode* GetNiagaraGraphNode() const { return NiagaraNode; }

protected:
	// SGraphNode interface
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;
	virtual void SetDefaultTitleAreaWidget(TSharedRef<SOverlay> DefaultTitleAreaWidget) override;
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	// End of SGraphNode interface

	/** Creates a preview viewport if necessary */
	//TSharedRef<SWidget> CreatePreviewWidget();

	/** Returns visibility of Expression Preview viewport */
	//EVisibility ExpressionPreviewVisibility() const;

	/** Show/hide Expression Preview */
	//void OnExpressionPreviewChanged(const ESlateCheckBoxState::Type NewCheckedState);

	/** hidden == unchecked, shown == checked */
	//ESlateCheckBoxState::Type IsExpressionPreviewChecked() const;

	/** Up when shown, down when hidden */
	//const FSlateBrush* GetExpressionPreviewArrow() const;

private:
	/** Slate viewport for rendering preview via custom slate element */
	//TSharedPtr<FPreviewViewport> PreviewViewport;

	/** Cached material graph node pointer to avoid casting */
	UNiagaraNode* NiagaraNode;
};

#endif
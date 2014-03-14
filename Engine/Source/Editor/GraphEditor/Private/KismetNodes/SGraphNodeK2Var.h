// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SGraphNodeK2Var_h__
#define __SGraphNodeK2Var_h__

class SGraphNodeK2Var : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeK2Var){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, UK2Node* InNode );

	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const;

	// SGraphNode interface
	virtual void UpdateGraphNode() OVERRIDE;
	// End of SGraphNode interface

protected:
	FSlateColor GetVariableColor() const;
};

#endif // __SGraphNodeK2Var_h__
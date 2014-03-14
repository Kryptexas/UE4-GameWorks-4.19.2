// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SGraphNodeK2Terminator_h__
#define __SGraphNodeK2Terminator_h__

class SGraphNodeK2Terminator : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeK2Terminator){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, UK2Node* InNode );

	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const;

protected:
	virtual void UpdateGraphNode() OVERRIDE;
};

#endif // __SGraphNodeK2Terminator_h__
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeK2Timeline : public SGraphNodeK2Default
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeK2Timeline){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UK2Node_Timeline* InNode);

	// SNodePanel::SNode interface
	void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const OVERRIDE;
	// End of SNodePanel::SNode interface
};

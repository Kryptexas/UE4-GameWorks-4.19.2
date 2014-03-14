// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeCallParameterCollectionFunction : public SGraphNodeK2Default
{
protected:

	// SGraphNode interface
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const OVERRIDE;
	// End of SGraphNode interface
};

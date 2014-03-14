// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/IToolkit.h"
#include "Toolkits/AssetEditorToolkit.h"


/** BT Editor public interface */
class IBehaviorTreeEditor : public FAssetEditorToolkit
{

public:
	virtual uint32 GetSelectedNodesCount() const = 0;
	
	virtual void InitializeDebuggerState(class FBehaviorTreeDebugger* ParentDebugger) const = 0;
};



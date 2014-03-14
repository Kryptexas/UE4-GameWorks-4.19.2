// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/IToolkit.h"
#include "Toolkits/AssetEditorToolkit.h"


/** DataTable Editor public interface */
class IDataTableEditor : public FAssetEditorToolkit
{

public:

	// ...

	/** Called after a data table was reloaded */
	virtual void OnDataTableReloaded(){}

};



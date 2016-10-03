// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMisc.h"

class FMeshHierarchyCmd : private FSelfRegisteringExec
{
public:
	/** Console commands, see embeded usage statement **/
	virtual bool Exec(UWorld*, const TCHAR* Cmd, FOutputDevice& Ar) override;
};
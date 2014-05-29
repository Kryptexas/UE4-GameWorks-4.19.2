// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIResourceInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UAIResourceInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IAIResourceInterface
{
	GENERATED_IINTERFACE_BODY()

	/** If resource is lockable lock it with indicating source */
	virtual void LockResource(EAILockSource::Type LockSource) {}

	/** clear resource lock of given origin */
	virtual void ClearResourceLock(EAILockSource::Type LockSource) {}

	/** Force-clears all locks on resource */
	virtual void ForceUnlockResource() {}
	
	/** check whether resource is currently locked */
	virtual bool IsResourceLocked() const {return false;}
};

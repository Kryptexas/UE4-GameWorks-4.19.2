// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogFunctionalTest, Log, All);

class FFuncTestManager : public IFuncTestManager, public TSharedFromThis<FFuncTestManager>
{
public:
		
	/** Triggers in sequence all functional tests found on the level.*/
	virtual void RunAllTestsOnMap(bool bClearLog, bool bRunLooped);
	
	virtual bool IsRunning() const;
	
	virtual void SetScript(class UFunctionalTestingManager* NewScript);

	virtual void SetLooping(const bool bLoop);

protected:

	TWeakObjectPtr<class UFunctionalTestingManager> TestScript;
};
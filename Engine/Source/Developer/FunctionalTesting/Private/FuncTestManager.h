// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "IFuncTestManager.h"

class UFunctionalTestingManager;

DECLARE_LOG_CATEGORY_EXTERN(LogFunctionalTest, Log, All);

class FFuncTestManager : public IFuncTestManager, public TSharedFromThis<FFuncTestManager>
{
public:

	FFuncTestManager();
	virtual ~FFuncTestManager();
		
	/** Triggers in sequence all functional tests found on the level.*/
	virtual void RunAllTestsOnMap(bool bClearLog, bool bRunLooped) override;

	virtual void RunTestOnMap(const FString& TestName, bool bClearLog, bool bRunLooped) override;
	
	virtual void MarkPendingActivation() override;

	virtual bool IsActivationPending() const override;

	virtual bool IsRunning() const override;

	virtual bool IsFinished() const override;
	
	virtual void SetScript(class UFunctionalTestingManager* NewScript) override;

	virtual class UFunctionalTestingManager* GetCurrentScript();

	virtual void SetLooping(const bool bLoop) override;

protected:
	UWorld* GetTestWorld();

	void OnGetAssetTagsForWorld(const UWorld* World, TArray<UObject::FAssetRegistryTag>& OutTags);

protected:

	TWeakObjectPtr<class UFunctionalTestingManager> TestScript;
	bool bPendingActivation;
};

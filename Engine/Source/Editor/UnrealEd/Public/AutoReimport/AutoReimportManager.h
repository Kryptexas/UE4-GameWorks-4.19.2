// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AutoReimportManager.generated.h"

/* Deals with auto reimporting of objects when the objects file on disk is modified*/
UCLASS(config=Editor, transient)
class UNREALED_API UAutoReimportManager : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	
	~UAutoReimportManager();

	/** Initialize during engine startup */
	void Initialize();

private:

	/** UObject Interface */
	virtual void BeginDestroy() override;

	/** Private implementation of the reimport manager */
	TSharedPtr<class FAutoReimportManager> Implementation;
};

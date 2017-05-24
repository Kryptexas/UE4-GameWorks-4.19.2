// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "PlatformFeatures.h"
#if PLATFORM_HTML5_BROWSER
#include "HTML5SaveGameSystem.h"
#endif

class FHTML5PlatformFeatures : public IPlatformFeaturesModule
{
public:
#if PLATFORM_HTML5_BROWSER
	virtual class ISaveGameSystem* GetSaveGameSystem() override
	{
		static FHTML5SaveGameSystem HTML5SaveGameSystem;
		return &HTML5SaveGameSystem;
	}
#endif
};

IMPLEMENT_MODULE(FHTML5PlatformFeatures, HTML5PlatformFeatures);

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CharacterAIPrivatePCH.h"
#include "CharacterAIModule.h"

//////////////////////////////////////////////////////////////////////////
// FCharacterAIModule

class FCharacterAIModule : public ICharacterAIModuleInterface
{
public:
	virtual void StartupModule() override
	{
		check(GConfig);		
	}

	virtual void ShutdownModule() override
	{
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FCharacterAIModule, CharacterAI);
DEFINE_LOG_CATEGORY(LogCharacterAI);

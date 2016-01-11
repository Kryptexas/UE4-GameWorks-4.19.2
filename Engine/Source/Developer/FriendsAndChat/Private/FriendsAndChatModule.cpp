// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"

/**
 * Implements the FriendsAndChat module.
 */
class FFriendsAndChatModule
	: public IFriendsAndChatModule
{
public:

	// IFriendsAndChatModule interface

	virtual TSharedRef<IFriendsAndChatManager> GetFriendsAndChatManager(FName MCPInstanceName) override
	{
		if(MCPInstanceName == TEXT(""))
		{
			if (!DefaultManager.IsValid())
			{
				DefaultManager = MakeShareable(new FFriendsAndChatManager());
				DefaultManager->Initialize();
			}
			return DefaultManager.ToSharedRef();
		}
		else
		{
			TSharedRef<FFriendsAndChatManager>* FoundManager = ManagerMap.Find(MCPInstanceName);
			if(FoundManager != nullptr)
			{
				return *FoundManager;
			}
		}

		TSharedRef<FFriendsAndChatManager> NewManager = MakeShareable(new FFriendsAndChatManager());
		NewManager->Initialize();
		ManagerMap.Add(MCPInstanceName, NewManager);
		return NewManager;
	}

public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
		if(DefaultManager.IsValid())
		{
			DefaultManager.Reset();
		}
		ManagerMap.Reset();
	}

private:

	TSharedPtr<FFriendsAndChatManager> DefaultManager;
	TMap<FName, TSharedRef<FFriendsAndChatManager>> ManagerMap;
};


IMPLEMENT_MODULE( FFriendsAndChatModule, FriendsAndChat );

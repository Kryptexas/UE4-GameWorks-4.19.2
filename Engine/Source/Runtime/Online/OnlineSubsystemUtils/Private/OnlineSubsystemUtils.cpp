// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "SocketSubsystem.h"
#include "ModuleManager.h"

#include "IPAddress.h"
#include "OnlineSubsystemUtils.generated.inl"
#include "NboSerializer.h"

#include "SoundDefinitions.h"

// Testing classes
#include "Tests/TestFriendsInterface.h"
#include "Tests/TestSessionInterface.h"
#include "Tests/TestCloudInterface.h"
#include "Tests/TestLeaderboardInterface.h"
#include "Tests/TestTimeInterface.h"
#include "Tests/TestIdentityInterface.h"
#include "Tests/TestTitleFileInterface.h"
#include "Tests/TestEntitlementsInterface.h"
#include "Tests/TestAchievementsInterface.h"
#include "Tests/TestSharingInterface.h"
#include "Tests/TestUserInterface.h"
#include "Tests/TestMessageInterface.h"

IMPLEMENT_MODULE( FOnlineSubsystemUtilsModule, OnlineSubsystemUtils );

/**
 * Called right after the module DLL has been loaded and the module object has been created
 * Overloaded to allow the default subsystem a chance to load
 */
void FOnlineSubsystemUtilsModule::StartupModule()
{
	
}

/**
 * Called before the module is unloaded, right before the module object is destroyed.
 * Overloaded to shut down all loaded online subsystems
 */
void FOnlineSubsystemUtilsModule::ShutdownModule()
{
	
}

UAudioComponent* CreateVoiceAudioComponent(uint32 SampleRate)
{
	UAudioComponent* AudioComponent = NULL;
	if (GEngine != NULL)
	{
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if (AudioDevice != NULL)
		{
			USoundWaveStreaming* SoundStreaming = NULL;
			SoundStreaming = ConstructObject<USoundWaveStreaming>(USoundWaveStreaming::StaticClass());
			SoundStreaming->SampleRate = SampleRate;
			SoundStreaming->NumChannels = 1;
			SoundStreaming->Duration = INDEFINITELY_LOOPING_DURATION;
			SoundStreaming->bLooping = false;

			AudioComponent = AudioDevice->CreateComponent(SoundStreaming, NULL, NULL, false);
			AudioComponent->bIsUISound = true;
			AudioComponent->SetVolumeMultiplier(1.5f);
		}
	}

	return AudioComponent;
}

/**
 * Exec handler that routes online specific execs to the proper subsystem
 *
 * @param InWorld World context
 * @param Cmd 	the exec command being executed
 * @param Ar 	the archive to log results to
 *
 * @return true if the handler consumed the input, false to continue searching handlers
 */
static bool OnlineExec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	bool bWasHandled = false;

	// Ignore any execs that don't start with ONLINE
	if (FParse::Command(&Cmd, TEXT("ONLINE")))
	{
		FString SubName;
		FParse::Value(Cmd, TEXT("Sub="), SubName);
		// Allow for either Sub=<platform> or Subsystem=<platform>
		if (SubName.Len() > 0)
		{
			Cmd += FCString::Strlen(TEXT("Sub=")) + SubName.Len();
		}
		else
		{ 
			FParse::Value(Cmd, TEXT("Subsystem="), SubName);
			if (SubName.Len() > 0)
			{
				Cmd += FCString::Strlen(TEXT("Subsystem=")) + SubName.Len();
			}
		}
		IOnlineSubsystem* OnlineSub = NULL;
		// If the exec requested a specific subsystem, the grab that one for routing
		if (SubName.Len())
		{
			OnlineSub = IOnlineSubsystem::Get(*SubName);
		}
		// Otherwise use the default subsystem and route to that
		else
		{
			OnlineSub = IOnlineSubsystem::Get();
		}
		if (OnlineSub != NULL)
		{
			bWasHandled = OnlineSub->Exec(Cmd, Ar);
			// If this wasn't handled, see if this is a testing request
			if (!bWasHandled)
			{
				if (FParse::Command(&Cmd, TEXT("TEST")))
				{
					if (FParse::Command(&Cmd, TEXT("FRIENDS")))
					{
						TArray<FString> Invites;
						for (FString FriendId=FParse::Token(Cmd, false); !FriendId.IsEmpty(); FriendId=FParse::Token(Cmd, false))
						{
							Invites.Add(FriendId);
						}
						// This class deletes itself once done
						(new FTestFriendsInterface(SubName))->Test(Invites);
						bWasHandled = true;
					}
					// Spawn the object that will exercise all of the session methods as host
					else if (FParse::Command(&Cmd, TEXT("SESSIONHOST")))
					{
						bool bTestLAN = FParse::Command(&Cmd, TEXT("LAN")) ? true : false;
						bool bTestPresence = FParse::Command(&Cmd, TEXT("PRESENCE")) ? true : false;

						// This class deletes itself once done
						(new FTestSessionInterface(SubName, true))->Test(InWorld, bTestLAN, bTestPresence);
						bWasHandled = true;
					}
					// Spawn the object that will exercise all of the session methods as client
					else if (FParse::Command(&Cmd, TEXT("SESSIONCLIENT")))
					{
						bool bTestLAN = FParse::Command(&Cmd, TEXT("LAN")) ? true : false;
						bool bTestPresence = FParse::Command(&Cmd, TEXT("PRESENCE")) ? true : false;

						// This class deletes itself once done
						(new FTestSessionInterface(SubName, false))->Test(InWorld, bTestLAN, bTestPresence);
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("CLOUD")))
					{
						// This class deletes itself once done
						(new FTestCloudInterface(SubName))->Test();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("LEADERBOARDS")))
					{
						// This class deletes itself once done
						(new FTestLeaderboardInterface(SubName))->Test();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("TIME")))
					{
						// This class deletes itself once done
						(new FTestTimeInterface(SubName))->Test();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("IDENTITY")))
					{
						FString Id = FParse::Token(Cmd, false);
						FString Auth = FParse::Token(Cmd, false);
						FString Type = FParse::Token(Cmd, false);

						// This class deletes itself once done
						(new FTestIdentityInterface(SubName))->Test( FOnlineAccountCredentials(Type, Id, Auth) );
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("UNIQUEIDREPL")))
					{
						extern void TestUniqueIdRepl();
						TestUniqueIdRepl();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("KEYVALUEPAIR")))
					{
						extern void TestKeyValuePairs();
						TestKeyValuePairs();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("TITLEFILE")))
					{
						// This class deletes itself once done
						(new FTestTitleFileInterface(SubName))->Test();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("ENTITLEMENTS")))
					{
						// This class also deletes itself once done
						(new FTestEntitlementsInterface(SubName))->Test();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("ACHIEVEMENTS")))
					{
						// This class also deletes itself once done
						(new FTestAchievementsInterface(SubName))->Test();
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("SHARING")))
					{
						bool bTestWithImage = FParse::Command(&Cmd, TEXT("IMG"));

						// This class also deletes itself once done
						(new FTestSharingInterface(SubName))->Test(bTestWithImage);
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("USER")))
					{
						TArray<FString> UserIds;
						for (FString Id=FParse::Token(Cmd, false); !Id.IsEmpty(); Id=FParse::Token(Cmd, false))
						{
							UserIds.Add(Id);
						}
						// This class also deletes itself once done
						(new FTestUserInterface(SubName))->Test(UserIds);
						bWasHandled = true;
					}
					else if (FParse::Command(&Cmd, TEXT("MESSAGE")))
					{
						TArray<FString> RecipientIds;
						for (FString UserId=FParse::Token(Cmd, false); !UserId.IsEmpty(); UserId=FParse::Token(Cmd, false))
						{
							RecipientIds.Add(UserId);
						}
						// This class also deletes itself once done
						(new FTestMessageInterface(SubName))->Test(RecipientIds);
						bWasHandled = true;
					}
				}
			}
		}
	}
	return bWasHandled;
}

/** Our entry point for all online exec routing */
FStaticSelfRegisteringExec OnlineExecRegistration(OnlineExec);


// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "ModuleManager.h"
#include "Online.h"
#include "TestAchievementsInterface.h"


/**
 * The process of this test is:
 *
 * [Read Achievements] -> [ReadAchievementsDelegate]
 * -> [Read Achievement Descriptions] -> [Read Achievement Descriptions Delegate]
 * -> -> [Write Achievements] -> [Achievement Unlocked Delegate]
 * -> -> -> [delete self]
 */


void FTestAchievementsInterface::Test(void)
{
	UE_LOG(LogOnline, Display, TEXT("FTestAchievementsInterface::Test"));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(FName(*SubsystemName));
	check(OnlineSub); 

	if (OnlineSub->GetIdentityInterface().IsValid())
	{
		UserId = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(0);

		OnlineAchievements = OnlineSub->GetAchievementsInterface();
	}

	if( OnlineAchievements.IsValid() && UserId.IsValid() )
	{
		ReadAchievements();
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: OSS [%s] does not have a valid achievement interface or identity interface for this test to run."), *SubsystemName );
		delete this;
	}
}


void FTestAchievementsInterface::ReadAchievements()
{
	// Add our delegate and read achievements
	OnlineAchievements->AddOnAchievementsReadDelegate(OnAchievementsReadDelegate);
	OnlineAchievements->ReadAchievements( *UserId.Get() );
}


void FTestAchievementsInterface::OnAchievementsRead(const FUniqueNetId& PlayerId, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Display, TEXT("FTestAchievementsInterface::OnAchievementsRead"));

	// Clear our delegate and if successful read achievements
	OnlineAchievements->ClearOnAchievementsReadDelegate(OnAchievementsReadDelegate);

	if (bWasSuccessful)
	{
		UE_LOG(LogOnline, Display, TEXT("Loaded Achievements"));

		TArray<FOnlineAchievement> PlayerAchievements;
		if (!OnlineAchievements->GetAchievements(*UserId.Get(), PlayerAchievements) || PlayerAchievements.Num() == 0)
		{
			UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: Either GetAchievements() failed or number of achievements is 0"));
			delete this;
			return;
		}

		UE_LOG(LogOnline, Display, TEXT("Number of Achievements: %d"), PlayerAchievements.Num());
		for(int32 Idx = 0; Idx < PlayerAchievements.Num(); ++Idx)
		{
			UE_LOG(LogOnline, Display, TEXT(" Achievement %d: %s"), Idx, *PlayerAchievements[ Idx ].ToDebugString());
		}

		ReadAchievementDescriptions();
	}
	else
	{
		// if our test failed, be sure to clean itself up
		UE_LOG(LogOnline, Display, TEXT("TEST FAILED: Failed to Load Achievements"));
		delete this;
	}
}


void FTestAchievementsInterface::ReadAchievementDescriptions()
{
	// Add our delegate and read achievement descriptions
	OnlineAchievements->AddOnAchievementDescriptionsReadDelegate(OnAchievementDescriptionsReadDelegate);
	OnlineAchievements->ReadAchievementDescriptions( *UserId.Get() );
}


void FTestAchievementsInterface::OnAchievementDescriptionsRead(const FUniqueNetId& PlayerId, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Display, TEXT("FTestAchievementsInterface::OnAchievementsRead"));

	// Clear our delegate and if successful write achievements
	OnlineAchievements->ClearOnAchievementDescriptionsReadDelegate(OnAchievementDescriptionsReadDelegate);
	if (bWasSuccessful)
	{
		UE_LOG(LogOnline, Display, TEXT("Loaded Achievement descriptions"));

		TArray<FOnlineAchievement> PlayerAchievements;
		if (!OnlineAchievements->GetAchievements(*UserId.Get(), PlayerAchievements) || PlayerAchievements.Num() == 0)
		{
			UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: Either GetAchievements() failed or number of achievements is 0"));
			delete this;
			return;
		}

		for(int32 Idx = 0; Idx < PlayerAchievements.Num(); ++Idx)
		{
			FOnlineAchievementDesc Desc;
			if (!OnlineAchievements->GetAchievementDescription(PlayerAchievements[ Idx ].Id, Desc))
			{
				UE_LOG(LogOnline, Warning, TEXT("Failed to GetAchievementDescription() for achievement '%s'"),
					*PlayerAchievements[ Idx ].Id
					);
				delete this;
				return;
			}
			UE_LOG(LogOnline, Display, TEXT(" Descriptor for achievement '%s': %s"), *PlayerAchievements[ Idx ].Id, *Desc.ToDebugString());
		}
		
		WriteAchievements();
	}
	else
	{
		// if our test failed, be sure to clean itself up
		UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: Failed to Load Achievement descriptions"));
		delete this;
	}
}


void FTestAchievementsInterface::WriteAchievements()
{
	WriteObject = MakeShareable(new FOnlineAchievementsWrite());
	FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();

	TArray<FOnlineAchievement> PlayerAchievements;
	if (!OnlineAchievements->GetAchievements(*UserId.Get(), PlayerAchievements) || PlayerAchievements.Num() == 0)
	{
		UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: Either GetAchievements() failed or number of achievements is 0"));
		delete this;
		return;
	}

	if (PlayerAchievements.Num() >= 3)
	{
		// report progress on first, last and median achievements
		WriteObject->SetFloatStat(*PlayerAchievements[ 0 ].Id, 1.0f);
		WriteObject->SetFloatStat(*PlayerAchievements[ PlayerAchievements.Num() - 1 ].Id, 50.0f);
		WriteObject->SetFloatStat(*PlayerAchievements[ PlayerAchievements.Num() / 2 ].Id, 100.0f);
	}
	else
	{
		// report progress on all achievements
		for (int32 IdxAch = 0; IdxAch < PlayerAchievements.Num(); ++IdxAch)
		{
			WriteObject->SetFloatStat(*PlayerAchievements[ IdxAch ].Id, 50.0f);
		}
	}

	// Add our delegate and write some achievements to the server
	OnlineAchievements->AddOnAchievementsWrittenDelegate( OnAchievementsWrittenDelegate );
	OnlineAchievements->AddOnAchievementUnlockedDelegate( OnAchievementUnlockedDelegate );
	OnlineAchievements->WriteAchievements( *UserId.Get(), WriteObjectRef );
}


void FTestAchievementsInterface::OnAchievementsWritten(const FUniqueNetId& PlayerId, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Display, TEXT("FTestAchievementsInterface::OnAchievementsWritten( bWasSuccessful = %s )"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	// Clear our delegate and delete this test.
	OnlineAchievements->ClearOnAchievementsWrittenDelegate(OnAchievementsWrittenDelegate);
	OnlineAchievements->ClearOnAchievementUnlockedDelegate(OnAchievementUnlockedDelegate);

	if (bWasSuccessful && WriteObject->WriteState == EOnlineAsyncTaskState::Done)
	{
#if !UE_BUILD_SHIPPING
		OnlineAchievements->ResetAchievements(*UserId.Get());
#endif // !UE_BUILD_SHIPPING
		// TODO: spin for a while?
		UE_LOG(LogOnline, Display, TEXT("TEST COMPLETED SUCCESSFULLY."));
	}
	else
	{
		if (WriteObject->WriteState != EOnlineAsyncTaskState::Done)
		{
			UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: WriteObject->WriteState is not in state %s, but instead %s"),
				EOnlineAsyncTaskState::ToString(EOnlineAsyncTaskState::Done),
				EOnlineAsyncTaskState::ToString(WriteObject->WriteState)
				);
		}

		if (!bWasSuccessful)
		{
			UE_LOG(LogOnline, Warning, TEXT("TEST FAILED: Write did not complete successfully") );
		}
	}

	delete this;
}

void FTestAchievementsInterface::OnAchievementsUnlocked(const FUniqueNetId& PlayerId, const FString& AchievementId)
{
	UE_LOG(LogOnline, Display, TEXT("Achievement Unlocked - %s"), *AchievementId);
}
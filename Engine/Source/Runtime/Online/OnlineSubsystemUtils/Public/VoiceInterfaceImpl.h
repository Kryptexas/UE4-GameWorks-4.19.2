// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VoiceInterface.h"
#include "VoicePacketImpl.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystemUtilsPackage.h"

/**
 * The generic implementation of the voice interface 
 */

class ONLINESUBSYSTEMUTILS_API FOnlineVoiceImpl : public IOnlineVoice
{
	/** Reference to the main online subsystem */
	class IOnlineSubsystem* OnlineSubsystem;
	/** Reference to the sessions interface */
	class IOnlineSession* SessionInt;
	/** Reference to the profile interface */
	class IOnlineIdentity* IdentityInt;
	/** Reference to the voice engine for acquiring voice data */
	class IVoiceEngine* VoiceEngine;

	/** Maximum permitted local talkers */
	int32 MaxLocalTalkers;
	/** Maximum permitted remote talkers */
	int32 MaxRemoteTalkers;

	/** State of all possible local talkers */
	TArray<FLocalTalker> LocalTalkers;
	/** State of all possible remote talkers */
	TArray<FRemoteTalker> RemoteTalkers;
	/** Remote players locally muted */
	TArray<FUniqueNetIdString> MuteList;

	/** Time to wait for new data before triggering "not talking" */
	float VoiceNotificationDelta;

	/** Buffered voice data I/O */
	FVoiceDataImpl VoiceData;

	/**
	 * Finds a remote talker in the cached list
	 *
	 * @param UniqueId the net id of the player to search for
	 *
	 * @return pointer to the remote talker or NULL if not found
	 */
	struct FRemoteTalker* FindRemoteTalker(const FUniqueNetId& UniqueId);

PACKAGE_SCOPE:

	FOnlineVoiceImpl() :
		OnlineSubsystem(NULL),
		SessionInt(NULL),
		IdentityInt(NULL),
		VoiceEngine(NULL),
		MaxLocalTalkers(MAX_SPLITSCREEN_TALKERS),
		MaxRemoteTalkers(MAX_REMOTE_TALKERS),
		VoiceNotificationDelta(0.0f)
	{};

	// IOnlineVoice
	virtual bool Init() OVERRIDE;
	void ProcessMuteChangeNotification() OVERRIDE;

	/**
	 * Processes any talking delegates that need to be fired off
	 *
	 * @param DeltaTime the amount of time that has elapsed since the last tick
	 */
	void ProcessTalkingDelegates(float DeltaTime);

	/**
	 * Reads any data that is currently queued
	 */
	void ProcessLocalVoicePackets();

	/**
	 * Submits network packets to audio system for playback
	 */
	void ProcessRemoteVoicePackets();

	/**
	 * Figures out which remote talkers need to be muted for a given local talker
	 *
	 * @param TalkerIndex the talker that needs the mute list checked for
	 * @param PlayerController the player controller associated with this talker
	 */
	void UpdateMuteListForLocalTalker(int32 TalkerIndex, class APlayerController* PlayerController);

public:

	/** Constructor */
	FOnlineVoiceImpl(class IOnlineSubsystem* InOnlineSubsystem);

	/** Virtual destructor to force proper child cleanup */
	virtual ~FOnlineVoiceImpl() {}

	// IOnlineVoice
	virtual void StartNetworkedVoice(uint8 LocalUserNum) OVERRIDE;
	virtual void StopNetworkedVoice(uint8 LocalUserNum) OVERRIDE;
    virtual bool RegisterLocalTalker(uint32 LocalUserNum) OVERRIDE;
	virtual void RegisterLocalTalkers() OVERRIDE;
    virtual bool UnregisterLocalTalker(uint32 LocalUserNum) OVERRIDE;
	virtual void UnregisterLocalTalkers() OVERRIDE;
    virtual bool RegisterRemoteTalker(const FUniqueNetId& UniqueId) OVERRIDE;
    virtual bool UnregisterRemoteTalker(const FUniqueNetId& UniqueId) OVERRIDE;
	virtual void RemoveAllRemoteTalkers() OVERRIDE;
    virtual bool IsHeadsetPresent(uint32 LocalUserNum) OVERRIDE;
    virtual bool IsLocalPlayerTalking(uint32 LocalUserNum) OVERRIDE;
	virtual bool IsRemotePlayerTalking(const FUniqueNetId& UniqueId) OVERRIDE;
	bool IsMuted(uint32 LocalUserNum, const FUniqueNetId& UniqueId) const OVERRIDE;
	bool MuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) OVERRIDE;
	bool UnmuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) OVERRIDE;
	virtual TSharedPtr<class FVoicePacket> SerializeRemotePacket(FArchive& Ar) OVERRIDE;
	virtual TSharedPtr<class FVoicePacket> GetLocalPacket(uint32 LocalUserNum) OVERRIDE;
	virtual int32 GetNumLocalTalkers() OVERRIDE { return LocalTalkers.Num(); };
	virtual void ClearVoicePackets() OVERRIDE;
	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual FString GetVoiceDebugState() const OVERRIDE;
};

typedef TSharedPtr<FOnlineVoiceImpl, ESPMode::ThreadSafe> FOnlineVoiceImplPtr;

// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineDelegateMacros.h"
#include "OnlineSubsystemTypes.h"

/**
 * Account credentials needed to sign in to an online service
 */
class FOnlineAccountCredentials
{
public:

	/** Type of account. Needed to identity the auth method to use (epic, internal, facebook, etc) */
	FString Type;
	/** Id of the user logging in (email, display name, facebook id, etc) */
	FString Id;
	/** Credentials of the user logging in (password or auth token) */
	FString Token;

	/**
	 * Equality operator
	 */
	bool operator==(const FOnlineAccountCredentials& Other) const
	{
		return Other.Type == Type && 
			Other.Id == Id;
	}

	/**
	 * Constructor
	 */
	FOnlineAccountCredentials(const FString& InType, const FString& InId, const FString& InToken) :
		Type(InType),
		Id(InId),
		Token(InToken)
	{
	}

	/**
	 * Constructor
	 */
	FOnlineAccountCredentials()
	{
	}
};

/**
 * Delegate called when a player logs in/out
 *
 * @param LocalUserNum the controller number of the associated user
 * @param LocalUserNum the player that logged in/out
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoginChanged, int32);
typedef FOnLoginChanged::FDelegate FOnLoginChangedDelegate;

/**
 * Delegate called when a player's status changes but doesn't change profiles
 *
 * @param LocalUserNum the controller number of the associated user
 * @param NewStatus the new login status for the user
 * @param NewId the new id to associate with the user
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLoginStatusChanged, int32, ELoginStatus::Type, const FUniqueNetId&);
typedef FOnLoginStatusChanged::FDelegate FOnLoginStatusChangedDelegate;

/**
 * Called when user account login has completed after calling Login() or AutoLogin()
 *
 * @param LocalUserNum the controller number of the associated user
 * @param bWasSuccessful true if server was contacted and a valid result received
 * @param UserId the user id received from the server on successful login
 * @param Error string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnLoginComplete, int32, bool, const FUniqueNetId&, const FString&);
typedef FOnLoginComplete::FDelegate FOnLoginCompleteDelegate;


/**
 * Delegate used in notifying the UI/game that the manual logout completed
 *
 * @param LocalUserNum the controller number of the associated user
 * @param bWasSuccessful whether the async call completed properly or not
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLogoutComplete, int32, bool);
typedef FOnLogoutComplete::FDelegate FOnLogoutCompleteDelegate;

/**
 * Interface for registration/authentication of user identities
 */
class IOnlineIdentity
{

protected:
	IOnlineIdentity() {};

public:
	virtual ~IOnlineIdentity() {};

	/**
	 * Delegate called when a player logs in/out
	 *
	 * @param LocalUserNum the player that logged in/out
	 */
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnLoginChanged, int32);

	/**
	 * Delegate called when a player's login status changes but doesn't change identity
	 *
	 * @param LocalUserNum the controller number of the associated user
	 * @param NewStatus the new login status for the user
	 * @param NewId the new id to associate with the user
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_TWO_PARAM(MAX_LOCAL_PLAYERS, OnLoginStatusChanged, ELoginStatus::Type, const FUniqueNetId&);

	/**
	 * Login/Authenticate with user credentials.
	 * Will call OnLoginComplete() delegate when async task completes
	 *
	 * @param LocalUserNum the controller number of the associated user
	 * @param AccountCredentials user account credentials needed to sign in to the online service
	 *
	 * @return true if login task was started
	 */
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) = 0;

	/**
	 * Called when user account login has completed after calling Login() or AutoLogin()
	 *
	 * @param LocalUserNum the controller number of the associated user
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param UserId the user id received from the server on successful login
	 * @param Error string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_THREE_PARAM(MAX_LOCAL_PLAYERS, OnLoginComplete, bool, const FUniqueNetId&, const FString&);

	/**
	 * Signs the player out of the online service
	 * Will call OnLogoutComplete() delegate when async task completes
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return true if logout task was started
	 */
	virtual bool Logout(int32 LocalUserNum) = 0;

	/**
	 * Delegate used in notifying the that manual logout completed
	 *
	 * @param LocalUserNum the controller number of the associated user
	 * @param bWasSuccessful whether the async call completed properly or not
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_ONE_PARAM(MAX_LOCAL_PLAYERS, OnLogoutComplete, bool);

	/**
	 * Logs the player into the online service using parameters passed on the
	 * command line. Expects -Login=<UserName> -Password=<password>. If either
	 * are missing, the function returns false and doesn't start the login
	 * process
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return true if the async call started ok, false otherwise
	 */
	virtual bool AutoLogin(int32 LocalUserNum) = 0;

	/**
	 * Obtain online account info for a user that has been registered
	 *
	 * @param UserId user to search for
	 *
	 * @return info about the user if found, NULL otherwise
	 */
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const = 0;

	/**
	 * Obtain list of all known/registered user accounts
	 *
	 * @param UserId user to search for
	 *
	 * @return info about the user if found, NULL otherwise
	 */
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const = 0;

	/**
	 * Gets the platform specific unique id for the specified player
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return Valid player id object if the call succeeded, NULL otherwise
	 */
	virtual TSharedPtr<FUniqueNetId> GetUniquePlayerId(int32 LocalUserNum) const = 0;

	/**
	 * Create a unique id from binary data (used during replication)
	 * 
	 * @param Bytes opaque data from appropriate platform
	 * @param Size size of opaque data
	 *
	 * @return UniqueId from the given data, NULL otherwise
	 */
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(uint8* Bytes, int32 Size) = 0;

	/**
	 * Create a unique id from string
	 * 
	 * @param Str string holding textual representation of an Id
	 *
	 * @return UniqueId from the given data, NULL otherwise
	 */
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(const FString& Str) = 0;

	/**
	 * Fetches the login status for a given player
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return the enum value of their status
	 */
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const = 0;

	/**
	 * Reads the player's nick name from the online service
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return a string containing the players nick name
	 */
	//@todo - move to user interface
	virtual FString GetPlayerNickname(int32 LocalUserNum) const = 0;

	/**
	 * Gets a user's platform specific authentication token to verify their identity
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return String representing the authentication token
	 */
	//@todo - remove and use GetUserAccount instead
	virtual FString GetAuthToken(int32 LocalUserNum) const = 0;
};

typedef TSharedPtr<IOnlineIdentity, ESPMode::ThreadSafe> IOnlineIdentityPtr;
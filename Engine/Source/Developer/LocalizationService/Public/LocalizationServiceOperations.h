// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ILocalizationServiceOperation.h"

#define LOCTEXT_NAMESPACE "LocalizationService"

/**
 * Operation used to connect (or test a connection) to localization service
 */
class FConnectToProvider : public ILocalizationServiceOperation
{
public:
	// ILocalizationServiceOperation interface
	virtual FName GetName() const override 
	{ 
		return "Connect"; 
	}

	virtual FText GetInProgressString() const override
	{ 
		return LOCTEXT("LocalizationService_Connecting", "Connecting to localization service..."); 
	}

	const FString& GetPassword() const
	{
		return Password;
	}

	void SetPassword(const FString& InPassword)
	{
		Password = InPassword;
	}

	const FText& GetErrorText() const
	{
		return OutErrorText;
	}

	void SetErrorText(const FText& InErrorText)
	{
		OutErrorText = InErrorText;
	}

protected:
	/** Password we use for this operation */
	FString Password;

	/** Error text for easy diagnosis */
	FText OutErrorText;
};

/**
 * Operation used to commit localization(s) to localization service
 */
class FDownloadLocalizationTargetFile : public ILocalizationServiceOperation
{
public:

	// ILocalizationServiceOperation interface
	virtual FName GetName() const override
	{
		return "DownloadLocalizationTargetFile";
	}

	virtual FText GetInProgressString() const override
	{ 
		return LOCTEXT("LocalizationService_FLocalizationServicerDownloadLocalizationTargetFile", "Getting Localization Target File from localization Service..."); 
	}


	void SetInTargetGuid(const FGuid& NewInTargetGuid)
	{
		InTargetGuid = NewInTargetGuid;
	}

	const FGuid& GetInTargetGuid() const
	{
		return InTargetGuid;
	}

	void SetInRelativeOutputFilePathAndName(const FString& NewInRelativeOutputFilePathAndName)
	{
		InRelativeOutputFilePathAndName = NewInRelativeOutputFilePathAndName;
	}

	const FString& GetInRelativeOutputFilePathAndName() const
	{
		return InRelativeOutputFilePathAndName;
	}

	FString GetInLocale()
	{
		return InLocale;
	}

	void SetInLocale(FString NewLocale)
	{
		InLocale = NewLocale;
	}

	FText GetOutErrorText()
	{
		return OutErrorText;
	}

	void SetOutErrorText(const FText& NewOutErrorText)
	{
		OutErrorText = NewOutErrorText;
	}


protected:

	/** The GUID of the Localization Target */
	FGuid InTargetGuid;

	/** The locale (culture code, for example "fr" or "en-us" or "ja-jp") to download the translations for */
	FString InLocale;

	/** The path and name to the file downloaded, relative to project directory */
	FString InRelativeOutputFilePathAndName;

	/** Place to easily store and access error message */
	FText OutErrorText;
};

///**
// * Operation used to commit localization(s) to localization service
// */
//class FCommitTranslations : public ILocalizationServiceOperation
//{
//public:
//	// ILocalizationServiceOperation interface
//	virtual FName GetName() const override
//	{
//		return "CommitTranslations";
//	}
//
//	virtual FText GetInProgressString() const override
//	{ 
//		return LOCTEXT("LocalizationService_FCommit", "Committing translation(s) to localization Service..."); 
//	}
//
//
//	void SetSuccessMessage( const FText& InSuccessMessage )
//	{
//		SuccessMessage = InSuccessMessage;
//	}
//
//	const FText& GetSuccessMessage() const
//	{
//		return SuccessMessage;
//	}
//
//protected:
//
//	/** A short message listing changelist/revision we submitted, if successful */
//	FText SuccessMessage;
//
//	/** The translations we are committing */
//	TArray<TPair<FLocalizationServiceTranslationIdentifier, FString>> TranslationsToCommit;
//};

///**
//* Operation used to update the localization service status of files
//*/
//class FLocalizationServiceUpdateStatus : public ILocalizationServiceOperation
//{
//public:
//	FLocalizationServiceUpdateStatus()
//		: bUpdateHistory(false)
//	{
//	}
//
//	// ILocalizationServiceOperation interface
//	virtual FName GetName() const override
//	{
//		return "UpdateStatus";
//	}
//
//	virtual FText GetInProgressString() const override
//	{
//		return LOCTEXT("LocalizationService_Update", "Updating translation(s) localization service status...");
//	}
//
//	void SetUpdateHistory(bool bInUpdateHistory)
//	{
//		bUpdateHistory = bInUpdateHistory;
//	}
//
//	bool ShouldUpdateHistory() const
//	{
//		return bUpdateHistory;
//	}
//	
//protected:
//	/** Whether to update history */
//	bool bUpdateHistory;
//};


#undef LOCTEXT_NAMESPACE
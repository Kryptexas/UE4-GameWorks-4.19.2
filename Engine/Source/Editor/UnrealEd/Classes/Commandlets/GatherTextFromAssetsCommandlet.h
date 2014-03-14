// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "GatherTextFromAssetsCommandlet.generated.h"

namespace EAssetTextGatherStatus
{
	enum Type
	{
		None,
		MissingKey,
		MissingKey_Resolved,
		IdentityConflict,
		IdentityConflict_Resolved,
	};
}

class FLocMetadataValue;

/**
 *	UGatherTextFromAssetsCommandlet: Localization commandlet that collects all text to be localized from the game assets.
 */
UCLASS()
class UGatherTextFromAssetsCommandlet : public UGatherTextCommandletBase
{
	GENERATED_UCLASS_BODY()

	void ProcessPackages( const TArray< UPackage* >& PackagesToProcess );
	void ProcessObject( UObject* Object, const UPackage* ObjectPackage );
	void ProcessDialogueWave( const UDialogueWave* DialogueWave );
	bool ProcessTextProperty( UTextProperty* TextProp, UObject* Object, const FString& ObjectPath, const bool bFixBroken, bool& OutRepaired );

public:
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface

private:
	struct FConflictTracker
	{
		struct FEntry
		{
			FString ObjectPath;
			TSharedPtr<FString> SourceString;
			EAssetTextGatherStatus::Type Status;
		};

		typedef TArray<FEntry> FEntryArray;
		typedef TMap<FString, FEntryArray> FKeyTable;
		typedef TMap<FString, FKeyTable> FNamespaceTable;

		FNamespaceTable Namespaces;
	};

	// Class that takes DialogueWaves and prepares internationalization manifest entries 
	class FDialogueHelper
	{
	public:
		bool ProcessDialogueWave( const UDialogueWave* DialogueWave );
		const FString& GetSpokenSource() const { return SpokenSource; }
		
		TSharedPtr< const FContext > GetBaseContext() const { return Base; };
		const TArray< TSharedPtr< FContext > >& GetContextSpecificVariations() const { return ContextSpecificVariations; }

	private:
		TSharedPtr< FLocMetadataValue > GenSourceTargetMetadata( const FString& SpeakerName, const TArray< FString >& TargetNames, bool bCompact = true );
		FString ArrayMetaDataToString( const TArray< TSharedPtr< FLocMetadataValue > >& MetadataArray );
		FString GetDialogueVoiceName( const UDialogueVoice* DialogueVoice );
		FString GetGrammaticalGenderString( TEnumAsByte<EGrammaticalGender::Type> Gender );
		FString GetGrammaticalNumberString( TEnumAsByte<EGrammaticalNumber::Type> Plurality );

	public:
		static const FString DialogueNamespace;
		static const FString PropertyName_VoiceActorDirection;
		static const FString PropertyName_Speaker;
		static const FString PropertyName_Speakers;
		static const FString PropertyName_Targets;
		static const FString PropertyName_GrammaticalGender;
		static const FString PropertyName_GrammaticalPlurality;
		static const FString PropertyName_TargetGrammaticalGender;
		static const FString PropertyName_TargetGrammaticalNumber;
		static const FString PropertyName_Optional;
		static const FString PropertyName_DialogueVariations;
		static const FString PropertyName_IsMature;

	private:
		FString DialogueKey;
		FString SourceLocation;
		FString SpokenSource;
		FString VoiceActorDirection;
		bool bIsMature;

		// The non-optional entry
		TSharedPtr< FContext > Base;
		// Optional entries that are context specific
		TArray< TSharedPtr< FContext > > ContextSpecificVariations;
		
	};

private:
	static const FString UsageText;

	FConflictTracker ConflictTracker;

	bool bFixBroken;
};

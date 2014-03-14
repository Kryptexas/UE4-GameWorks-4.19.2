// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "AudioDecompress.h"
#include "TargetPlatform.h"
#include "AudioDerivedData.h"
#include "SubtitleManager.h"

UDialogueVoice::UDialogueVoice(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, LocalizationGUID( FGuid::NewGuid() )
{
}

// Begin UObject interface. 
bool UDialogueVoice::IsReadyForFinishDestroy()
{
	return true;
}

FName UDialogueVoice::GetExporterName()
{
	return NAME_None;
}

FString UDialogueVoice::GetDesc()
{
	FString SummaryString;
	{
		UByteProperty* GenderProperty = CastChecked<UByteProperty>( FindFieldChecked<UProperty>( GetClass(), "Gender" ) );
		SummaryString += GenderProperty->Enum->GetEnumString(Gender);

		if( Plurality != EGrammaticalNumber::Singular )
		{
			UByteProperty* PluralityProperty = CastChecked<UByteProperty>( FindFieldChecked<UProperty>( GetClass(), "Plurality" ) );

			SummaryString += ", ";
			SummaryString += PluralityProperty->Enum->GetEnumString(Plurality);
		}
	}

	return FString::Printf( TEXT( "%s (%s)" ), *( GetName() ), *(SummaryString) );
}

void UDialogueVoice::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);
}

void UDialogueVoice::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if( !bDuplicateForPIE )
	{
		LocalizationGUID = FGuid::NewGuid();
	}
}
// End UObject interface. 
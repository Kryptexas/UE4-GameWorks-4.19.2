// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UDeviceProfile::UDeviceProfile(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BaseProfileName = TEXT("");
	DeviceType = TEXT("");

	bVisible = true;

	FString DeviceProfileFileName = FPaths::EngineConfigDir() + TEXT("Deviceprofiles.ini");
	LoadConfig(GetClass(), *DeviceProfileFileName);
}


#if WITH_EDITOR

void UDeviceProfile::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty( PropertyChangedEvent );

	if( PropertyChangedEvent.Property->GetFName() == TEXT("BaseProfileName") )
	{
		FString NewParentName = *PropertyChangedEvent.Property->ContainerPtrToValuePtr<FString>( this );

		if( UObject* ParentRef = FindObject<UDeviceProfile>( GetTransientPackage(), *NewParentName ) )
		{
			// Generation and profile reference
			TMap<UDeviceProfile*,int32> DependentProfiles;

			int32 NumGenerations = 1;
			DependentProfiles.Add(this,0);

			for( TObjectIterator<UDeviceProfile> DeviceProfileIt; DeviceProfileIt; ++DeviceProfileIt )
			{
				UDeviceProfile* ParentProfile = *DeviceProfileIt;

				if( !ParentProfile->HasAnyFlags(RF_PendingKill) )
				{
					int32 ProfileGeneration = 1;
					while( ParentProfile != NULL )
					{
						if( this->GetName() == ParentProfile->BaseProfileName )
						{
							NumGenerations = NumGenerations > ProfileGeneration ? NumGenerations : ProfileGeneration;
							DependentProfiles.Add(*DeviceProfileIt,ProfileGeneration);
							break;
						}

						ParentProfile = FindObject<UDeviceProfile>( GetTransientPackage(), *ParentProfile->BaseProfileName );
						++ProfileGeneration;
					}
				}
			}


			UDeviceProfile* ClassCDO = CastChecked<UDeviceProfile>(GetClass()->GetDefaultObject());

			for( int32 CurrentGeneration = 0; CurrentGeneration < NumGenerations; CurrentGeneration++ )
			{
				for( TMap<UDeviceProfile*,int32>::TIterator DeviceProfileIt(DependentProfiles); DeviceProfileIt; ++DeviceProfileIt )
				{
					if( CurrentGeneration == DeviceProfileIt.Value() )
					{
						UDeviceProfile* CurrentGenerationProfile = DeviceProfileIt.Key();
						UDeviceProfile* ParentProfile = FindObject<UDeviceProfile>( GetTransientPackage(), *CurrentGenerationProfile->BaseProfileName );
						if( ParentProfile == NULL )
						{
							ParentProfile = ClassCDO;
						}

						for (TFieldIterator<UProperty> CurrentObjPropertyIter( GetClass() ); CurrentObjPropertyIter; ++CurrentObjPropertyIter)
						{
							bool bIsSameParent = CurrentObjPropertyIter->Identical_InContainer( ClassCDO, CurrentGenerationProfile );
							if( bIsSameParent )
							{
								void* CurrentGenerationProfilePropertyAddress = CurrentObjPropertyIter->ContainerPtrToValuePtr<void>( CurrentGenerationProfile );
								void* ParentPropertyAddr = CurrentObjPropertyIter->ContainerPtrToValuePtr<void>( ParentRef );

								CurrentObjPropertyIter->CopyCompleteValue( CurrentGenerationProfilePropertyAddress, ParentPropertyAddr );
							}
						}
					}
				}
			}
		}
	}
}

#endif
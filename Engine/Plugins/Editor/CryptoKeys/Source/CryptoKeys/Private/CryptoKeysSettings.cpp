// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "CryptoKeysSettings.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Base64.h"

UCryptoKeysSettings::UCryptoKeysSettings()
{
	// Migrate any settings from the old ini files if they exist
	UProjectPackagingSettings* ProjectPackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
	if (ProjectPackagingSettings)
	{
		bEncryptPakIniFiles = ProjectPackagingSettings->bEncryptIniFiles_DEPRECATED;
		bEncryptPakIndex = ProjectPackagingSettings->bEncryptPakIndex_DEPRECATED;

		if (GConfig->IsReadyForUse())
		{
			FString EncryptionIni;
			FConfigCacheIni::LoadGlobalIniFile(EncryptionIni, TEXT("Encryption"));

			FString OldEncryptionKey;
			if (GConfig->GetString(TEXT("Core.Encryption"), TEXT("aes.key"), OldEncryptionKey, EncryptionIni))
			{
				EncryptionKey = FBase64::Encode(OldEncryptionKey);
			}

			FString OldSigningModulus, OldSigningPublicExponent, OldSigningPrivateExponent;
			if ((bEnablePakSigning = GConfig->GetString(TEXT("Core.Encryption"), TEXT("rsa.privateexp"), OldSigningPrivateExponent, EncryptionIni)
				&& GConfig->GetString(TEXT("Core.Encryption"), TEXT("rsa.publicexp"), OldSigningPublicExponent, EncryptionIni)
				&& GConfig->GetString(TEXT("Core.Encryption"), TEXT("rsa.modulus"), OldSigningModulus, EncryptionIni)))
			{
				SigningModulus = FBase64::Encode(OldSigningModulus);
				SigningPublicExponent = FBase64::Encode(OldSigningPublicExponent);
				SigningPrivateExponent = FBase64::Encode(OldSigningPrivateExponent);
			}
		}
	}
}

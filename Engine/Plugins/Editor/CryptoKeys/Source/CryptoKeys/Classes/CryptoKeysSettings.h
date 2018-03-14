// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "CryptoKeysSettings.generated.h"

/**
* Implements the settings for imported Paper2D assets, such as sprite sheet textures.
*/
UCLASS(config = Crypto, defaultconfig)
class CRYPTOKEYS_API UCryptoKeysSettings : public UObject
{
	GENERATED_BODY()

public:

	UCryptoKeysSettings();

	bool IsEncryptionEnabled() const
	{
		return EncryptionKey.Len() > 0 && (bEncryptAllAssetFiles || bEncryptPakIndex || bEncryptPakIniFiles || bEncryptUAssetFiles);
	}

	bool IsSigningEnabled() const
	{
		return bEnablePakSigning && SigningModulus.Len() > 0 && SigningPrivateExponent.Len() > 0 && SigningPublicExponent.Len() > 0;
	}

	// The 128-bit AES encryption key used to protect the pak file
	UPROPERTY(config, VisibleAnywhere, Category = Encryption)
	FString EncryptionKey;

	// Encrypts all ini files in the pak. Gives security to the most common sources of mineable information, with minimal runtime IO cost
	UPROPERTY(config, EditAnywhere, Category = Encryption)
	bool bEncryptPakIniFiles;

	// Encrypt the pak index, making it impossible to use unrealpak to manipulate the pak file without the encryption key
	UPROPERTY(config, EditAnywhere, Category = Encryption)
	bool bEncryptPakIndex;

	// Encrypts the uasset file in cooked data. Less runtime IO cost, and protection to package header information, including most string data, but still leaves the bulk of the data unencrypted. 
	UPROPERTY(config, EditAnywhere, Category = Encryption)
	bool bEncryptUAssetFiles;

	// Encrypt all files in the pak file. Secure, but will cause some slowdown to runtime IO performance, and high entropy to packaged data which will be bad for patching
	UPROPERTY(config, EditAnywhere, Category = Encryption)
	bool bEncryptAllAssetFiles;

	// The RSA key public exponent used for signing a pak file
	UPROPERTY(config, VisibleAnywhere, Category = Signing)
	FString SigningPublicExponent;

	// The RSA key modulus used for signing a pak file
	UPROPERTY(config, VisibleAnywhere, Category = Signing)
	FString SigningModulus;

	// The RSA key private exponent used for signing a pak file
	UPROPERTY(config, VisibleAnywhere, Category = Signing)
	FString SigningPrivateExponent;

	// Enable signing of pak files, to prevent tampering of the data
	UPROPERTY(config, EditAnywhere, Category = Signing)
	bool bEnablePakSigning;
};

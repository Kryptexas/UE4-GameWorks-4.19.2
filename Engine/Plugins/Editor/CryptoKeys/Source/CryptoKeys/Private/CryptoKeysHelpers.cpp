// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CryptoKeysHelpers.h"
#include "CryptoKeysOpenSSL.h"

#include "Misc/Base64.h"
#include "Math/BigInt.h"

#include "CryptoKeysSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogCryptoKeys, Log, All);

namespace CryptoKeysHelpers
{
	bool TestKeys(FEncryptionKey& InPublicKey, FEncryptionKey& InPrivateKey)
	{
		UE_LOG(LogCryptoKeys, Display, TEXT("Testing signature keys."));

		// Just some random values
		static TEncryptionInt TestData[] =
		{
			11,
			253,
			128,
			234,
			56,
			89,
			34,
			179,
			29,
			1024,
			(int64)(MAX_int32),
			(int64)(MAX_uint32)-1
		};

		for (int32 TestIndex = 0; TestIndex < ARRAY_COUNT(TestData); ++TestIndex)
		{
			TEncryptionInt EncryptedData = FEncryption::ModularPow(TestData[TestIndex], InPrivateKey.Exponent, InPrivateKey.Modulus);
			TEncryptionInt DecryptedData = FEncryption::ModularPow(EncryptedData, InPublicKey.Exponent, InPublicKey.Modulus);
			if (TestData[TestIndex] != DecryptedData)
			{
				UE_LOG(LogCryptoKeys, Error, TEXT("Keys do not properly encrypt/decrypt data (failed test with %lld)"), TestData[TestIndex].ToInt());
				return false;
			}
		}

		UE_LOG(LogCryptoKeys, Display, TEXT("Signature keys check completed successfuly."));

		return true;
	}

	bool GenerateNewEncryptionKey(UCryptoKeysSettings* InSettings)
	{
		bool bResult = false;

		TArray<uint8> NewEncryptionKey;
		if (CryptoKeysOpenSSL::GenerateNewEncryptionKey(NewEncryptionKey))
		{
			InSettings->EncryptionKey = FBase64::Encode(NewEncryptionKey);
			bResult = true;
		}
	
		return bResult;
	}

	bool GenerateNewSigningKeys(UCryptoKeysSettings* InSettings)
	{
		bool bResult = false;

		TArray<uint8> PublicExponent, PrivateExponent, Modulus;
		if (CryptoKeysOpenSSL::GenerateNewSigningKey(PublicExponent, PrivateExponent, Modulus))
		{
			InSettings->SigningPublicExponent = FBase64::Encode(PublicExponent);
			InSettings->SigningPrivateExponent = FBase64::Encode(PrivateExponent);
			InSettings->SigningModulus = FBase64::Encode(Modulus);
			bResult = true;
		}

		return bResult;
	}
}

// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "CryptoKeysOpenSSL.h"
#include "ModuleManager.h"

#if PLATFORM_WINDOWS
#include <AllowWindowsPlatformTypes.h>
#endif

#include <openssl/rand.h>
#include <openssl/pem.h>

#if PLATFORM_WINDOWS
#include <HideWindowsPlatformTypes.h>
#endif

#include "Math/BigInt.h"

DEFINE_LOG_CATEGORY_STATIC(LogCryptoKeys, Log, All);

namespace CryptoKeysOpenSSL
{
	bool GenerateNewEncryptionKey(TArray<uint8>& OutKey)
	{
		OutKey.Empty(32);
		OutKey.AddUninitialized(32);
		bool bResult = !!RAND_bytes(&OutKey[0], 32);
		if (!bResult)
		{
			OutKey.Empty();
		}
		return bResult;
	}

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

	bool GenerateNewSigningKey(TArray<uint8>& OutPublicExponent, TArray<uint8>& OutPrivateExponent, TArray<uint8>& OutModulus)
	{
		TArray<uint8> NewP, NewQ;

		RSA* RSAKey = RSA_new();
		BIGNUM* E = BN_new();
		BN_set_word(E, RSA_F4);
		RSA_generate_key_ex(RSAKey, 255, E, nullptr);

		BIGNUM* P = RSAKey->p;
		BIGNUM* Q = RSAKey->q;

		const uint32 WordSize = sizeof(BN_ULONG);

		uint32 NumBytes = WordSize * P->dmax;
		NewP.Empty(NumBytes);
		NewP.AddUninitialized(NumBytes);
		FMemory::Memcpy(&NewP[0], P->d, NumBytes);

		NumBytes = WordSize * Q->dmax;
		NewQ.Empty(NumBytes);
		NewQ.AddUninitialized(NumBytes);
		FMemory::Memcpy(&NewQ[0], Q->d, NumBytes);

		BN_free(P);
		BN_free(Q);

		NewP.AddZeroed(sizeof(TEncryptionInt) - NewP.Num());
		NewQ.AddZeroed(sizeof(TEncryptionInt) - NewQ.Num());

		check(NewP.Num() == sizeof(TEncryptionInt));
		check(NewQ.Num() == sizeof(TEncryptionInt));

		TEncryptionInt OurP((uint32*)&NewP[0]);
		TEncryptionInt OurQ((uint32*)&NewQ[0]);

		FEncryptionKey PublicKey, PrivateKey;
		FEncryption::GenerateKeyPair(OurP, OurQ, PublicKey, PrivateKey);

		TestKeys(PublicKey, PrivateKey);

		OutPublicExponent.AddUninitialized(sizeof(TEncryptionInt));
		OutPrivateExponent.AddUninitialized(sizeof(TEncryptionInt));
		OutModulus.AddUninitialized(sizeof(TEncryptionInt));
		FMemory::Memcpy(&OutPublicExponent[0], (const uint8*)PublicKey.Exponent.GetBits(), sizeof(TEncryptionInt));
		FMemory::Memcpy(&OutPrivateExponent[0], (const uint8*)PrivateKey.Exponent.GetBits(), sizeof(TEncryptionInt));
		FMemory::Memcpy(&OutModulus[0], (const uint8*)PublicKey.Modulus.GetBits(), sizeof(TEncryptionInt));

		return true;
	}
}

IMPLEMENT_MODULE(FDefaultModuleImpl, CryptoKeysOpenSSL)
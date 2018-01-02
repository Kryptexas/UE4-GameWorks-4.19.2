// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CryptoKeysCommandlet.h"
#include "CryptoKeysSettings.h"
#include "CryptoKeysHelpers.h"
#include "CryptoKeysOpenSSL.h"

DEFINE_LOG_CATEGORY_STATIC(LogCryptoKeysCommandlet, Log, All);

UCryptoKeysCommandlet::UCryptoKeysCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

int32 UCryptoKeysCommandlet::Main(const FString& InParams)
{
	bool bUpdateAllKeys = FParse::Param(*InParams, TEXT("updateallkeys"));
	bool bUpdateEncryptionKey = bUpdateAllKeys || FParse::Param(*InParams, TEXT("updateencryptionkey"));
	bool bUpdateSigningKey = bUpdateAllKeys || FParse::Param(*InParams, TEXT("updatesigningkey"));
	bool bTestSigningKeyGeneration = FParse::Param(*InParams, TEXT("testsigningkeygen")); 

	if (bUpdateEncryptionKey || bUpdateSigningKey)
	{
		UCryptoKeysSettings* Settings = GetMutableDefault<UCryptoKeysSettings>();
		bool Result;

		if (bUpdateEncryptionKey)
		{
			Result = CryptoKeysHelpers::GenerateNewEncryptionKey(Settings);
			check(Result);
		}

		if (bUpdateSigningKey)
		{
			Result = CryptoKeysHelpers::GenerateNewSigningKeys(Settings);
			check(Result);
		}

		Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
	}

	if (bTestSigningKeyGeneration)
	{
		TArray<uint32> PublicExponentCRCs, PrivateExponentCRCs, ModulusCRCs;

		static const int32 NumLoops = INT32_MAX;
		for (int32 LoopCount = 0; LoopCount < NumLoops; ++LoopCount)
		{
			UE_LOG(LogCryptoKeysCommandlet, Display, TEXT("Key generation test [%i/%i]"), LoopCount + 1, NumLoops);
			TArray<uint8> PublicExponent, PrivateExponent, Modulus;
			bool bResult = CryptoKeysOpenSSL::GenerateNewSigningKey(PublicExponent, PrivateExponent, Modulus);
			check(bResult);

			uint32 PublicExponentCRC = FCrc::MemCrc32(&PublicExponent[0], PublicExponent.Num());
			uint32 PrivateExponentCRC = FCrc::MemCrc32(&PrivateExponent[0], PrivateExponent.Num());
			uint32 ModulusCRC = FCrc::MemCrc32(&Modulus[0], Modulus.Num());
			check(!PublicExponentCRCs.Contains(PublicExponentCRC));
			check(!PrivateExponentCRCs.Contains(PrivateExponentCRC));
			check(!ModulusCRCs.Contains(ModulusCRC));

			PublicExponentCRCs.Add(PublicExponentCRC);
			PrivateExponentCRCs.Add(PrivateExponentCRC);
			ModulusCRCs.Add(ModulusCRC);
		}
	}

	return 0;
}

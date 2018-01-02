// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

namespace CryptoKeysOpenSSL
{
	bool CRYPTOKEYSOPENSSL_API GenerateNewEncryptionKey(TArray<uint8>& OutKey);
	bool CRYPTOKEYSOPENSSL_API GenerateNewSigningKey(TArray<uint8>& OutPublicExponent, TArray<uint8>& OutPrivateExponent, TArray<uint8>& OutModulus);
}
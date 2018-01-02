// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

class UCryptoKeysSettings;

namespace CryptoKeysHelpers
{
	bool GenerateNewEncryptionKey(UCryptoKeysSettings* InSettings);
	bool GenerateNewSigningKeys(UCryptoKeysSettings* InSettings);
}
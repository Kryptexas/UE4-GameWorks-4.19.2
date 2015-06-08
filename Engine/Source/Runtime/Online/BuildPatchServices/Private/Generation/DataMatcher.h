// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_BUILDPATCHGENERATION

namespace FBuildPatchServices
{
	class FDataMatcher
	{
	public:
		virtual bool CompareData(const FGuid& DataGuid, uint64 Hash, const TArray<uint8>& NewData) const = 0;
	};

	typedef TSharedRef<FDataMatcher, ESPMode::ThreadSafe> FDataMatcherRef;
	typedef TSharedPtr<FDataMatcher, ESPMode::ThreadSafe> FDataMatcherPtr;

	class FDataMatcherFactory
	{
	public:
		static FDataMatcherRef Create(const FString& CloudDirectory);
	};
}

#endif

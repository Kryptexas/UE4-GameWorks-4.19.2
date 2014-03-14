// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class CORE_API FEngineBuildSettings
{
public:
	/**
	 * @return True if the build was gotten from perforce
	 */
	static bool IsPerforceBuild();

	/**
	 * @return True if the build is for internal projects only
	 */
	static bool IsInternalBuild();
};
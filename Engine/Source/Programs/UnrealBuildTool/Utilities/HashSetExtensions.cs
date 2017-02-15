// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.Collections.Generic;

namespace UnrealBuildTool
{
	public static class HashSetExtensions
	{
		public static HashSet<T> ToHashSet<T>(this IEnumerable<T> Sequence)
		{
			return new HashSet<T>(Sequence);
		}
	}
}
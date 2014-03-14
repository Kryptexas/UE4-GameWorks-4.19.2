// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace APIDocTool
{
	class APITypeIndex : APIMemberIndex
	{
		public const string IgnorePrefixLetters = "UAFTS"; 

		public APITypeIndex(APIPage InParent, IEnumerable<APIMember> Members) : base(InParent, "Types", "Alphabetical index of all types")
		{
			foreach (APIMember Member in Members)
			{
				if ((Member is APIRecord && (!((APIRecord)Member).bIsTemplateSpecialization)) || (Member is APITypeDef) || (Member is APIEnum) || (Member is APIEventParameters))
				{
					Entry NewEntry = new Entry(Member);
					if (NewEntry.SortName.Length >= 2 && IgnorePrefixLetters.Contains(NewEntry.SortName[0]) && Char.IsUpper(NewEntry.SortName[1]))
					{
						NewEntry.SortName = NewEntry.SortName.Substring(1);
					}
					AddToDefaultCategory(NewEntry);
				}
			}
		}
	}
}

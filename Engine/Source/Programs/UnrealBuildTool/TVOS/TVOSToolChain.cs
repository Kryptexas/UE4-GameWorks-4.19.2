// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Diagnostics;
using System.Security.AccessControl;
using System.Xml;
using System.Text;
using Ionic.Zip;
using Ionic.Zlib;

namespace UnrealBuildTool
{
	class TVOSToolChainSettings : IOSToolChainSettings
	{
		public TVOSToolChainSettings() : base("AppleTVOS", "AppleTVSimulator")
		{
		}
	}

	class TVOSToolChain : IOSToolChain
	{
		public TVOSToolChain(FileReference InProjectFile, IOSPlatformContext InPlatformContext, TVOSProjectSettings InProjectSettings)
			: base(CppPlatform.TVOS, InProjectFile, InPlatformContext, InProjectSettings, () => new TVOSToolChainSettings())
		{
		}
	};
}

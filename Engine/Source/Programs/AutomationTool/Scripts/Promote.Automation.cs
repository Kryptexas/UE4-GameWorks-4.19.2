// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;

[Help("Promotes a build")]
[RequireP4]
class Promote : BuildCommand
{
	#region Fields
	#endregion

	void ExecuteInner()
	{
		MakeDownstreamLabel("");
	}

	public override void ExecuteBuild()
	{
		Log("************************* Promote");

		Log("Promote Label {0}  CL {1}", P4Env.LabelToSync, P4Env.ChangelistString);

		ExecuteInner();
	}
}

// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	class RemoteExecutor : LocalExecutor
	{
		public RemoteExecutor()
		{
		}

		public override string Name
		{
			get { return "Remote"; }
		}

		public override int GetMaxActionsToExecuteInParallel()
		{
			Int32 RemoteCPUCount = RPCUtilHelper.GetCommandSlots();
			if (RemoteCPUCount == 0)
			{
				RemoteCPUCount = Environment.ProcessorCount;
			}
			return RemoteCPUCount;
		}
	}
}

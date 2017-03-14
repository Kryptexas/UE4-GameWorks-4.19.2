// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Linq.Expressions;

namespace Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories
{
    public interface IFunctionRepository : IDataRepository<FunctionCall>
    {
        int FirstId(Expression<Func<FunctionCall, bool>> filter);
    }
}

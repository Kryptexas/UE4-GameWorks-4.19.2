// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories
{
    public interface IUserRepository: IDataRepository<User>
    {
        User GetByUserName(string userName);
    }
}

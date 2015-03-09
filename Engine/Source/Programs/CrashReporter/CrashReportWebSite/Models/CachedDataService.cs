// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Web.Caching;

namespace Tools.CrashReporter.CrashReportWebSite.Models
{
	/// <summary>
	/// A class to handle caching of database results on the web server.
	/// </summary>
	public class CachedDataService 
	{
		private Cache CacheInstance;

		private const string CacheKeyPrefix = "_CachedDataService_";
		private const string CallstackKeyPrefix = "_CallStack_";
		private const string FunctionCallKeyPrefix = "_FunctionCallStack_";
		private const string UserNameKeyPrefix = "_UserName_";

		/// <summary>
		/// Link the Http context cache to a crash repository.
		/// </summary>
		/// <param name="InCache">The current Http context cache.</param>
		public CachedDataService( Cache InCache )
		{
			CacheInstance = InCache;
		}

		/// <summary>
		/// Retrieve a cached (and pre-parsed) callstack container from the cache, or parse the raw callstack, and add to the cache.
		/// </summary>
		/// <param name="CurrentCrash">The crash to retrieve the parsed callstack for.</param>
		/// <returns>A parsed callstack.</returns>
		public CallStackContainer GetCallStack( Crash CurrentCrash )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(CrashId=" + CurrentCrash.Id + ")" ) )
			{
				string Key = CacheKeyPrefix + CallstackKeyPrefix + CurrentCrash.Id;
				CallStackContainer CallStack = (CallStackContainer)CacheInstance[Key];
				if( CallStack == null )
				{
					CallStack = new CallStackContainer( CurrentCrash );
					CallStack.bDisplayFunctionNames = true;
					CacheInstance.Insert( Key, CallStack );
				}

				return CallStack;
			}
		}

		/// <summary>
		/// Retrieve a list of function call names from an encoded pattern. This is either from the cache, or from the database and then added to the cache.
		/// </summary>
		/// <param name="Pattern">A callstack pattern in the form '1+3+34+2'.</param>
		/// <returns>A list of function names.</returns>
		public List<string> GetFunctionCalls( string Pattern )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(Count=" + CacheInstance.Count + ")" ) )
			{
				string Key = CacheKeyPrefix + FunctionCallKeyPrefix + Pattern;
				List<string> FunctionCalls = (List<string>)CacheInstance[Key];
				if( FunctionCalls == null )
				{
					string[] Ids = Pattern.Split( "+".ToCharArray(), StringSplitOptions.RemoveEmptyEntries );
					List<int> IdList = new List<int>();
					foreach( string id in Ids )
					{
						int i;
						if( int.TryParse( id, out i ) )
						{
							IdList.Add( i );
						}
					}

					// These function calls don't add any new information.
					IdList.Remove( 63039 ); // 63039	KERNELBASE!RaiseException()
					IdList.Remove( 63138 ); // 63138	UE4Editor_Core!FDebug::EnsureFailed()
					IdList.Remove( 63137 ); // 63137	UE4Editor_Core!NewReportEnsure()
					IdList.Remove( 63149 ); // 63149	UE4Editor_Core!FOutputDeviceWindowsError::Serialize()
					IdList.Remove( 63151 ); // 63151	UE4Editor_Core!FDebug::AssertFailed()
					IdList.Remove( 63445 ); // 63445	UE4Editor_Core!FDebug::EnsureNotFalseFormatted()
					IdList.Remove( 64334 ); // 64334	UE4Editor_Core!FOutputDevice::Logf__VA()


					FunctionCalls = FRepository.Get().Buggs.GetFunctionCalls( IdList );
					CacheInstance.Insert( Key, FunctionCalls );
				}

				return FunctionCalls;
			}
		}
	}
}
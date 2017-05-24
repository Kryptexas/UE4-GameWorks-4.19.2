// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;

namespace UnrealBuildTool
{
	/// <summary>
	/// Base class for exceptions thrown by UBT
	/// </summary>
	public class BuildException : Exception
	{
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="MessageFormat">Format for the message string</param>
		/// <param name="MessageObjects">Format arguments</param>
		public BuildException(string MessageFormat, params Object[] MessageObjects) :
			base("ERROR: " + string.Format(MessageFormat, MessageObjects))
		{
		}

		/// <summary>
		/// Constructor which wraps another exception
		/// </summary>
		/// <param name="InnerException">The inner exception being wrapped</param>
		/// <param name="MessageFormat">Format for the message string</param>
		/// <param name="MessageObjects">Format arguments</param>
		public BuildException(Exception InnerException, string MessageFormat, params Object[] MessageObjects) :
			base("ERROR: " + string.Format(MessageFormat, MessageObjects), InnerException)
		{
		}

		/// <summary>
		/// Returns the string representing the exception. Our build exceptions do not show the callstack since they are used to report known error conditions.
		/// </summary>
		/// <returns>Message for the exception</returns>
		public override string ToString()
		{
			if (Message != null && Message.Length > 0)
			{
				return Message;
			}
			else
			{
				return base.ToString();
			}
		}
	};

	class MissingModuleException : BuildException
	{
		public MissingModuleException(string InModuleName) :
			base("Couldn't find module rules file for module '{0}'.", InModuleName)
		{
			ModuleName = InModuleName;
		}

		public string ModuleName;
	};
}

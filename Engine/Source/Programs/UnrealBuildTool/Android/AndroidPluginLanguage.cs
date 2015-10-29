// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Linq;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool.Android
{
	/* AndroidPluginLanguage (APL) is a simple XML-based language for manipulating XML and returning
	 * strings.  It contains an <init> section which is evaluated once per architecture before any
	 * other sections.  The state is maintained and carried forward to the next section evaluated
	 * so the order the sections are executed matters.
	 * 
	 * If you need to see the instructions executed in your plugin context add the following to
	 * enable tracing:
	 * 
	 *	<trace enable="true"/>
	 *	
	 * After this instuction all the nodes actually executed in your context will be written to the
	 * log until you do a <trace enable="false"/>.  You can also get a dump of all the variables in
	 * your context with this command:
	 * 
	 *	<dumpvars/>
	 * 
	 * Bool, Int, and String variable types are supported.  Any attribute may reference a variable
	 * and will be replaced with the string equivalent before evaluation using this syntax:
	 * 
	 *	$B(name) = boolean variable "name"'s value
	 *	$I(name) = integer variable "name"'s value
	 *	$S(name) = string variable "name"'s value
	 *	$E(name) = element variable "name"'s value
	 * 
	 * The following variables are initialized automatically:
	 * 
	 *	$S(Output) = the output returned for evaluating the section (initialized to Input)
	 *	$S(Architecture) = target architecture (armeabi-armv7, armeabi-armv8, x86, x86_64)
	 *	$S(PluginDir) = directory the XML file was loaded from
	 *	$S(EngineDir) = engine directory
	 *	$S(BuildDir) = project's Intermediate/Android/APK directory
	 *	$B(Distribution) = true if distribution build
	 * 
	 * Note: with the exception of the above variables, all are in the context of the plugin to
	 * prevent namespace collision; trying to set a new value to any of the above, with the
	 * exception of Output, will only affect the current context.
	 * 
	 * The following nodes allow manipulation of variables:
	 * 
	 *	<setBool result="" value=""/>
	 *	<setInt result="" value=""/>
	 *	<setString result="" value=""/>
	 *	<setElement result="" value=""/>
	 *	<setElement result="" xml=""/>
	 *	
	 * <setElement> with value creates an empty XML element with the tag set to value.
	 * <setElement> with xml will parse the XML provided.  Remember to escape any special characters!
	 * 
	 * Variables may also be set from a property in an ini file:
	 * 
	 *	<setBoolFromProperty result="" ini="" section="" property="" default=""/>
	 *	<setIntFromProperty result="" ini="" section="" property="" default=""/>
	 *	<setStringFromProperty result="" ini="" section="" property="" default=""/>
	 * 
	 * Boolean variables may also be set to the result of applying operators:
	 * 
	 *	<setBoolNot result="" source=""/>
	 *	<setBoolAnd result="" arg1="" arg2=""/>
	 *	<setBoolOr result="" arg1="" arg2=""/>
	 *	<setBoolIsEqual result="" arg1="" arg2=""/>
	 *	<setBoolIsLess result="" arg1="" arg2=""/>
	 *	<setBoolIsLessEqual result="" arg1="" arg2=""/>
	 *	<setBoolIsGreater result="" arg1="" arg2=""/>
	 *	<setBoolIsGreaterEqual result="" arg1="" arg2=""/>
	 * 
	 * Integer variables may use these arithmetic operations: 
	 * 
	 *	<setIntAdd result="" arg1="" arg2=""/>
	 *	<setIntSubtract result="" arg1="" arg2=""/>
	 *	<setIntMultiply result="" arg1="" arg2=""/>
	 *	<setIntDivide result="" arg1="" arg2=""/>
	 * 
	 * Strings are manipulated with the following:
	 * 
 	 *	<setStringAdd result="" arg1="" arg2=""/>
	 *	<setStringSubstring result="" source="" start="" length=""/>
	 *	<setStringReplace result="" source="" find="" with=""/>
	 * 
	 * String length may be retrieved with:
	 * 
	 *	<setIntLength result="" source=""/>
	 * 
	 * The index of a search string may be found in source with:
	 * 
	 *	<setIntFindString result="" source="" find=""/>
	 *	
	 * The following shortcut string comparisons may also be used instead of using <setIntFindString>
	 * and checking the result:
	 * 
	 *	<setBoolStartsWith result="" source="" find=""/>
	 *	<setBoolEndsWith result="" source="" find=""/>
	 *	<setBoolContains result="" source="" find=""/>
	 *	
	 * Messages are written to the log with this node:
	 *
	 *	<log text=""/>
	 * 
	 * Conditional execution uses the following form:
	 * 
	 *	<if condition="">
	 *		<true>
	 *			<!-- executes if boolean variable in condition is true -->
	 *		</true>
	 *		<false>
	 *			<!-- executes if boolean variable in condition is false -->
	 *		</false>
	 *	</if>
	 * 
	 * The <true> and <false> blocks are optional.  The condition must be in a boolean variable.
	 * The boolean operator nodes may be combined to create a final state for more complex
	 * conditions:
	 * 
	 *	<setBoolNot result="notDistribution" value="$B(Distribution)/>
	 *	<setBoolEquals result="isX86" arg1="$S(Architecture)" arg2="x86"/>
	 *	<setBoolEquals result="isX86_64" arg2="$S(Architecture)" arg2="x86_64/">
	 *	<setBoolOr result="isIntel" arg1="$B(isX86)" arg2="$B(isX86_64)"/>
	 *	<setBoolAnd result="intelAndNotDistribution" arg1="$B(isIntel)" arg2="$B(notDistribution)"/>
	 *	<if condition="intelAndNotDistribution">
	 *		<true>
	 *			<!-- do something for Intel if not a distribution build -->
	 *		</true>
	 *	</if>
	 * 
	 * Note the "isIntel" could also be done like this:
	 * 
	 *	<setStringSubstring result="subarch" source="$S(Architecture)" start="0" length="3"/>
	 *	<setBoolEquals result="isIntel" arg1="$S(subarch)" arg2="x86"/>
	 * 
	 * Two shortcut nodes are available for conditional execution:
	 * 
	 *	<isArch arch="armeabi-armv7">
	 *		<!-- do stuff -->
	 *	</isArch>
	 * 
	 * is the equivalent of:
	 * 
	 *	<setBoolEquals result="temp" arg1="$S(Architecture)" arg2="armeabi-armv7">
	 *	<if condition="temp">
	 *		<true>
	 *			<!-- do stuff -->
	 *		</true>
	 *	</if>
	 *	
	 * and
	 * 
	 *	<isDistribution>
	 *		<!-- do stuff -->
	 *	</isDistribution>
	 *	
	 * is the equivalent of:
	 * 
	 *	<if condition="$B(Distribution)">
	 *		<!-- do stuff -->
	 *	</if>
	 * 
	 * Execution may be stopped with:
	 * 
	 *	<return/>
	 *	 
	 * Loops may be created using these nodes:
	 * 
	 *	<while condition="">
	 *		<!-- do stuff -->
	 *	</while>
	 *	
	 *	<break/>
	 *	<continue/>
	 *	
	 * The <while> body will execute until the condition is false or a <break/> is hit.  The
	 * <continue/> will restart execution of the loop if the condition is still true or exit.
	 * 
	 * Note: <break/> outside a <while> body will act the same as <return/>
	 * 
	 * Here is an example loop which writes 1 to 5 to the log, skipping 3.  Note the update of the
	 * while condition should be done before the continue otherwise it may not exit.
	 * 
	 *	<setInt result="index" value="0"/>
	 *	<setBool result="loopRun" value="true"/>
	 *	<while condition="loopRun">
	 *		<setIntAdd result="index" arg1="$I(index)" arg2="1"/>
	 *		<setBoolIsLess result="loopRun" arg1="$I(index)" arg2="5"/>
	 *		<setBoolIsEqual result="indexIs3" arg1="$I(index)" arg2="3"/>
	 *		<if condition="indexIs3">
	 *			<true>
	 *				<continue/>
	 *			</true>
	 *		</if>
	 *		<log text="$I(index)"/>
	 *	</while>
	 *	
	 * It is possible to use variable replacement in generating the result variable
	 * name as well.  This makes the creation of arrays in loops possible:
	 * 
	 *	<setString result="array_$I(index)" value="element $I(index) in array"/>
	 *	
	 * This may be retrieved using the following (value is treated as the variable
	 * name):
	 * 
	 *	<setStringFrom result="out" value="array_$I(index)"/>
	 *	
	 * For boolean and integer types, you may use <setBoolFrom/> and <setIntFrom/>.
	 * 
	 * Nodes for inserting text into the section are as follows:
	 * 
	 *	<insert> body </insert>
	 *	<insertValue value=""/>
	 *	<loadLibrary name="" failmsg=""/>
	 *	
	 * The first one will insert either text or nodes into the returned section
	 * string.  Please note you must use escaped characters for:
	 * 
	 *	< = &lt;
	 *	> = &gt;
	 *	& = &amp;
	 * 
	 *	<insertValue value=""/> evaluates variables in value before insertion.  If value contains
	 * double quote ("), you must escape it with &quot;.
	 * 
	 *	<loadLibrary name="" failmsg=""/> is a shortcut to insert a system.LoadLibrary try/catch
	 * block with an optional logged message for failure to load case.
	 * 
	 * You can do a search and replace in the Output with:
	 * 
	 *	<replace find="" with=""/>
	 * 
	 * Note you can also manipulate the actual $S(Output) directly, the above are more efficient:
	 * 
	 *	<setStringAdd result="Input" arg1="$S(Output)" arg2="sample\n"/>
	 *	<setStringReplace result="Input" source="$S(Output)" find=".LAUNCH" with=".INFO"/>
	 *	
	 * XML manipulation uses the following nodes:
	 * 
	 *	<addElement tag="" name=""/>
	 *	<addElements tag=""> body </addElements>
	 *	<removeElement tag=""/>
	 *	<setStringFromTag result="" tag="" name=""/>
	 *	<setStringFromAttribute result="" tag="" name=""/>
	 *	<addAttribute tag="" name="" value=""/>
	 *	<removeAttribute tag="" name=""/>
	 *	<loopElements tag=""> instructions </loopElements>
	 *	
	 * The current element is referenced with tag="$".  Element variables are referenced with $varname
	 * since using $E(varname) will be expanded to the string equivalent of the XML.
	 * 
	 * <uses-permission> and <uses-feature> are updated with:
	 * 
	 *	<addPermission android:name="" .. />
	 *	<addFeature android:name="" .. />
	 *
	 * Any attributes in the above commands are copied to the element added to the manifest so you
	 * can do the following, for example:
	 * 
	 *	<addFeature android:name="android.hardware.usb.host" android:required="true"/>
	 *	
	 * Finally, these nodes allow copying of files useful for staging jar and so files:
	 * 
	 *	<copyFile src="" dst=""/>
	 *	<copyDir src="" dst=""/>
	 *	
	 * The following should be used as the base for the src and dst paths:
	 * 
	 *	$S(PluginDir) = directory the XML file was loaded from
 	 *	$S(EngineDir) = engine directory
	 *	$S(BuildDir) = project's Intermediate/Android/APK directory
	 *	
	 * While it is possible to write outside the APK directory, it is not recommended.
	 * 
	 * If you must remove files (like development-only files from distribution builds) you can
	 * use this node:
	 * 
	 *	<removeFiles mask=""/>
	 *	
	 * It is restricted to only removing files from the BuildDir.  Here is example usage to remove
	 * the Oculus Signature Files (osig) from the assets directory:
	 * 
	 *	<removeFiles mask="assets/oculussig_*"/>
	 *	
	 * The following sections are evaluated during the packaging or deploy stages:
	 * 
	 *	<!-- init section is always evaluated once per architecture -->
	 * 	<init> </init>
	 * 	
	 * 	<!-- optional updates applied to AndroidManifest.xml -->
	 * 	<androidManifestUpdates> </androidManifestUpdates>
	 * 	
	 * 	<!-- optional additions to proguard -->
	 * 	<proguardAdditions>	</proguardAdditions>
	 * 	
	 * 	<!-- optional files or directories to copy or delete from Intermediate/Android/APK -->
	 * 	<resourceCopies> </resourceCopies>
	 * 	
	 * 	<!-- optional additions to the GameActivity imports in GameActivity.java -->
	 * 	<gameActivityImportAdditions> </gameActivityImportAdditions>
	 * 	
	 * 	<!-- optional additions to the GameActivity class in GameActivity.java -->
	 * 	<gameActivityClassAdditions> </gameActivityOnClassAdditions>
	 * 	
	 * 	<!-- optional additions to GameActivity onCreate metadata reading in GameActivity.java -->
	 * 	<gameActivityReadMetadata> </gameActivityReadMetadata>
	 * 
	 *	<!-- optional additions to GameActivity onCreate in GameActivity.java -->
	 *	<gameActivityOnCreateAdditions> </gameActivityOnCreateAdditions>
	 * 	
	 * 	<!-- optional additions to GameActivity onDestroy in GameActivity.java -->
	 * 	<gameActivityOnDestroyAdditions> </gameActivityOnDestroyAdditions>
	 * 	
	 * 	<!-- optional additions to GameActivity onStart in GameActivity.java -->
	 * 	<gameActivityOnStartAdditions> </gameActivityOnStartAdditions>
	 * 	
	 * 	<!-- optional additions to GameActivity onStop in GameActivity.java -->
	 * 	<gameActivityOnStopAdditions> </gameActivityOnStopAdditions>
	 * 	
	 * 	<!-- optional additions to GameActivity onPause in GameActivity.java -->
	 * 	<gameActivityOnPauseAdditions> </gameActivityOnPauseAdditions>
	 * 	
	 * 	<!-- optional additions to GameActivity onResume in GameActivity.java -->
	 *	<gameActivityOnResumeAdditions>	</gameActivityOnResumeAdditions>
	 *	
	 * 	<!-- optional additions to GameActivity onActivityResult in GameActivity.java -->
	 * 	<gameActivityOnActivityResultAdditions>	</gameActivityOnActivityResultAdditions>
	 * 	
	 * 	<!-- optional libraries to load in GameActivity.java before libUE4.so -->
	 * 	<soLoadLibrary>	</soLoadLibrary>
	 * 	
	 * 
	 * Here is the complete list of supported nodes:
	 * 
	 * <isArch arch="">
	 * <isDistribution>
	 * <if> => <true> / <false>
	 * <while condition="">
	 * <return/>
	 * <break/>
	 * <continue/>
	 * <log text=""/>
	 * <insert> </insert>
	 * <insertValue value=""/>
	 * <replace find="" with""/>
	 * <copyFile src="" dst=""/>
	 * <copyDir src="" dst=""/>
	 * <loadLibrary name="" failmsg=""/>
	 * <setBool result="" value=""/>
	 * <setBoolFrom result="" value=""/>
	 * <setBoolFromProperty result="" ini="" section="" property="" default=""/>
	 * <setBoolNot result="" source=""/>
	 * <setBoolAnd result="" arg1="" arg2=""/>
	 * <setBoolOr result="" arg1="" arg2=""/>
	 * <setBoolIsEqual result="" arg1="" arg2=""/>
	 * <setBoolIsLess result="" arg1="" arg2=""/>
	 * <setBoolIsLessEqual result="" arg1="" arg2=""/>
	 * <setBoolIsGreater result="" arg1="" arg2=""/>
	 * <setBoolIsGreaterEqual result="" arg1="" arg2=""/>
	 * <setInt result="" value=""/>
	 * <setIntFrom result="" value=""/>
	 * <setIntFromProperty result="" ini="" section="" property="" default=""/>
	 * <setIntAdd result="" arg1="" arg2=""/>
	 * <setIntSubtract result="" arg1="" arg2=""/>
	 * <setIntMultiply result="" arg1="" arg2=""/>
	 * <setIntDivide result="" arg1="" arg2=""/>
	 * <setIntLength result="" source=""/>
	 * <setIntFindString result="" source="" find=""/>
	 * <setString result="" value=""/>
	 * <setStringFrom result="" value=""/>
	 * <setStringFromProperty result="" ini="" section="" property="" default=""/>
	 * <setStringAdd result="" arg1="" arg2=""/>
	 * <setStringSubstring result="" source="" index="" length=""/>
	 * <setStringReplace result="" source="" find="" with=""/>
	 * 
	 */

	public class AndroidPluginLanguage
	{
		/** The merged XML program to run */
		private XDocument XDoc;

		/** Android namespace */
		private XNamespace AndroidNameSpace;

		/** Trace flag to enable debugging */
		static private bool bGlobalTrace = false;

		static private XDocument XMLDummy = XDocument.Parse("<manifest></manifest>");

		private class APLContext
		{
			/** Variable state */
			public Dictionary<string, bool> BoolVariables;
			public Dictionary<string, int> IntVariables;
			public Dictionary<string, string> StringVariables;
			public Dictionary<string, XElement> ElementVariables;

			/** Local context trace */
			public bool bTrace;

			public APLContext(string Architecture, string PluginDir)
			{
				BoolVariables = new Dictionary<string, bool>();
				IntVariables = new Dictionary<string, int>();
				StringVariables = new Dictionary<string, string>();
				ElementVariables = new Dictionary<string, XElement>();

				StringVariables["Architecture"] = Architecture;
				StringVariables["PluginDir"] = PluginDir;

				bTrace = false;
			}
		}

		private APLContext GlobalContext;
		private Dictionary<string, APLContext> Contexts;
		private int ContextIndex;

		public AndroidPluginLanguage(List<string> XMLFiles, List<string> Architectures)
		{
			Contexts = new Dictionary<string, APLContext>();
			GlobalContext = new APLContext("", "");
			ContextIndex = 0;

			AndroidNameSpace = "http://schemas.android.com/apk/res/android";

			string PathPrefix = Path.GetFileName(Directory.GetCurrentDirectory()).Equals("Source") ? ".." : "Engine";

			XDoc = XDocument.Parse("<root xmlns:android=\"http://schemas.android.com/apk/res/android\"></root>");
			foreach (string Basename in XMLFiles)
			{
				string Filename = Path.Combine(PathPrefix, Basename.Replace("\\", "/"));
				Log.TraceInformation("\nAPL: {0}", Filename);
				if (File.Exists(Filename))
				{
					string PluginDir = Path.GetDirectoryName(Filename);
					try
					{
						XDocument MergeDoc = XDocument.Load(Filename);
						MergeXML(MergeDoc, PluginDir, Architectures);
					}
					catch (Exception e)
					{
						Log.TraceError("\nAndroid Plugin file {0} parsing failed! {1}", Filename, e);
					}
				}
				else
				{
					Log.TraceError("\nAndroid Plugin file {0} missing!", Filename);
					Log.TraceInformation("\nCWD: {0}", Directory.GetCurrentDirectory());
				}
			}
		}

		public bool GetTrace() { return bGlobalTrace; }
		public void SetTrace() { bGlobalTrace = true; }
		public void ClearTrace() { bGlobalTrace = false; }

		public bool MergeXML(XDocument MergeDoc, string PluginDir, List<string> Architectures)
		{
			if (MergeDoc == null)
			{
				return false;
			}
	
			// create a context for each architecture
			ContextIndex++;
			foreach (string Architecture in Architectures)
			{
				APLContext Context = new APLContext(Architecture, PluginDir);
				Contexts[Architecture + "_" + ContextIndex] = Context;
			}

			// merge in the nodes
			foreach (var Element in MergeDoc.Root.Elements())
			{
				var Parent = XDoc.Root.Element(Element.Name);
				if (Parent != null)
				{
					var Entry = new XElement("Context", new XAttribute("index", ContextIndex.ToString()));
					Entry.Add(Element.Elements());
					Parent.Add(Entry);
				}
				else
				{
					var Entry = new XElement("Context", new XAttribute("index", ContextIndex.ToString()));
					Entry.Add(Element.Elements());
					var Base = new XElement(Element.Name);
					Base.Add(Entry);
					XDoc.Root.Add(Base);
				}
			}

			return true;
		}

		public void SaveXML(string Filename)
		{
			if (XDoc != null)
			{
				XDoc.Save(Filename);
			}
		}

		private string DumpContext(APLContext Context)
		{
			StringBuilder Text = new StringBuilder();
			foreach (var Variable in Context.BoolVariables)
			{
				Text.AppendLine(string.Format("\tbool {0} = {1}", Variable.Key, Variable.Value.ToString().ToLower()));
			}
			foreach (var Variable in Context.IntVariables)
			{
				Text.AppendLine(string.Format("\tint {0} = {1}", Variable.Key, Variable.Value));
			}
			foreach (var Variable in Context.StringVariables)
			{
				Text.AppendLine(string.Format("\tstring {0} = {1}", Variable.Key, Variable.Value));
			}
			foreach (var Variable in Context.ElementVariables)
			{
				Text.AppendLine(string.Format("\telement {0} = {1}", Variable.Key, Variable.Value));
			}
			return Text.ToString();
		}

		public string DumpVariables()
		{
			string Result = "Global Context:\n" + DumpContext(GlobalContext);
			foreach (var Context in Contexts)
			{
				Result += "Context " + Context.Key + ": " + Context.Value.StringVariables["PluginDir"] + "\n" + DumpContext(Context.Value);
			}
			return Result;
		}

		private bool GetCondition(APLContext Context, XElement Node, string Condition, out bool Result)
		{
			Result = false;
			if (!Context.BoolVariables.TryGetValue(Condition, out Result))
			{
				if (!GlobalContext.BoolVariables.TryGetValue(Condition, out Result))
				{
					Log.TraceWarning("\nMissing condition '{0}' in '{1}' (skipping instruction)", Condition, TraceNodeString(Node));
					return false;
				}
			}
			return true;
		}

		private string ExpandVariables(APLContext Context, string InputString)
		{
			string Result = InputString;
			for (int Idx = Result.IndexOf("$B("); Idx != -1; Idx = Result.IndexOf("$B(", Idx))
			{
				// Find the end of the variable name
				int EndIdx = Result.IndexOf(')', Idx + 3);
				if (EndIdx == -1)
				{
					break;
				}

				// Extract the variable name from the string
				string Name = Result.Substring(Idx + 3, EndIdx - (Idx + 3));

				// Find the value for it, either from the dictionary or the environment block
				bool Value;
				if (!Context.BoolVariables.TryGetValue(Name, out Value))
				{
					if (!GlobalContext.BoolVariables.TryGetValue(Name, out Value))
					{
						Idx = EndIdx + 1;
						continue;
					}
				}

				// Replace the variable, or skip past it
				Result = Result.Substring(0, Idx) + Value.ToString().ToLower() + Result.Substring(EndIdx + 1);
			}
			for (int Idx = Result.IndexOf("$I("); Idx != -1; Idx = Result.IndexOf("$I(", Idx))
			{
				// Find the end of the variable name
				int EndIdx = Result.IndexOf(')', Idx + 3);
				if (EndIdx == -1)
				{
					break;
				}

				// Extract the variable name from the string
				string Name = Result.Substring(Idx + 3, EndIdx - (Idx + 3));

				// Find the value for it, either from the dictionary or the environment block
				int Value;
				if (!Context.IntVariables.TryGetValue(Name, out Value))
				{
					if (!GlobalContext.IntVariables.TryGetValue(Name, out Value))
					{
						Idx = EndIdx + 1;
						continue;
					}
				}

				// Replace the variable, or skip past it
				Result = Result.Substring(0, Idx) + Value.ToString() + Result.Substring(EndIdx + 1);
			}
			for (int Idx = Result.IndexOf("$S("); Idx != -1; Idx = Result.IndexOf("$S(", Idx))
			{
				// Find the end of the variable name
				int EndIdx = Result.IndexOf(')', Idx + 3);
				if (EndIdx == -1)
				{
					break;
				}

				// Extract the variable name from the string
				string Name = Result.Substring(Idx + 3, EndIdx - (Idx + 3));

				// Find the value for it, either from the dictionary or the environment block
				string Value;
				if (!Context.StringVariables.TryGetValue(Name, out Value))
				{
					if (!GlobalContext.StringVariables.TryGetValue(Name, out Value))
					{
						Idx = EndIdx + 1;
						continue;
					}
				}

				// Replace the variable, or skip past it
				Result = Result.Substring(0, Idx) + Value + Result.Substring(EndIdx + 1);
			}
			for (int Idx = Result.IndexOf("$E("); Idx != -1; Idx = Result.IndexOf("$E(", Idx))
			{
				// Find the end of the variable name
				int EndIdx = Result.IndexOf(')', Idx + 3);
				if (EndIdx == -1)
				{
					break;
				}

				// Extract the variable name from the string
				string Name = Result.Substring(Idx + 3, EndIdx - (Idx + 3));

				// Find the value for it, either from the dictionary or the environment block
				XElement Value;
				if (!Context.ElementVariables.TryGetValue(Name, out Value))
				{
					if (!GlobalContext.ElementVariables.TryGetValue(Name, out Value))
					{
						Idx = EndIdx + 1;
						continue;
					}
				}

				// Replace the variable, or skip past it
				Result = Result.Substring(0, Idx) + Value + Result.Substring(EndIdx + 1);
			}
			return Result;
		}

		private string TraceNodeString(XElement Node)
		{
			string Result = Node.Name.ToString();
			foreach (var Attrib in Node.Attributes())
			{
				Result += " " + Attrib.ToString();
			}
			return Result;
		}

		private bool StringToBool(string Input)
		{
			if (Input == null)
			{
				return false;
			}
			Input = Input.ToLower();
			return !(Input.Equals("0") || Input.Equals("false") || Input.Equals("off") || Input.Equals("no"));
		}

		private int StringToInt(string Input, XElement Node)
		{
			int Result = 0;
			if (!int.TryParse(Input, out Result))
			{
				Log.TraceWarning("\nInvalid integer '{0}' in '{1}' (defaulting to 0)", Input, TraceNodeString(Node));
			}
			return Result;
		}

		private string GetAttribute(APLContext Context, XElement Node, string AttributeName, bool bExpand = true, bool bRequired = true, string Fallback = null)
		{
			XAttribute Attribute = Node.Attribute(AttributeName);
			if (Attribute == null)
			{
				if (bRequired)
				{
					Log.TraceWarning("\nMissing attribute '{0}' in '{1}' (skipping instruction)", AttributeName, TraceNodeString(Node));
				}
				return Fallback;
			}
			string Result = Attribute.Value;
			return bExpand ? ExpandVariables(Context, Result) : Result;
		}

		private string GetAttributeWithNamespace(APLContext Context, XElement Node, XNamespace Namespace, string AttributeName, bool bExpand = true, bool bRequired = true, string Fallback = null)
		{
			XAttribute Attribute = Node.Attribute(Namespace + AttributeName);
			if (Attribute == null)
			{
				if (bRequired)
				{
					Log.TraceWarning("\nMissing attribute '{0}' in '{1}' (skipping instruction)", AttributeName, TraceNodeString(Node));
				}
				return Fallback;
			}
			string Result = Attribute.Value;
			return bExpand ? ExpandVariables(Context, Result) : Result;
		}

		static private Dictionary<string, ConfigCacheIni> ConfigCache = null;

		private static ConfigCacheIni GetConfigCacheIni(string baseIniName)
		{
			if (ConfigCache == null)
			{
				ConfigCache = new Dictionary<string, ConfigCacheIni>();
			}
			ConfigCacheIni config = null;
			if (!ConfigCache.TryGetValue(baseIniName, out config))
			{
				config = new ConfigCacheIni(UnrealTargetPlatform.Android, "Engine", UnrealBuildTool.GetUProjectPath());
				ConfigCache.Add(baseIniName, config);
			}
			return config;
		}

		private static void CopyFileDirectory(string SourceDir, string DestDir)
		{
			if (!Directory.Exists(SourceDir))
			{
				return;
			}

			string[] Files = Directory.GetFiles(SourceDir, "*.*", SearchOption.AllDirectories);
			foreach (string Filename in Files)
			{
				// make the dst filename with the same structure as it was in SourceDir
				string DestFilename = Path.Combine(DestDir, Utils.MakePathRelativeTo(Filename, SourceDir));
				if (File.Exists(DestFilename))
				{
					File.Delete(DestFilename);
				}

				// make the subdirectory if needed
				string DestSubdir = Path.GetDirectoryName(DestFilename);
				if (!Directory.Exists(DestSubdir))
				{
					Directory.CreateDirectory(DestSubdir);
				}

				File.Copy(Filename, DestFilename);

				// remove any read only flags
				FileInfo DestFileInfo = new FileInfo(DestFilename);
				DestFileInfo.Attributes = DestFileInfo.Attributes & ~FileAttributes.ReadOnly;
			}
		}

		private static void DeleteFiles(string Filespec)
		{
			string BaseDir = Path.GetDirectoryName(Filespec);
			string Mask = Path.GetFileName(Filespec);

			if (!Directory.Exists(BaseDir))
			{
				return;
			}

			string[] Files = Directory.GetFiles(BaseDir, Mask, SearchOption.TopDirectoryOnly);
			foreach (string Filename in Files)
			{
				File.Delete(Filename);
				Log.TraceInformation("\nDeleted file {0}", Filename);
			}
		}

		private void AddAttribute(XElement Element, string Name, string Value)
		{
			XAttribute Attribute;
			int Index = Name.IndexOf(":");
			if (Index >= 0)
			{
				Name = Name.Substring(Index + 1);
				Attribute = Element.Attribute(AndroidNameSpace + Name);
			}
			else
			{
				Attribute = Element.Attribute(Name);
			}

			if (Attribute != null)
			{
				Attribute.SetValue(Value);
			}
			else
			{
				if (Index >= 0)
				{
					Element.Add(new XAttribute(AndroidNameSpace + Name, Value));
				}
				else
				{
					Element.Add(new XAttribute(Name, Value));
				}
			}
		}

		private void RemoveAttribute(XElement Element, string Name)
		{
			XAttribute Attribute;
			int Index = Name.IndexOf(":");
			if (Index >= 0)
			{
				Name = Name.Substring(Index + 1);
				Attribute = Element.Attribute(AndroidNameSpace + Name);
			}
			else
			{
				Attribute = Element.Attribute(Name);
			}

			if (Attribute != null)
			{
				Attribute.Remove();
			}
		}

		private void AddElements(XElement Target, XElement Source)
		{
			if (Source.HasElements)
			{
				foreach (var Index in Source.Elements())
				{
//					if (Target.Element(Index.Name) == null)
					{
						Target.Add(Index);
					}
				}
			}
		}

		public string ProcessPluginNode(string Architecture, string NodeName, string Input)
		{
			return ProcessPluginNode(Architecture, NodeName, Input, ref XMLDummy);
		}

		public string ProcessPluginNode(string Architecture, string NodeName, string Input, ref XDocument XMLWork)
		{
			// add all instructions to execution list
			var ExecutionStack = new Stack<XElement>();
			var StartNode = XDoc.Root.Element(NodeName);
			if (StartNode != null)
			{
				foreach (var Instruction in StartNode.Elements().Reverse())
				{
					ExecutionStack.Push(Instruction);
				}
			}

			if (ExecutionStack.Count == 0)
			{
				return Input;
			}

			var ContextStack = new Stack<APLContext>();
			APLContext CurrentContext = GlobalContext;

			// update Output in global context
			GlobalContext.StringVariables["Output"] = Input;

			var ElementStack = new Stack<XElement>();
			XElement CurrentElement = XMLWork.Elements().First();

			// run the instructions
			while (ExecutionStack.Count > 0)
			{
				var Node = ExecutionStack.Pop();
				if (bGlobalTrace || CurrentContext.bTrace)
				{
					Log.TraceInformation("Execute: '{0}'", TraceNodeString(Node));
				}
				switch (Node.Name.ToString())
				{
					case "trace":
						{
							string Enable = GetAttribute(CurrentContext, Node, "enable");
							if (Enable != null)
							{
								CurrentContext.bTrace = StringToBool(Enable);
								if (!bGlobalTrace && CurrentContext.bTrace)
								{
									Log.TraceInformation("Context: '{0}' using Architecture='{1}', NodeName='{2}', Input='{3}'", CurrentContext.StringVariables["PluginDir"], Architecture, NodeName, Input);
								}
							}
						}
						break;

					case "dumpvars":
						{
							if (!bGlobalTrace && !CurrentContext.bTrace)
							{
								Log.TraceInformation("Context: '{0}' using Architecture='{1}', NodeName='{2}', Input='{3}'", CurrentContext.StringVariables["PluginDir"], Architecture, NodeName, Input);
							}
							Log.TraceInformation("Variables:\n{0}", DumpContext(CurrentContext));
						}
						break;

					case "Context":
						{
							ContextStack.Push(CurrentContext);
							string index = GetAttribute(CurrentContext, Node, "index");
							CurrentContext = Contexts[Architecture + "_" + index];
							ExecutionStack.Push(new XElement("PopContext"));
							foreach (var instruction in Node.Elements().Reverse())
							{
								ExecutionStack.Push(instruction);
							}

							if (bGlobalTrace || CurrentContext.bTrace)
							{
								Log.TraceInformation("Context: '{0}' using Architecture='{1}', NodeName='{2}', Input='{3}'", CurrentContext.StringVariables["PluginDir"], Architecture, NodeName, Input);
							}
						}
						break;

					case "PopContext":
						{
							CurrentContext = ContextStack.Pop();
						}
						break;

					case "PopElement":
						{
							CurrentElement = ElementStack.Pop();
						}
						break;

					case "isArch":
						{
							string arch = GetAttribute(CurrentContext, Node, "arch");
							if (arch != null && arch.Equals(Architecture))
							{
								foreach (var instruction in Node.Elements().Reverse())
								{
									ExecutionStack.Push(instruction);
								}
							}
						}
						break;

					case "isDistribution":
						{
							bool Result = false;
							if (GetCondition(CurrentContext, Node, "Distribution", out Result))
							{
								if (Result)
								{
									foreach (var Instruction in Node.Elements().Reverse())
									{
										ExecutionStack.Push(Instruction);
									}
								}
							}
						}
						break;

					case "if":
						{
							bool Result;
							if (GetCondition(CurrentContext, Node, GetAttribute(CurrentContext, Node, "condition"), out Result))
							{
								var ResultNode = Node.Element(Result ? "true" : "false");
								if (ResultNode != null)
								{
									foreach (var Instruction in ResultNode.Elements().Reverse())
									{
										ExecutionStack.Push(Instruction);
									}
								}
							}
						}
						break;

					case "while":
						{
							bool Result;
							if (GetCondition(CurrentContext, Node, GetAttribute(CurrentContext, Node, "condition"), out Result))
							{
								if (Result)
								{
									var ResultNode = Node.Elements();
									if (ResultNode != null)
									{
										ExecutionStack.Push(Node);
										foreach (var Instruction in ResultNode.Reverse())
										{
											ExecutionStack.Push(Instruction);
										}
									}
								}
							}
						}
						break;

					case "return":
						{
							while (ExecutionStack.Count > 0)
							{
								ExecutionStack.Pop();
							}
						}
						break;

					case "break":
						{
							// remove up to while (or acts like a return if outside by removing everything)
							while (ExecutionStack.Count > 0)
							{
								Node = ExecutionStack.Pop();
								if (Node.Name.ToString().Equals("while"))
								{
									break;
								}
							}
						}
						break;

					case "continue":
						{
							// remove up to while (or acts like a return if outside by removing everything)
							while (ExecutionStack.Count > 0)
							{
								Node = ExecutionStack.Pop();
								if (Node.Name.ToString().Equals("while"))
								{
									ExecutionStack.Push(Node);
									break;
								}
							}
						}
						break;

					case "log":
						{
							string Text = GetAttribute(CurrentContext, Node, "text");
							if (Text != null)
							{
								Log.TraceInformation("{0}", Text);
							}
						}
						break;

					case "loopElements":
						{
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							ElementStack.Push(CurrentElement);
							ExecutionStack.Push(new XElement("PopElement"));
							var WorkList = (Tag == "$") ? CurrentElement.Elements().Reverse() : CurrentElement.Descendants(Tag).Reverse();
							foreach (var WorkNode in WorkList)
							{
								foreach (var Instruction in Node.Elements().Reverse())
								{
									ExecutionStack.Push(Instruction);
								}
								ElementStack.Push(WorkNode);
								ExecutionStack.Push(new XElement("PopElement"));
							}
						}
						break;

					case "addAttribute":
						{
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							string Name = GetAttribute(CurrentContext, Node, "name");
							string Value = GetAttribute(CurrentContext, Node, "value");
							if (Tag != null && Name != null && Value != null)
							{
								if (Tag.StartsWith("$"))
								{
									XElement Target = CurrentElement;
									if (Tag.Length > 1)
									{
										if (!CurrentContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
										{
											if (!GlobalContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
											{
												Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
												continue;
											}
										}
									}
									AddAttribute(Target, Name, Value);
								}
								else
								{
									if (CurrentElement.Name.ToString().Equals(Tag))
									{
										AddAttribute(CurrentElement, Name, Value);
									}
									foreach (var WorkNode in CurrentElement.Descendants(Tag))
									{
										AddAttribute(WorkNode, Name, Value);
									}
								}
							}
						}
						break;

					case "removeAttribute":
						{
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							string Name = GetAttribute(CurrentContext, Node, "name");
							if (Tag != null && Name != null)
							{
								if (Tag.StartsWith("$"))
								{
									XElement Target = CurrentElement;
									if (Tag.Length > 1)
									{
										if (!CurrentContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
										{
											if (!GlobalContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
											{
												Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
												continue;
											}
										}
									}
									RemoveAttribute(Target, Name);
								}
								else
								{
									if (CurrentElement.Name.ToString().Equals(Tag))
									{
										RemoveAttribute(CurrentElement, Name);
									}
									foreach (var WorkNode in CurrentElement.Descendants(Tag))
									{
										RemoveAttribute(WorkNode, Name);
									}
								}
							}
						}
						break;

					case "addPermission":
						{
							string Name = GetAttributeWithNamespace(CurrentContext, Node, AndroidNameSpace, "name");
							if (Name != null)
							{
								// make sure it isn't already added
								bool bFound = false;
								foreach (var Element in XMLWork.Descendants("uses-permission"))
								{
									XAttribute Attribute = Element.Attribute(AndroidNameSpace + "name");
									if (Attribute != null)
									{
										if (Attribute.Value == Name)
										{
											bFound = true;
											break;
										}
									}
								}

								// add it if not found
								if (!bFound)
								{
									XMLWork.Element("manifest").Add(new XElement("uses-permission", Node.Attributes()));
								}
							}
						}
						break;

					case "removePermission":
						{
							string Name = GetAttributeWithNamespace(CurrentContext, Node, AndroidNameSpace, "name");
							if (Name != null)
							{
								foreach (var Element in XMLWork.Descendants("uses-permission"))
								{
									XAttribute Attribute = Element.Attribute(AndroidNameSpace + "name");
									if (Attribute != null)
									{
										if (Attribute.Value == Name)
										{
											Element.Remove();
											break;
										}
									}
								}
							}
						}
						break;

					case "addFeature":
						{
							string Name = GetAttributeWithNamespace(CurrentContext, Node, AndroidNameSpace, "name");
							if (Name != null)
							{
								// make sure it isn't already added
								bool bFound = false;
								foreach (var Element in XMLWork.Descendants("uses-feature"))
								{
									XAttribute Attribute = Element.Attribute(AndroidNameSpace + "name");
									if (Attribute != null)
									{
										if (Attribute.Value == Name)
										{
											bFound = true;
											break;
										}
									}
								}

								// add it if not found
								if (!bFound)
								{
									XMLWork.Element("manifest").Add(new XElement("uses-feature", Node.Attributes()));
								}
							}
						}
						break;

					case "removeFeature":
						{
							string Name = GetAttributeWithNamespace(CurrentContext, Node, AndroidNameSpace, "name");
							if (Name != null)
							{
								foreach (var Element in XMLWork.Descendants("uses-feature"))
								{
									XAttribute Attribute = Element.Attribute(AndroidNameSpace + "name");
									if (Attribute != null)
									{
										if (Attribute.Value == Name)
										{
											Element.Remove();
											break;
										}
									}
								}
							}
						}
						break;

					case "removeElement":
						{
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							if (Tag != null)
							{
								if (Tag == "$")
								{
									XElement Parent = CurrentElement.Parent;
									CurrentElement.Remove();
									CurrentElement = Parent;
								}
								else
								{
									// use a list since Remove() may modify it
									foreach (var Element in XMLWork.Descendants(Tag).ToList())
									{
										Element.Remove();
									}
								}
							}
						}
						break;

					case "addElement":
						{
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							string Name = GetAttribute(CurrentContext, Node, "name");
							if (Tag != null && Name != null)
							{
								XElement Element;
								if (!CurrentContext.ElementVariables.TryGetValue(Name, out Element))
								{
									if (!GlobalContext.ElementVariables.TryGetValue(Name, out Element))
									{
										Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
										continue;
									}
								}
								if (Tag.StartsWith("$"))
								{
									XElement Target = CurrentElement;
									if (Tag.Length > 1)
									{
										if (!CurrentContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
										{
											if (!GlobalContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
											{
												Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
												continue;
											}
										}
									}
									Target.Add(new XElement(Element));
								}
								else
								{
									if (CurrentElement.Name.ToString().Equals(Tag))
									{
										CurrentElement.Add(new XElement(Element));
									}
									foreach (var WorkNode in CurrentElement.Descendants(Tag))
									{
										WorkNode.Add(new XElement(Element));
									}
								}
							}
						}
						break;

					case "addElements":
						{
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							if (Tag != null)
							{
								if (Tag.StartsWith("$"))
								{
									XElement Target = CurrentElement;
									if (Tag.Length > 1)
									{
										if (!CurrentContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
										{
											if (!GlobalContext.ElementVariables.TryGetValue(Tag.Substring(1), out Target))
											{
												Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
												continue;
											}
										}
									}
									AddElements(Target, Node);
								}
								else
								{
									if (CurrentElement.Name.ToString().Equals(Tag))
									{
										AddElements(CurrentElement, Node);
									}
									foreach (var WorkNode in CurrentElement.Descendants(Tag))
									{
										AddElements(WorkNode, Node);
									}
								}
							}
						}
						break;

					case "insert":
						{
							if (Node.HasElements)
							{
								foreach (var Element in Node.Elements())
								{
									string Value = Element.ToString().Replace(" xmlns:android=\"http://schemas.android.com/apk/res/android\" ", "");
									GlobalContext.StringVariables["Output"] += Value + "\n";
								}
							}
							else
							{
								string Value = Node.Value.ToString();

								// trim trailing tabs
								int Index = Value.Length;
								while (Index > 0 && Value[Index - 1] == '\t')
								{
									Index--;
								}
								if (Index < Value.Length)
								{
									Value = Value.Substring(0, Index);
								}

								// trim leading newlines
								Index = 0;
								while (Index < Value.Length && Value[Index] == '\n')
								{
									Index++;
								}

								if (Index < Value.Length)
								{
									GlobalContext.StringVariables["Output"] += Value.Substring(Index);
								}
							}
						}
						break;

					case "insertValue":
						{
							string Value = GetAttribute(CurrentContext, Node, "value");
							if (Value != null)
							{
								GlobalContext.StringVariables["Output"] += Value;
							}
						}
						break;

					case "replace":
						{
							string Find = GetAttribute(CurrentContext, Node, "find");
							string With = GetAttribute(CurrentContext, Node, "with");
							if (Find != null && With != null)
							{
								GlobalContext.StringVariables["Output"] = GlobalContext.StringVariables["Output"].Replace(Find, With);
							}
						}
						break;

					case "copyFile":
						{
							string Src = GetAttribute(CurrentContext, Node, "src");
							string Dst = GetAttribute(CurrentContext, Node, "dst");
							if (Src != null && Dst != null)
							{
								if (File.Exists(Src))
								{
									// check to see if newer than last time we copied
									bool bFileExists = File.Exists(Dst);
									TimeSpan Diff = File.GetLastWriteTimeUtc(Dst) - File.GetLastWriteTimeUtc(Src);
									if (!bFileExists || Diff.TotalSeconds < -1 || Diff.TotalSeconds > 1)
									{
										if (bFileExists)
										{
											File.Delete(Dst);
										}
										Directory.CreateDirectory(Path.GetDirectoryName(Dst));
										File.Copy(Src, Dst, true);
										Log.TraceInformation("\nFile {0} copied to {1}", Src, Dst);
									}
								}
							}
						}
						break;

					case "copyDir":
						{
							string Src = GetAttribute(CurrentContext, Node, "src");
							string Dst = GetAttribute(CurrentContext, Node, "dst");
							if (Src != null && Dst != null)
							{
								CopyFileDirectory(Src, Dst);
								Log.TraceInformation("\nDirectory {0} copied to {1}", Src, Dst);
							}
						}
						break;

					case "deleteFiles":
						{
							string Filespec = GetAttribute(CurrentContext, Node, "filespec");
							if (Filespec != null)
							{
								if (Filespec.Contains(":") || Filespec.Contains(".."))
								{
									Log.TraceInformation("\nFilespec {0} not allowed; ignored.", Filespec);
								}
								else
								{
									// force relative to BuildDir (and only from global context so someone doesn't try to be clever)
									DeleteFiles(Path.Combine(GlobalContext.StringVariables["BuildDir"], Filespec));
								}
							}
						}
						break;

					case "loadLibrary":
						{
							string Name = GetAttribute(CurrentContext, Node, "name");
							string FailMsg = GetAttribute(CurrentContext, Node, "failmsg", true, false);
							if (Name != null)
							{
								string Work = "\t\ttry\n" +
											"\t\t{\n" +
											"\t\t\tSystem.loadLibrary(\"" + Name + "\");\n" +
											"\t\t}\n" +
											"\t\tcatch (java.lang.UnsatisfiedLinkError e)\n" +
											"\t\t{\n";
								if (FailMsg != null)
								{
									Work += "\t\t\tLog.debug(\"" + FailMsg + "\");\n";
								}
								GlobalContext.StringVariables["Output"] += Work + "\t\t}\n";
							}
						}
						break;

					case "setBool":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false, "false");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = StringToBool(Value);
							}
						}
						break;

					case "setBoolFrom":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false, "false");
							if (Result != null)
							{
								Value = ExpandVariables(CurrentContext, "$B(" + Value + ")");
								CurrentContext.BoolVariables[Result] = StringToBool(Value);
							}
						}
						break;

					case "setBoolFromProperty":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Ini = GetAttribute(CurrentContext, Node, "ini");
							string Section = GetAttribute(CurrentContext, Node, "section");
							string Property = GetAttribute(CurrentContext, Node, "property");
							string DefaultVal = GetAttribute(CurrentContext, Node, "default", true, false, "false");
							if (Result != null && Ini != null && Section != null && Property != null)
							{
								bool Value = StringToBool(DefaultVal);

								ConfigCacheIni ConfigIni = GetConfigCacheIni(Ini);
								if (ConfigIni != null)
								{
									ConfigIni.GetBool(Section, Property, out Value);
								}
								CurrentContext.BoolVariables[Result] = Value;
							}
						}
						break;

					case "setBoolContains":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							string Find = GetAttribute(CurrentContext, Node, "find", true, false, "");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = Source.Contains(Find);
							}
						}
						break;

					case "setBoolStartsWith":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							string Find = GetAttribute(CurrentContext, Node, "find", true, false, "");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = Source.StartsWith(Find);
							}
						}
						break;

					case "setBoolEndsWith":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							string Find = GetAttribute(CurrentContext, Node, "find", true, false, "");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = Source.EndsWith(Find);
							}
						}
						break;

					case "setBoolNot":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "false");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = !StringToBool(Source);
							}
						}
						break;

					case "setBoolAnd":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "false");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "false");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = StringToBool(Arg1) && StringToBool(Arg2);
							}
						}
						break;

					case "setBoolOr":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "false");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "false");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = StringToBool(Arg1) || StringToBool(Arg2);
							}
						}
						break;

					case "setBoolIsEqual":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "");
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = Arg1.Equals(Arg2);
							}
						}
						break;

					case "setBoolIsLess":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false);
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false);
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = (StringToInt(Arg1, Node) < StringToInt(Arg2, Node));
							}
						}
						break;

					case "setBoolIsLessEqual":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false);
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false);
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = (StringToInt(Arg1, Node) <= StringToInt(Arg2, Node));
							}
						}
						break;

					case "setBoolIsGreater":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false);
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false);
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = (StringToInt(Arg1, Node) > StringToInt(Arg2, Node));
							}
						}
						break;

					case "setBoolIsGreaterEqual":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false);
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false);
							if (Result != null)
							{
								CurrentContext.BoolVariables[Result] = (StringToInt(Arg1, Node) >= StringToInt(Arg2, Node));
							}
						}
						break;

					case "setInt":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false, "0");
							if (Result != null)
							{
								CurrentContext.IntVariables[Result] = StringToInt(Value, Node);
							}
						}
						break;

					case "setIntFrom":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false, "0");
							if (Result != null)
							{
								Value = ExpandVariables(CurrentContext, "$I(" + Value + ")");
								CurrentContext.IntVariables[Result] = StringToInt(Value, Node);
							}
						}
						break;

					case "setIntFromProperty":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Ini = GetAttribute(CurrentContext, Node, "ini");
							string Section = GetAttribute(CurrentContext, Node, "section");
							string Property = GetAttribute(CurrentContext, Node, "property");
							string DefaultVal = GetAttribute(CurrentContext, Node, "default", true, false, "0");
							if (Result != null && Ini != null && Section != null && Property != null)
							{
								int Value = StringToInt(DefaultVal, Node);

								ConfigCacheIni ConfigIni = GetConfigCacheIni(Ini);
								if (ConfigIni != null)
								{
									ConfigIni.GetInt32(Section, Property, out Value);
								}
								CurrentContext.IntVariables[Result] = Value;
							}
						}
						break;

					case "setIntAdd":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "0");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "0");
							if (Result != null)
							{
								CurrentContext.IntVariables[Result] = StringToInt(Arg1, Node) + StringToInt(Arg2, Node);
							}
						}
						break;

					case "setIntSubtract":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "0");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "0");
							if (Result != null)
							{
								CurrentContext.IntVariables[Result] = StringToInt(Arg1, Node) - StringToInt(Arg2, Node);
							}
						}
						break;

					case "setIntMultiply":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "1");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "1");
							if (Result != null)
							{
								CurrentContext.IntVariables[Result] = StringToInt(Arg1, Node) * StringToInt(Arg2, Node);
							}
						}
						break;

					case "setIntDivide":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "1");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "1");
							if (Result != null)
							{
								int Denominator = StringToInt(Arg2, Node);
								if (Denominator == 0)
								{
									CurrentContext.IntVariables[Result] = StringToInt(Arg1, Node);
								}
								else
								{
									CurrentContext.IntVariables[Result] = StringToInt(Arg1, Node) / Denominator;
								}
							}
						}
						break;

					case "setIntLength":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							if (Result != null)
							{
								CurrentContext.IntVariables[Result] = Source.Length;
							}
						}
						break;

					case "setIntFindString":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							string Find = GetAttribute(CurrentContext, Node, "find", true, false, "");
							if (Result != null)
							{
								CurrentContext.IntVariables[Result] = Source.IndexOf(Find);
							}
						}
						break;

					case "setString":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false, "");
							if (Result != null)
							{
								if (Result == "Output")
								{
									GlobalContext.StringVariables["Output"] = Value;
								}
								else
								{
									CurrentContext.StringVariables[Result] = Value;
								}
							}
						}
						break;

					case "setStringFrom":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false, "0");
							if (Result != null)
							{
								Value = ExpandVariables(CurrentContext, "$S(" + Value + ")");
								CurrentContext.StringVariables[Result] = Value;
							}
						}
						break;

					case "setStringFromTag":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Tag = GetAttribute(CurrentContext, Node, "tag", true, false, "$");
							if (Result != null)
							{
								XElement Element = CurrentElement;
								if (Tag.StartsWith("$"))
								{
									if (Tag.Length > 1)
									{
										if (!CurrentContext.ElementVariables.TryGetValue(Tag.Substring(1), out Element))
										{
											if (!GlobalContext.ElementVariables.TryGetValue(Tag.Substring(1), out Element))
											{
												Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
												continue;
											}
										}
									}
								}
								CurrentContext.StringVariables[Result] = Element.Name.ToString();
							}
						}
						break;

					case "setStringFromAttribute":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Tag = GetAttribute(CurrentContext, Node, "tag");
							string Name = GetAttribute(CurrentContext, Node, "name");
							if (Result != null && Tag != null && Name != null)
							{
								XElement Element = CurrentElement;
								if (Tag.StartsWith("$"))
								{
									if (Tag.Length > 1)
									{
										if (!CurrentContext.ElementVariables.TryGetValue(Tag.Substring(1), out Element))
										{
											if (!GlobalContext.ElementVariables.TryGetValue(Tag.Substring(1), out Element))
											{
												Log.TraceWarning("\nMissing element variable '{0}' in '{1}' (skipping instruction)", Tag, TraceNodeString(Node));
												continue;
											}
										}
									}
								}

								XAttribute Attribute;
								int Index = Name.IndexOf(":");
								if (Index >= 0)
								{
									Name = Name.Substring(Index + 1);
									Attribute = Element.Attribute(AndroidNameSpace + Name);
								}
								else
								{
									Attribute = Element.Attribute(Name);
								}

								CurrentContext.StringVariables[Result] = (Attribute != null) ? Attribute.Value : "";
							}
						}
						break;

					case "setStringFromProperty":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Ini = GetAttribute(CurrentContext, Node, "ini");
							string Section = GetAttribute(CurrentContext, Node, "section");
							string Property = GetAttribute(CurrentContext, Node, "property");
							string DefaultVal = GetAttribute(CurrentContext, Node, "default", true, false, "");
							if (Result != null && Ini != null && Section != null && Property != null)
							{
								string Value = DefaultVal;

								ConfigCacheIni ConfigIni = GetConfigCacheIni(Ini);
								if (ConfigIni != null)
								{
									ConfigIni.GetString(Section, Property, out Value);
								}
								if (Result == "Output")
								{
									GlobalContext.StringVariables["Output"] = Value;
								}
								else
								{
									CurrentContext.StringVariables[Result] = Value;
								}
							}
						}
						break;

					case "setStringAdd":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Arg1 = GetAttribute(CurrentContext, Node, "arg1", true, false, "");
							string Arg2 = GetAttribute(CurrentContext, Node, "arg2", true, false, "");
							if (Result != null)
							{
								string Value = Arg1 + Arg2;
								if (Result == "Output")
								{
									GlobalContext.StringVariables["Output"] = Value;
								}
								else
								{
									CurrentContext.StringVariables[Result] = Value;
								}
							}
						}
						break;

					case "setStringSubstring":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							string Start = GetAttribute(CurrentContext, Node, "start", true, false, "0");
							string Length = GetAttribute(CurrentContext, Node, "length", true, false, "0");
							if (Result != null && Source != null)
							{
								int Index = StringToInt(Start, Node);
								int Count = StringToInt(Length, Node);
								Index = (Index < 0) ? 0 : (Index > Source.Length) ? Source.Length : Index;
								Count = (Index + Count > Source.Length) ? Source.Length - Index : Count;
								string Value = Source.Substring(Index, Count);
								if (Result == "Output")
								{
									GlobalContext.StringVariables["Output"] = Value;
								}
								else
								{
									CurrentContext.StringVariables[Result] = Value;
								}
							}
						}
						break;

					case "setStringReplace":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Source = GetAttribute(CurrentContext, Node, "source", true, false, "");
							string Find = GetAttribute(CurrentContext, Node, "find");
							string With = GetAttribute(CurrentContext, Node, "with", true, false, "");
							if (Result != null && Find != null)
							{
								string Value = Source.Replace(Find, With);
								if (Result == "Output")
								{
									GlobalContext.StringVariables["Output"] = Value;
								}
								else
								{
									CurrentContext.StringVariables[Result] = Value;
								}
							}
						}
						break;

					case "setElement":
						{
							string Result = GetAttribute(CurrentContext, Node, "result");
							string Value = GetAttribute(CurrentContext, Node, "value", true, false);
							string Parse = GetAttribute(CurrentContext, Node, "xml", true, false);
							if (Result != null)
							{
								if (Value != null)
								{
									CurrentContext.ElementVariables[Result] = new XElement(Value);
								}
								else if (Parse != null)
								{
									try
									{
										CurrentContext.ElementVariables[Result] = XElement.Parse(Parse);
									}
									catch (Exception e)
									{
										Log.TraceError("\nXML parsing {0} failed! {1} (skipping instruction)", Parse, e);
									}
								}
							}
						}
						break;

					default:
						Log.TraceWarning("\nUnknown command: {0}", Node.Name);
						break;
				}
			}

			return GlobalContext.StringVariables["Output"];
		}

		public void Init(List<string> Architectures, bool bDistribution, string EngineDirectory, string BuildDirectory)
		{
			GlobalContext.BoolVariables["Distribution"] = bDistribution;
			GlobalContext.StringVariables["EngineDir"] = EngineDirectory;
			GlobalContext.StringVariables["BuildDir"] = BuildDirectory;

			foreach (string Arch in Architectures)
			{
				Log.TraceInformation("APL Init: {0}", Arch);
				ProcessPluginNode(Arch, "init", "");
			}

			if (bGlobalTrace)
			{
				Log.TraceInformation("\nVariables:\n{0}", DumpVariables());
			}
		}
	}
}

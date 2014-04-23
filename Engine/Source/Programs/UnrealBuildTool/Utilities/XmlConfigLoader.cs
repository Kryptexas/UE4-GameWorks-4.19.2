using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Xml;

namespace UnrealBuildTool
{
	/// <summary>
	/// Loads data from disk XML files and stores it in memory.
	/// </summary>
	public static class XmlConfigLoader
	{
		public static void Load(Type Class)
		{
			XmlConfigLoaderClassData ClassData;

			if(Data.TryGetValue(Class, out ClassData))
			{
				ClassData.LoadXmlData();
			}
		}

		/// <summary>
		/// Initializing data and resets all found classes.
		/// </summary>
		public static void Init()
		{
			LoadData();

			foreach(var ClassData in Data)
			{
				ClassData.Value.ResetData();
			}
		}

		/// <summary>
		/// Cache entry class to store loaded info for given class.
		/// </summary>
		class XmlConfigLoaderClassData
		{
			public XmlConfigLoaderClassData(Type ConfigClass)
			{
				// Adding empty types array to make sure this is parameterless Reset
				// in case of overloading.
				DefaultValuesLoader = ConfigClass.GetMethod("Reset", new Type[] { });
			}

			/// <summary>
			/// Resets previously stored data into class.
			/// </summary>
			public void ResetData()
			{
				bDoneLoading = false;

				if(DefaultValuesLoader != null)
				{
					DefaultValuesLoader.Invoke(null, new object[] { });
				}

				if (!bDoneLoading)
				{
					LoadXmlData();
				}
			}

			/// <summary>
			/// Loads previously stored data into class.
			/// </summary>
			public void LoadXmlData()
			{
				foreach (var DataPair in DataMap)
				{
					DataPair.Key.SetValue(null, DataPair.Value);
				}

				bDoneLoading = true;
			}

			/// <summary>
			/// Adds or overrides value in the cache.
			/// </summary>
			/// <param name="Field">The field info of the class.</param>
			/// <param name="Value">The value to store.</param>
			public void SetValue(FieldInfo Field, object Value)
			{
				if(DataMap.ContainsKey(Field))
				{
					DataMap[Field] = Value;
				}
				else
				{
					DataMap.Add(Field, Value);
				}
			}

			// A variable to indicate if loading was done during invoking of
			// default values loader.
			bool bDoneLoading = false;

			// Method info loader to invoke before overriding fields with XML files data.
			MethodInfo DefaultValuesLoader = null;

			// Loaded data map.
			Dictionary<FieldInfo, object> DataMap = new Dictionary<FieldInfo, object>();
		}

		/// <summary>
		/// Loads BuildConfiguration from XML into memory.
		/// </summary>
		private static void LoadData()
		{
			var ConfigXmlFileName = "BuildConfiguration.xml";
			var UE4EnginePath = new FileInfo(Path.Combine(Utils.GetExecutingAssemblyDirectory(), "..", "..")).FullName;

			/*
			 *	There are four possible location for this file:
			 *		a. UE4/Engine/Programs/UnrealBuildTool
			 *		b. UE4/Engine/Programs/NoRedist/UnrealBuildTool
			 *		c. UE4/Engine/Saved/UnrealBuildTool
			 *		d. My Documnets/Unreal Engine/UnrealBuildTool
			 *
			 *	The UBT is looking for it in all four places in the given order and
			 *	overrides already read data with the loaded ones, hence d. has the
			 *	priority. Not defined classes and fields are left alone.
			 */

			var ConfigLocationHierarchy = new string[]
			{
				Path.Combine(UE4EnginePath, "Programs", "UnrealBuildTool"),
				Path.Combine(UE4EnginePath, "Programs", "NoRedist", "UnrealBuildTool"),
				Path.Combine(UE4EnginePath, "Saved", "UnrealBuildTool"),
				Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal), "Unreal Engine", "UnrealBuildTool")
			};

			foreach (var PossibleConfigLocation in ConfigLocationHierarchy)
			{
				var FilePath = Path.Combine(PossibleConfigLocation, ConfigXmlFileName);

				if (File.Exists(FilePath))
				{
					Load(FilePath);
				}
			}
		}

		/// <summary>
		/// Sets values of this class with values from given XML file.
		/// </summary>
		/// <param name="ConfigurationXmlPath">The path to the file with values.</param>
		private static void Load(string ConfigurationXmlPath)
		{
			var ConfigDocument = new XmlDocument();
			ConfigDocument.Load(ConfigurationXmlPath);

			var XmlClasses = ConfigDocument.DocumentElement.SelectNodes("/Configuration/*");

			foreach (XmlNode XmlClass in XmlClasses)
			{
				var ClassType = Type.GetType("UnrealBuildTool." + XmlClass.Name);

				if(ClassType == null)
				{
					Log.TraceVerbose("XmlConfig Loading: class '{0}' doesn't exist.", XmlClass.Name);
					continue;
				}

				XmlConfigLoaderClassData ClassData;
				if (!Data.TryGetValue(ClassType, out ClassData))
				{
					ClassData = new XmlConfigLoaderClassData(ClassType);
					Data.Add(ClassType, ClassData);
				}

				var XmlFields = XmlClass.SelectNodes("*");

				foreach (XmlNode XmlField in XmlFields)
				{
					FieldInfo Field = ClassType.GetField(XmlField.Name);

					if (Field == null || !Field.IsPublic || !Field.IsStatic)
					{
						throw new BuildException("BuildConfiguration Loading: field '{0}' doesn't exist or is either non-public or non-static.", XmlField.Name);
					}

					if(Field.FieldType.IsArray)
					{
						// If the type is an array type get items for it.
						var XmlItems = XmlField.SelectNodes("Item");

						// Get the C# type of the array.
						var ItemType = Field.FieldType.GetElementType();

						// Create the array according to the ItemType.
						var OutputArray = Array.CreateInstance(ItemType, XmlItems.Count);

						int Id = 0;
						foreach(XmlNode XmlItem in XmlItems)
						{
							// Append values to the OutputArray.
							OutputArray.SetValue(ParseFieldData(ItemType, XmlItem.InnerText), Id++);
						}

						ClassData.SetValue(Field, OutputArray);
					}
					else
					{
						ClassData.SetValue(Field, ParseFieldData(Field.FieldType, XmlField.InnerText));
					}
				}
			}
		}

		private static object ParseFieldData(Type FieldType, string Text)
		{
			if (FieldType.Equals(typeof(System.String)))
			{
				return Text;
			}
			else
			{
				// Declaring parameters array used by TryParse method.
				// Second parameter is "out", so you have to just
				// assign placeholder null to it.
				object ParsedValue;
				if (!TryParse(FieldType, Text, out ParsedValue))
				{
					throw new BuildException("BuildConfiguration Loading: Parsing {0} value from \"{1}\" failed.", FieldType.Name, Text);
				}

				// If Invoke returned true, the second object of the
				// parameters array is set to the parsed value.
				return ParsedValue;
			}
		}

		/// <summary>
		/// Emulates TryParse behavior on custom type. If the type implements
		/// Parse(string, IFormatProvider) or Parse(string) static method uses
		/// one of them to parse with preference of the one with format
		/// provider (but passes invariant culture).
		/// </summary>
		/// <param name="ParsingType">Type to parse.</param>
		/// <param name="UnparsedValue">String representation of the value.</param>
		/// <param name="ParsedValue">Output parsed value.</param>
		/// <returns>True if parsing succeeded. False otherwise.</returns>
		private static bool TryParse(Type ParsingType, string UnparsedValue, out object ParsedValue)
		{
			// Getting Parse method for FieldType which is required,
			// if it doesn't exists for complex type, author should add
			// one. The signature should be one of:
			//     static T Parse(string Input, IFormatProvider Provider) or
			//     static T Parse(string Input)
			// where T is containing type.
			// The one with format provider is preferred and invoked with
			// InvariantCulture.

			bool bWithCulture = true;
			var ParseMethod = ParsingType.GetMethod("Parse", new Type[] { typeof(System.String), typeof(IFormatProvider) });

			if(ParseMethod == null)
			{
				ParseMethod = ParsingType.GetMethod("Parse", new Type[] { typeof(System.String) });
				bWithCulture = false;
			}

			if (ParseMethod == null)
			{
				throw new BuildException("BuildConfiguration Loading: Parsing of the type {0} is not supported.", ParsingType.Name);
			}

			var ParametersList = new List<object> { UnparsedValue };

			if(bWithCulture)
			{
				ParametersList.Add(CultureInfo.InvariantCulture);
			}

			try
			{
				ParsedValue = ParseMethod.Invoke(null, ParametersList.ToArray());
			}
			catch(Exception e)
			{
				if(e is TargetInvocationException &&
					(
						e.InnerException is ArgumentNullException ||
						e.InnerException is FormatException ||
						e.InnerException is OverflowException
					)
				)
				{
					ParsedValue = null;
					return false;
				}

				throw;
			}

			return true;
		}

		// Map to store class data in.
		private static readonly Dictionary<Type, XmlConfigLoaderClassData> Data = new Dictionary<Type, XmlConfigLoaderClassData>();
	}
}

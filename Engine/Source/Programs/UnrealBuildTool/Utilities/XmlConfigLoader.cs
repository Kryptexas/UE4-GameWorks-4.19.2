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
		/// Class that stores information about possible BuildConfiguration.xml
		/// location and its name that will be displayed in IDE.
		/// </summary>
		public class XmlConfigLocation
		{
			/// <summary>
			/// Returns location of the BuildConfiguration.xml.
			/// </summary>
			/// <returns>Location of the BuildConfiguration.xml.</returns>
			private static string GetConfigLocation(IEnumerable<string> PossibleLocations, out bool bExists)
			{
				if(PossibleLocations.Count() == 0)
				{
					throw new ArgumentException("Empty possible locations", "PossibleLocations");
				}

				const string ConfigXmlFileName = "BuildConfiguration.xml";

				// Filter out non-existing
				var ExistingLocations = new List<string>();

				foreach(var PossibleLocation in PossibleLocations)
				{
					var FilePath = Path.Combine(PossibleLocation, ConfigXmlFileName);

					if(File.Exists(FilePath))
					{
						ExistingLocations.Add(FilePath);
					}
				}

				if(ExistingLocations.Count == 0)
				{
					bExists = false;
					return Path.Combine(PossibleLocations.First(), ConfigXmlFileName);
				}

				bExists = true;

				if(ExistingLocations.Count == 1)
				{
					return ExistingLocations.First();
				}

				// Choose most recently used from existing.
				return ExistingLocations.OrderBy(Location => File.GetLastWriteTime(Location)).Last();
			}

			// Possible location of the config file in the file system.
			public string FSLocation { get; private set; }

			// IDE folder name that will contain this location if file will be found.
			public string IDEFolderName { get; private set; }

			// Tells if UBT has to create a template config file if it does not exist in the location.
			public bool bCreateIfDoesNotExist { get; private set; }

			// Tells if config file exists in this location.
			public bool bExists { get; set; }

			public XmlConfigLocation(string[] FSLocations, string IDEFolderName, bool bCreateIfDoesNotExist = false)
			{
				bool bExists;

				this.FSLocation = GetConfigLocation(FSLocations, out bExists);
				this.IDEFolderName = IDEFolderName;
				this.bCreateIfDoesNotExist = bCreateIfDoesNotExist;
				this.bExists = bExists;
			}

			public XmlConfigLocation(string FSLocation, string IDEFolderName, bool bCreateIfDoesNotExist = false)
				: this(new string[] { FSLocation }, IDEFolderName, bCreateIfDoesNotExist)
			{
			}
		}

		public static readonly XmlConfigLocation[] ConfigLocationHierarchy;

		static XmlConfigLoader()
		{
			/*
			 *	There are four possible location for this file:
			 *		a. UE4/Engine/Programs/UnrealBuildTool
			 *		b. UE4/Engine/Programs/NotForLicensees/UnrealBuildTool
			 *		c. UE4/Engine/Saved/UnrealBuildTool
			 *		d. <AppData or My Documnets>/Unreal Engine/UnrealBuildTool -- the location is
			 *		   chosen by existence and if both exist most recently used.
			 *
			 *	The UBT is looking for it in all four places in the given order and
			 *	overrides already read data with the loaded ones, hence d. has the
			 *	priority. Not defined classes and fields are left alone.
			 */

			var UE4EnginePath = new FileInfo(Path.Combine(Utils.GetExecutingAssemblyDirectory(), "..", "..")).FullName;

			ConfigLocationHierarchy = new XmlConfigLocation[]
			{
				new XmlConfigLocation(Path.Combine(UE4EnginePath, "Programs", "UnrealBuildTool"), "Default"),
				new XmlConfigLocation(Path.Combine(UE4EnginePath, "Programs", "NotForLicensees", "UnrealBuildTool"), "NotForLicensees"),
				new XmlConfigLocation(Path.Combine(UE4EnginePath, "Saved", "UnrealBuildTool"), "User", true),
				new XmlConfigLocation(new string[] {
					Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Unreal Engine", "UnrealBuildTool"),
					Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal), "Unreal Engine", "UnrealBuildTool")
				}, "Global", true)
			};
		}

		/// <summary>
		/// Loads BuildConfiguration from XML into memory.
		/// </summary>
		private static void LoadData()
		{
			foreach (var PossibleConfigLocation in ConfigLocationHierarchy)
			{
				if(!PossibleConfigLocation.bExists)
				{
					continue;
				}

				Load(PossibleConfigLocation.FSLocation);
			}

			foreach (var PossibleConfigLocation in ConfigLocationHierarchy)
			{
				if(!PossibleConfigLocation.bCreateIfDoesNotExist || PossibleConfigLocation.bExists)
				{
					continue;
				}

				CreateUserXmlConfigTemplate(PossibleConfigLocation.FSLocation);
				PossibleConfigLocation.bExists = true;
			}
		}


		/// <summary>
		/// Creates template file in one of the user locations if it not
		/// </summary>
		/// <param name="Location">Location of the xml config file to create.</param>
		private static void CreateUserXmlConfigTemplate(string Location)
		{
			try
			{
				Directory.CreateDirectory(Path.GetDirectoryName(Location));

				var FilePath = Path.Combine(Location);

				const string TemplateContent =
					"<Configuration>\n" +
					"	<BuildConfiguration>\n" +
					"	</BuildConfiguration>\n" +
					"</Configuration>\n";

				File.WriteAllText(FilePath, TemplateContent);
			}
			catch (Exception)
			{
				// Ignore quietly.
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

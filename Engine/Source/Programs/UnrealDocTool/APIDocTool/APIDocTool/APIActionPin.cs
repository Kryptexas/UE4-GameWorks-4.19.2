using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace APIDocTool
{
	class APIActionPin : APIPage
	{
		public readonly string Tooltip;
		public readonly string TypeText;
		public readonly string DefaultValue;

		public readonly string PinCategory;
		public readonly string PinSubCategory;
		public readonly string PinSubCategoryObject;

		public readonly bool bInputPin;
		public readonly bool bIsConst;
		public readonly bool bIsReference;
		public readonly bool bIsArray;
		public readonly bool bShowAssetPicker;
		public readonly bool bShowPinLabel;

		public static string GetPinName(Dictionary<string, object> PinProperties)
		{
			object Value;
			string Name;

			if (PinProperties.TryGetValue("Name", out Value))
			{
				Name = (string)Value;
			}
			else
			{
				Debug.Assert((string)PinProperties["PinCategory"] == "exec");
				
				Name = ((string)PinProperties["Direction"] == "input" ? "In" : "Out");
			}

			return Name;
		}

		public APIActionPin(APIPage InParent, string InName, Dictionary<string, object> PinProperties)
			: base(InParent, InName)
		{
			object Value;

			Debug.Assert(InName == GetPinName(PinProperties));

			// We have to check this again to set the show label property
			bShowPinLabel = PinProperties.TryGetValue("Name", out Value);

			bInputPin = (string)PinProperties["Direction"] == "input";
			PinCategory = (string)PinProperties["PinCategory"];
			TypeText = (string)PinProperties["TypeText"];

			if (PinProperties.TryGetValue("Tooltip", out Value))
			{
				Tooltip = String.Concat(((string)Value).Split('\n').Select(Line => Line.Trim() + '\n'));
			}
			else
			{
				Tooltip = "";
			}

			if (PinProperties.TryGetValue("PinSubCategory", out Value))
			{
				PinSubCategory = (string)Value;
			}
			else
			{
				PinSubCategory = "";
			}

			if (PinProperties.TryGetValue("PinSubCategoryObject", out Value))
			{
				PinSubCategoryObject = (string)Value;
			}
			else
			{
				PinSubCategoryObject = "";
			}
			
			if (PinProperties.TryGetValue("DefaultValue", out Value))
			{
				DefaultValue = (string)Value;
			}
			else
			{
				DefaultValue = "";
			}
			if (PinProperties.TryGetValue("IsConst", out Value))
			{
				bIsConst = Convert.ToBoolean((string)Value);
			}

			if (PinProperties.TryGetValue("IsReference", out Value))
			{
				bIsReference = Convert.ToBoolean((string)Value);
			}

			if (PinProperties.TryGetValue("IsArray", out Value))
			{
				bIsArray = Convert.ToBoolean((string)Value);
			}

			if (PinProperties.TryGetValue("ShowAssetPicker", out Value))
			{
				bShowAssetPicker = Convert.ToBoolean((string)Value);
			}
			else
			{
				bShowAssetPicker = true;
			}
		}

		private string GetPinCategory()
		{
			if (PinCategory == "struct")
			{
				if (PinSubCategoryObject == "Vector")
				{
					return "vector";
				}
				else if (PinSubCategoryObject == "Transform")
				{
					return "transform";
				}
				else if (PinSubCategoryObject == "Rotator")
				{
					return "rotator";
				}
			}
			else if (PinCategory == "bool")
			{
				return "boolean";
			}
			else if (PinCategory == "int")
			{
				return "integer";
			}
			else if (PinCategory == "byte")
			{
				if (PinSubCategoryObject != "")
				{
					return "enum";
				}
			}

			return PinCategory;
		}

		private string GetId()
		{
			return Name.Replace(' ', '_');
		}

		public void WriteObject(UdnWriter Writer, bool bDrawLabels)
		{
			string PinCategory = GetPinCategory();

			Writer.EnterObject("BlueprintPin");
			Writer.WriteParamLiteral("type", PinCategory + (bIsArray ? " array" : ""));
			Writer.WriteParamLiteral("id", GetId());

			if (bShowPinLabel && bDrawLabels)
			{
				Writer.WriteParamLiteral("title",  Name);
			}

			var DefaultValueElements = DefaultValue.Split(',');
			switch(PinCategory)
			{
				case "byte":
					Writer.WriteParamLiteral("value", (DefaultValueElements.Length > 0 ? DefaultValueElements[0] : "0"));
					break;

				case "class":
					if (bShowAssetPicker)
					{
						Writer.WriteParamLiteral("value", "Select Class");
					}
					break;

				case "float":
					Writer.WriteParamLiteral("value", (DefaultValueElements.Length > 0 ? DefaultValueElements[0] : "0.0"));
					break;

				case "integer":
					Writer.WriteParamLiteral("value", (DefaultValueElements.Length > 0 ? DefaultValueElements[0] : "0"));
					break;

				case "object":
					if (bShowAssetPicker)
					{
						Writer.WriteParamLiteral("value", "Select Asset");
					}
					break;

				case "rotator":
					Writer.WriteParamLiteral("yaw", (DefaultValueElements.Length > 0 ? DefaultValueElements[0] : "0.0"));
					Writer.WriteParamLiteral("pitch", (DefaultValueElements.Length > 1 ? DefaultValueElements[1] : "0.0"));
					Writer.WriteParamLiteral("roll", (DefaultValueElements.Length > 2 ? DefaultValueElements[2] : "0.0"));
					break;

				case "struct":
					if (PinSubCategoryObject == "Vector2D")
					{
						Writer.WriteParamLiteral("x", (DefaultValueElements.Length > 0 ? DefaultValueElements[0] : "0.0"));
						Writer.WriteParamLiteral("y", (DefaultValueElements.Length > 1 ? DefaultValueElements[1] : "0.0"));
					}
					break;

				case "vector":
					Writer.WriteParamLiteral("x",(DefaultValueElements.Length > 0 ? DefaultValueElements[0] : "0.0"));
					Writer.WriteParamLiteral("y",(DefaultValueElements.Length > 1 ? DefaultValueElements[1] : "0.0"));
					Writer.WriteParamLiteral("z",(DefaultValueElements.Length > 2 ? DefaultValueElements[2] : "0.0"));
					break;

				default:
					Writer.WriteParamLiteral("value", DefaultValue);
					break;
			}

			Writer.LeaveObject();
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, Name);
			}
		}

		public override void GatherReferencedPages(List<APIPage> Pages)
		{

		}

		public override void AddToManifest(UdnManifest Manifest)
		{
			Manifest.Add(Name, this);
		}

		public void WritePin(UdnWriter Writer)
		{
			// Get all the icons
			List<Icon> ItemIcons = new List<Icon>();
			if (bIsArray)
			{
				ItemIcons.Add(Icons.ArrayVariablePinIcons[GetPinCategory()]);
			}
			else
			{
				ItemIcons.Add(Icons.VariablePinIcons[GetPinCategory()]);
			}

			Writer.EnterObject("ActionPinListItem");
			Writer.WriteParam("icons", ItemIcons);
			Writer.WriteParamLiteral("name", Name);
			Writer.WriteParamLiteral("type", TypeText);
			Writer.WriteParam("tooltip", Tooltip);
			Writer.WriteParamLiteral("id", GetId());
			Writer.LeaveObject();
		}
	}
}

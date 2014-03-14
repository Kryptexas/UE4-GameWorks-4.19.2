/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization;
using System.Windows.Forms.DataVisualization.Charting;

namespace MemoryProfiler2
{
	public static class FTimeLineChartView
	{
		/// <summary> Reference to the main memory profiler window. </summary>
		private static MainWindow OwnerWindow;

		public static void SetProfilerWindow( MainWindow InMainWindow )
		{
			OwnerWindow = InMainWindow;
		}

		public static void ClearView()
		{
			for( int SerieIndex = 0; SerieIndex < OwnerWindow.TimeLineChart.Series.Count; SerieIndex++ )
			{
				Series ChartSeries = OwnerWindow.TimeLineChart.Series[ SerieIndex ];
				if( ChartSeries != null )
				{
					ChartSeries.Points.Clear();
				}
			}
		}

		private static void AddSeries( Chart TimeLineChart, string SeriesName, FStreamSnapshot Snapshot, ESliceTypes SliceType, bool bVisible )
		{
			Series ChartSeries = TimeLineChart.Series[ SeriesName ];
			ChartSeries.Enabled = bVisible;

			if( ChartSeries != null )
			{
				ChartSeries.Points.Clear();

				foreach( FMemorySlice Slice in Snapshot.OverallMemorySlice )
				{
					DataPoint Point = new DataPoint();
					Point.YValues[ 0 ] = Slice.GetSliceInfo( SliceType ) / ( 1024.0 * 1024.0 );

					ChartSeries.Points.Add( Point );
				}
			}
		}

		public static void ParseSnapshot( Chart TimeLineChart, FStreamSnapshot Snapshot )
		{
			// Progress bar.
			OwnerWindow.UpdateStatus( "Updating time line view for " + OwnerWindow.CurrentFilename );

			TimeLineChart.Annotations.Clear();

			bool bIsXbox360 = FStreamInfo.GlobalInstance.Platform == EPlatformType.Xbox360;
			bool bIsPS3 = FStreamInfo.GlobalInstance.Platform == EPlatformType.PS3;

			AddSeries( TimeLineChart, "Allocated Memory", Snapshot, ESliceTypes.TotalUsed, true );

			AddSeries( TimeLineChart, "Image Size", Snapshot, ESliceTypes.ImageSize, bIsXbox360 || bIsPS3 );
			AddSeries( TimeLineChart, "OS Overhead", Snapshot, ESliceTypes.OSOverhead, bIsXbox360 );

			AddSeries( TimeLineChart, "Virtual Used", Snapshot, ESliceTypes.CPUUsed, true );
			AddSeries( TimeLineChart, "Virtual Slack", Snapshot, ESliceTypes.CPUSlack, true );
			AddSeries( TimeLineChart, "Virtual Waste", Snapshot, ESliceTypes.CPUWaste, true );
			AddSeries( TimeLineChart, "Physical Used", Snapshot, ESliceTypes.GPUUsed, bIsPS3 || bIsXbox360 );
			AddSeries( TimeLineChart, "Physical Slack", Snapshot, ESliceTypes.GPUSlack, bIsXbox360 );
			AddSeries( TimeLineChart, "Physical Waste", Snapshot, ESliceTypes.GPUWaste, bIsPS3 || bIsXbox360 );

			AddSeries( TimeLineChart, "Host Used", Snapshot, ESliceTypes.HostUsed, bIsPS3 );
			AddSeries( TimeLineChart, "Host Slack", Snapshot, ESliceTypes.HostSlack, bIsPS3 );
			AddSeries( TimeLineChart, "Host Waste", Snapshot, ESliceTypes.HostWaste, bIsPS3 );
		}

		public static bool AddCustomSnapshot( Chart TimeLineChart, CursorEventArgs Event )
		{
			if( TimeLineChart.Series["Allocated Memory"].Points.Count == 0 )
			{
				return false;
			}
			else 
			{
				VerticalLineAnnotation A = new VerticalLineAnnotation();
				A.AxisX = Event.Axis;
				A.AnchorX = ( int )Event.NewSelectionStart + 1;
				A.ToolTip = "Snapshot";
				A.IsInfinitive = true;
				TimeLineChart.Annotations.Add( A );
				return true;
			}
		}
	}
}


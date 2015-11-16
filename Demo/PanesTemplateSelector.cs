/************************************************************************

   AvalonDock

   Copyright (C) 2007-2013 Xceed Software Inc.

   This program is provided to you under the terms of the New BSD
   License (BSD) as published at http://avalondock.codeplex.com/license 

   For more features, controls, and fast professional support,
   pick up AvalonDock in Extended WPF Toolkit Plus at http://xceed.com/wpf_toolkit

   Stay informed: follow @datagrid on Twitter or Like facebook.com/datagrids

  **********************************************************************/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using System.Windows;
using Xceed.Wpf.AvalonDock.Layout;

using WpfRbTest2.ToolViewModels;

namespace WpfRbTest2
{
    class PanesTemplateSelector : DataTemplateSelector
    {
        public PanesTemplateSelector()
        {

        }


        public DataTemplate ExecutionHistoryViewTemplate
        {
            get;
            set;
        }

        public DataTemplate MemoryViewTemplate
        {
            get;
            set;
        }
        public DataTemplate MemoryMapViewTemplate
        {
            get;
            set;
        }
        public DataTemplate ModulesViewTemplate
        {
            get;
            set;
        }
        public DataTemplate RegistersViewTemplate
        {
            get;
            set;
        }
        public DataTemplate RiverStackViewTemplate
        {
            get;
            set;
        }
        public DataTemplate TrackStackViewTemplate
        {
            get;
            set;
        }
        public DataTemplate WaypointsViewTemplate
        {
            get;
            set;
        }

        public override System.Windows.DataTemplate SelectTemplate(object item, System.Windows.DependencyObject container)
        {
            var itemAsLayoutContent = item as LayoutContent;

            if (item is ExecutionHistoryViewModel)
            {
                return ExecutionHistoryViewTemplate;
            }

            if (item is MemoryViewModel)
            {
                return MemoryViewTemplate;
            }

            if (item is MemoryMapViewModel)
            {
                return MemoryMapViewTemplate;
            }

            if (item is ModulesViewModel)
            {
                return ModulesViewTemplate;
            }

            if (item is RegistersViewModel)
            {
                return RegistersViewTemplate;
            }

            if (item is RiverStackViewModel)
            {
                return RiverStackViewTemplate;
            }

            if (item is TrackStackViewModel)
            {
                return TrackStackViewTemplate;
            }

            if (item is WaypointsViewModel)
            {
                return WaypointsViewTemplate;
            }

            return base.SelectTemplate(item, container);
        }
    }
}

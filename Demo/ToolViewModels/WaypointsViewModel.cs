using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class WaypointsViewModel : ToolViewModel
    {
        public WaypointsViewModel()
            : base("Waypoints")
        {
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "WaypointsTool";
    }
}

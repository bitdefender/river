using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class TrackStackViewModel : ToolViewModel
    {
        public TrackStackViewModel()
            : base("Track Stack")
        {
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "TrackStackTool";
    }
}

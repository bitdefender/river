using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class RiverStackViewModel : ToolViewModel
    {
        public RiverStackViewModel()
            : base("River Stack")
        {
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "RiverStackTool";
    }
}

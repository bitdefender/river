using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class MemoryMapViewModel : ToolViewModel
    {
        public MemoryMapViewModel()
            : base("Memory Map")
        {
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "MemoryMapTool";
    }
}

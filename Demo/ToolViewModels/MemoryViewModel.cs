using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class MemoryViewModel : ToolViewModel
    {
        public MemoryViewModel(string idx)
            : base("Memory " + idx)
        {
            ContentId = ToolContentId + idx;
        }

        public const string ToolContentId = "MemoryTool";
    }
}

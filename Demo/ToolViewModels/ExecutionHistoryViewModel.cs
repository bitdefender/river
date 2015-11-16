using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class ExecutionHistoryViewModel : ToolViewModel
    {
        public ExecutionHistoryViewModel()
            : base("Execution History")
        {
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "ExecutionHistoryTool";
    }
}

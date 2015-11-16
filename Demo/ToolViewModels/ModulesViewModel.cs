using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class ModulesViewModel : ToolViewModel
    {
        public ModulesViewModel()
            : base("Modules")
        {
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "ModulesTool";
    }
}

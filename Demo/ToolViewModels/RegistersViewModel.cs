using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfRbTest2.ToolViewModels
{
    class RegistersViewModel : ToolViewModel
    {
        public RegistersViewModel()
            : base("Registers")
        {
            Workspace.This.ExecutionStateChanged += new EventHandler(OnExecutionStateChanged);
            ContentId = ToolContentId;
        }

        public const string ToolContentId = "RegistersTool";

        void OnExecutionStateChanged(object sender, EventArgs e)
        {
            
        }

        #region Registers
        private long _eax, _ecx, _edx, _ebx, _esp, _ebp, _esi, _edi;
        private bool _max, _mcx, _mdx, _mbx, _msp, _mbp, _msi, _mdi;
        public long EAX
        {
            get { return _eax; }
            set
            {
                _max = (_eax != value);
                if (_max)
                {
                    _eax = value;
                    RaisePropertyChanged("EAX");
                }
            }
        }

        public bool ModifiedEAX
        {
            get { return _max; }
        }

        //==============================================================

        public long ECX
        {
            get { return _ecx; }
            set
            {
                _mcx = (_ecx != value);
                if (_mcx)
                {
                    _ecx = value;
                    RaisePropertyChanged("ECX");
                }
            }
        }

        public bool ModifiedECX
        {
            get { return _mcx; }
        }

        //==============================================================

        public long EDX
        {
            get { return _edx; }
            set
            {
                _mdx = (_edx != value);
                if (_mdx)
                {
                    _edx = value;
                    RaisePropertyChanged("EDX");
                }
            }
        }

        public bool ModifiedEDX
        {
            get { return _max; }
        }

        //==============================================================

        public long EBX
        {
            get { return _ebx; }
            set
            {
                _mbx = (_ebx != value);
                if (_mbx)
                {
                    _ebx = value;
                    RaisePropertyChanged("EBX");
                }
            }
        }

        public bool ModifiedEBX
        {
            get { return _mbx; }
        }

        //==============================================================

        public long ESP
        {
            get { return _esp; }
            set
            {
                _msp = (_esp != value);
                if (_msp)
                {
                    _esp = value;
                    RaisePropertyChanged("ESP");
                }
            }
        }

        public bool ModifiedESP
        {
            get { return _msp; }
        }

        //==============================================================

        public long EBP
        {
            get { return _ebp; }
            set
            {
                _mbp = (_ebp != value);
                if (_mbp)
                {
                    _ebp = value;
                    RaisePropertyChanged("EBP");
                }
            }
        }

        public bool ModifiedEBP
        {
            get { return _mbp; }
        }

        //==============================================================

        public long ESI
        {
            get { return _esi; }
            set
            {
                _msi = (_esi != value);
                if (_msi)
                {
                    _esi = value;
                    RaisePropertyChanged("ESI");
                }
            }
        }

        public bool ModifiedESI
        {
            get { return _msi; }
        }

        //==============================================================

        public long EDI
        {
            get { return _edi; }
            set
            {
                _mdi = (_edi != value);
                if (_mdi)
                {
                    _edi = value;
                    RaisePropertyChanged("EDI");
                }
            }
        }

        public bool ModifiedEDI
        {
            get { return _mdi; }
        }

        #endregion
    }
}

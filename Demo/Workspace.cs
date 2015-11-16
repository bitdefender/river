using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using System.Windows.Input;
using System.Collections.ObjectModel;

using Microsoft.Win32;

using WpfRbTest2.ToolViewModels;
using ExecutionWrapper;



namespace WpfRbTest2
{
    class Workspace : INotifyPropertyChanged {
        private Execution executionContext;

        protected virtual void RaisePropertyChanged(string propertyName) {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected Workspace() {
            executionContext = new Execution();
        }

        ~Workspace() {
            executionContext.Dispose();
        }

        static Workspace _this = new Workspace();

        public static Workspace This {
            get { 
                return _this; 
            }
        }


        ToolViewModel [] _tools = null;

        public IEnumerable<INotifyPropertyChanged> Tools
        {
            get
            {
                if (_tools == null)
                    _tools = new ToolViewModel[] { 
                        ExecutionHistory,
                        Memory,
                        MemoryMap,
                        Modules,
                        Registers,
                        RiverStack,
                        TrackStack,
                        Waypoints
                    };
                return _tools;
            }
        }

        //============================================================================

        ExecutionHistoryViewModel _executionHistory = null;
        public ExecutionHistoryViewModel ExecutionHistory
        {
            get
            {
                if (_executionHistory == null)
                    _executionHistory = new ExecutionHistoryViewModel();

                return _executionHistory;
            }
        }

        //============================================================================

        MemoryViewModel _memory = null;
        public MemoryViewModel Memory
        {
            get
            {
                if (_memory == null)
                    _memory = new MemoryViewModel("1");

                return _memory;
            }
        }

        //============================================================================

        MemoryMapViewModel _memorymap = null;
        public MemoryMapViewModel MemoryMap
        {
            get
            {
                if (_memorymap == null)
                    _memorymap = new MemoryMapViewModel();

                return _memorymap;
            }
        }


        //============================================================================

        ModulesViewModel _modules = null;
        public ModulesViewModel Modules
        {
            get
            {
                if (_modules == null)
                    _modules = new ModulesViewModel();

                return _modules;
            }
        }

        //============================================================================

        RegistersViewModel _registers = null;
        public RegistersViewModel Registers
        {
            get
            {
                if (_registers == null)
                    _registers = new RegistersViewModel();

                return _registers;
            }
        }

        //============================================================================

        RiverStackViewModel _riverstack = null;
        public RiverStackViewModel RiverStack
        {
            get
            {
                if (_riverstack == null)
                    _riverstack = new RiverStackViewModel();

                return _riverstack;
            }
        }

        //============================================================================

        TrackStackViewModel _trackstack = null;
        public TrackStackViewModel TrackStack
        {
            get
            {
                if (_trackstack == null)
                    _trackstack = new TrackStackViewModel();

                return _trackstack;
            }
        }

        //============================================================================

        WaypointsViewModel _waypoints = null;
        public WaypointsViewModel Waypoints
        {
            get
            {
                if (_waypoints == null)
                    _waypoints = new WaypointsViewModel();

                return _waypoints;
            }
        }

        //============================================================================

        private ObservableCollection<VirtualMemorySection> _virtualMemorySections;
        public bool UpdateVirtualMemorySections()
        {
            if (null == _virtualMemorySections)
            {
                _virtualMemorySections = new ObservableCollection<VirtualMemorySection>();
            }
            executionContext.GetProcessVirtualMemory(_virtualMemorySections);
            RaisePropertyChanged("VirtualMemorySections");
            return true;
        }

        public ObservableCollection<VirtualMemorySection> VirtualMemorySections
        {
            get
            {
                if (null == _virtualMemorySections)
                {
                    _virtualMemorySections = new ObservableCollection<VirtualMemorySection>();
                }
                return _virtualMemorySections;
            }
        }

        String _selectedFile = "";
        public String SelectedFile
        {
            get
            {
                return _selectedFile;
            }

            set
            {
                if (_selectedFile != value)
                {
                    _selectedFile = value;
                    RaisePropertyChanged("SelectedFile");
                }
                
            }
        }


        #region ExecutionState

        /*private FileViewModel _activeDocument = null;
        public FileViewModel ActiveDocument
        {
            get { return _activeDocument; }
            set
            {
                if (_activeDocument != value)
                {
                    _activeDocument = value;
                    RaisePropertyChanged("ActiveDocument");
                    if (ActiveDocumentChanged != null)
                        ActiveDocumentChanged(this, EventArgs.Empty);
                }
            }
        }*/

        public event EventHandler ExecutionStateChanged;

        #endregion


        #region ExecuteCommand
        RelayCommand _executeCommand = null;
        public ICommand ExecuteCommand
        {
            get
            {
                if (_executeCommand == null)
                {
                    _executeCommand = new RelayCommand((p) => OnExecute(p), (p) => CanExecute(p));
                }

                return _executeCommand;
            }
        }

        private bool CanExecute(object parameter) {
            return 
                (executionContext.GetState() == ExecutionState.INITIALIZED) ||
                (executionContext.GetState() == ExecutionState.TERMINATED);
            //return true;
        }

        private void OnExecute(object parameter)
        {
            //executionContext.SetPath("D:\\test\\a.exe");
            var ret = executionContext.Execute();

            UpdateVirtualMemorySections();

            RaisePropertyChanged("ExecutionState");
            if (null != ExecutionStateChanged) {
                ExecutionStateChanged(this, EventArgs.Empty);
            }
        }

        #endregion 

        #region TerminateCommand
        RelayCommand _terminateCommand = null;
        public ICommand TerminateCommand
        {
            get
            {
                if (_terminateCommand == null)
                {
                    _terminateCommand = new RelayCommand((p) => OnTerminate(p), (p) => CanTerminate(p));
                }

                return _terminateCommand;
            }
        }

        private bool CanTerminate(object parameter)
        {
            return executionContext.GetState() == ExecutionState.RUNNING;
            //return true;
        }

        private void OnTerminate(object parameter)
        {
            //executionContext.SetPath("D:\\test\\a.exe");
            var ret = executionContext.Terminate();

            UpdateVirtualMemorySections();

            RaisePropertyChanged("ExecutionState");
            if (null != ExecutionStateChanged)
            {
                ExecutionStateChanged(this, EventArgs.Empty);
            }
        }

        #endregion

        #region BrowseCommand
        RelayCommand _browseCommand = null;
        public ICommand BrowseCommand
        {
            get
            {
                if (_browseCommand == null)
                {
                    _browseCommand = new RelayCommand((p) => OnBrowse(p), (p) => CanBrowse(p));
                }

                return _browseCommand;
            }
        }

        private bool CanBrowse(object parameter)
        {
            return executionContext.GetState() != ExecutionState.RUNNING;
            //return true;
        }

        private void OnBrowse(object parameter)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "Executable files (*.exe)|*.exe";
            if (openFileDialog.ShowDialog() == true)
            {
                SelectedFile = openFileDialog.FileName;
            }
        }

        #endregion 

        public bool SetPath(String path)
        {
            return executionContext.SetPath(path);
        }
    }
}

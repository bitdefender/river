using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace WpfRbTest2.PanelParts
{
    public sealed class StateConverter : IValueConverter
    {
        const UInt32 EXEC_MEM_FREE = 0x10000;
        const UInt32 EXEC_MEM_RESERVED = 0x2000;
        const UInt32 EXEC_MEM_COMMIT = 0x1000;

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            UInt32 v = (UInt32)value;

            switch (v)
            {
                case EXEC_MEM_FREE :
                    return "Free";
                case EXEC_MEM_RESERVED :
                    return "Reserved";
                case EXEC_MEM_COMMIT :
                    return "Commited";
                case EXEC_MEM_RESERVED | EXEC_MEM_COMMIT :
                    return "Reserved & Commited";
                default :
                    return String.Format("Unknown (0x{0:x8})", value);
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException("MethodToValueConverter can only be used for one way conversion.");
        }
    }

    public sealed class ProtectionConverter : IValueConverter
    {
        const UInt32 EXEC_PAGE_NOACCESS = 0x01;
        const UInt32 EXEC_PAGE_EXECUTE = 0x10;
        const UInt32 EXEC_PAGE_READONLY	= 0x02;
        const UInt32 EXEC_PAGE_READWRITE = 0x04;
        const UInt32 EXEC_PAGE_GUARD = 0x100;
        const UInt32 EXEC_PAGE_NOCACHE = 0x200;
        const UInt32 EXEC_PAGE_WRITECOMBINE = 0x400;

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            UInt32 v = (UInt32)value;

            String ret;

            switch (v & 7)
            {
                case EXEC_PAGE_READONLY:
                    ret = "R";
                    break;
                case EXEC_PAGE_READWRITE:
                    ret = "RW";
                    break;
                case EXEC_PAGE_NOACCESS :
                default :
                    ret = "";
                    break;
            }

            if ((EXEC_PAGE_EXECUTE & v) != 0)
            {
                ret += "X ";
            }
            
            if ((EXEC_PAGE_GUARD & v) != 0)
            {
                ret += "+G";
            }

            if ((EXEC_PAGE_NOCACHE & v) != 0)
            {
                ret += "+N";
            }

            if ((EXEC_PAGE_WRITECOMBINE & v) != 0)
            {
                ret += "+C";
            }
            return ret;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException("MethodToValueConverter can only be used for one way conversion.");
        }
    }

    public sealed class TypeConverter : IValueConverter
    {
        const UInt32 EXEC_MEM_IMAGE = 0x1000000;
        const UInt32 EXEC_MEM_MAPPED = 0x0040000;
        const UInt32 EXEC_MEM_PRIVATE = 0x0020000;

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            UInt32 v = (UInt32)value;

            switch (v)
            {
                case 0:
                    return "";
                case EXEC_MEM_IMAGE:
                    return "Image";
                case EXEC_MEM_MAPPED:
                    return "Mapped";
                case EXEC_MEM_PRIVATE:
                    return "Private";
                default:
                    return String.Format("Unknown (0x{0:x8})", value);
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException("MethodToValueConverter can only be used for one way conversion.");
        }
    }


    /// <summary>
    /// Interaction logic for MemoryMap.xaml
    /// </summary>
    public partial class MemoryMap : UserControl
    {
        public MemoryMap()
        {
            InitializeComponent();

            MemoryMapGrid.ItemsSource = Workspace.This.VirtualMemorySections;
        }
    }
}

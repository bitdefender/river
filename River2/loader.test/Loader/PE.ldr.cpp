#include <stdio.h>
#include <vector>
using namespace std;

//#include "..\extern.h"

#define DONOTPRINT

#ifdef DONOTPRINT
#define dbg_log(fmt,...) ((void)0)
#endif

#include "PE.ldr.h"

//#include "common/debug-log.h"
//#define dbg_log DbgPrint

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor


#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_5    5
#define IMAGE_REL_BASED_RESERVED              6
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_7    7
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_8    8
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_9    9
#define IMAGE_REL_BASED_DIR64                 10

#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     

#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80     

#define PAGE_GUARD            0x100     
#define PAGE_NOCACHE          0x200     
#define PAGE_WRITECOMBINE     0x400     
#define PAGE_REVERT_TO_FILE_MAP     0x80000000 

#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

const USHORT DOS_MAGIC = 0x5A4D;
const DWORD PE_MAGIC = 0x00004550;
const WORD OPT32_MAGIC = 0x010B;
const WORD OPT64_MAGIC = 0x020B;

const WORD IMAGE_FILE_MACHINE_I386 = 0x14C;
const WORD IMAGE_FILE_MACHINE_AMD64 = 0x8664;

typedef struct _IMAGE_BASE_RELOCATION {
    DWORD   VirtualAddress;
    DWORD   SizeOfBlock;
//  WORD    TypeOffset[1];
} IMAGE_BASE_RELOCATION;

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    DWORD   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
    DWORD   TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

    DWORD   ForwarderChain;                 // -1 if no forwarders
    DWORD   Name;
    DWORD   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD Name;
	DWORD Base;
	DWORD NumberOfFunctions;
	DWORD NumberOfNames;
	DWORD AddressOfFunctions; // RVA from base of image
	DWORD AddressOfNames; // RVA from base of image
	DWORD AddressOfNameOrdinals; // RVA from base of image
} IMAGE_EXPORT_DIRECTORY;

void *FloatingPE::RVA(DWORD rva) const {
    for (int i = 0; i < peHdr.NumberOfSections; ++i) {
        if (rva >= sections[i].header.VirtualAddress) {
            DWORD dwOffset = rva - sections[i].header.VirtualAddress;
            if (dwOffset < sections[i].header.VirtualSize) {
                return sections[i].data + dwOffset;
            }
        }
    }
    return (void *)0;
}

bool FloatingPE::Relocate(DWORD newAddr) {
	long long offset = 0;
	IMAGE_BASE_RELOCATION *reloc;
    IMAGE_BASE_RELOCATION *term;

	switch (peHdr.Machine) {
		case IMAGE_FILE_MACHINE_I386:
			offset = newAddr - optHdr.w32.ImageBase;
			optHdr.w32.ImageBase = newAddr;

			reloc = (IMAGE_BASE_RELOCATION *)RVA(optHdr.w32.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			term = (IMAGE_BASE_RELOCATION *)((unsigned char *)reloc + optHdr.w32.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			offset = newAddr - optHdr.w64.ImageBase;
			optHdr.w64.ImageBase = newAddr;

			reloc = (IMAGE_BASE_RELOCATION *)RVA(optHdr.w64.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			term = (IMAGE_BASE_RELOCATION *)((unsigned char *)reloc + optHdr.w64.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			break;
		default:
			dbg_log("Unsupported machine type\n");
			return false;
	}

    while (reloc < term) {
        //dbg_log("Relocation Block %08x %08x", reloc->VirtualAddress, reloc->SizeOfBlock);
        WORD *wloc = (WORD *)reloc + 4;
        int wlocCount = (reloc->SizeOfBlock - sizeof(*reloc)) / sizeof(*wloc);

        for (int i = 0; i < wlocCount; ++i) {
            //dbg_log("%x %x;\t", wloc[i] >> 12, wloc[i] & 0x0FFF);
			WORD type = wloc[i] >> 12;
			WORD off = wloc[i] & 0x0FFF;
			
			if ((i % 3) == 0) {
				//dbg_log("\n");
			}
			
			switch (type) {
				case IMAGE_REL_BASED_ABSOLUTE :
					break;
				case IMAGE_REL_BASED_HIGHLOW :
					*((DWORD *)RVA(reloc->VirtualAddress + off)) += (long)offset;
					//dbg_log("%08x -> %08x\t", reloc->VirtualAddress + off, *((DWORD *)RVA(reloc->VirtualAddress + off)));
					break;
				case IMAGE_REL_BASED_DIR64 :
					*((QWORD *)RVA(reloc->VirtualAddress + off)) += (long)offset;
					break;
				default :
					//__asm int 3;
					__debugbreak();
			}
        }
		//dbg_log("\n");
		reloc = (IMAGE_BASE_RELOCATION *)((unsigned char *)reloc + reloc->SizeOfBlock);
    }


    return true;
}

bool FloatingPE::GetExport(const char *funcName, DWORD &funcRVA) const {
	DWORD exportRVA = 0;
	
	switch (peHdr.Machine) {
		case IMAGE_FILE_MACHINE_I386:
			exportRVA = optHdr.w32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			exportRVA = optHdr.w64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			break;
		default:
			dbg_log("Unsupported machine type\n");
			return false;
	};
	
	if (0 == exportRVA) {
		return false;
	}

	IMAGE_EXPORT_DIRECTORY *exprt = (IMAGE_EXPORT_DIRECTORY *)RVA(exportRVA);

	dbg_log("Number of functions: %d\n", exprt->NumberOfFunctions);
	dbg_log("Number of names: %d\n", exprt->NumberOfNames);

	DWORD *names = (DWORD *)RVA(exprt->AddressOfNames);
	int found = -1;
	for (DWORD i = 0; i < exprt->NumberOfNames; ++i) {
		char *name = (char *)RVA(names[i]);
		if (0 == strcmp(name, funcName)) {
			found = i;
			break;
		}
	}

	if (-1 == found) {
		return false;
	}

	WORD *ordinals = (WORD *)RVA(exprt->AddressOfNameOrdinals);
	WORD ordinal = ordinals[found] - (WORD)exprt->Base + 1;

	DWORD *exportTable = (DWORD *)RVA(exprt->AddressOfFunctions);
	funcRVA = exportTable[ordinal];

	return true;
}

DWORD FloatingPE::GetRequiredSize() const {
	DWORD maxAddr = 0;
	for (DWORD i = 0; i < sections.size(); ++i) {
		if (sections[i].header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		DWORD maxSec = ((sections[i].header.VirtualAddress + sections[i].header.VirtualSize) + 0xFFF) & (~0xFFF);
		if (maxSec > maxAddr) {
			maxAddr = maxSec;
		}
	}

	return maxAddr;
}

DWORD FloatingPE::GetSectionCount() const {
	return sections.size();
}

const PESection *FloatingPE::GetSection(DWORD dwIdx) const {
	return &sections[dwIdx];
}

bool FloatingPE::FixImports(AbstractPEMapper &mapper) {
	DWORD importRVA = 0;
	
	
	switch (peHdr.Machine) {
	case IMAGE_FILE_MACHINE_I386:
		importRVA = optHdr.w32.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		break;
	case IMAGE_FILE_MACHINE_AMD64:
		importRVA = optHdr.w64.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		break;
	default:
		dbg_log("Unsupported machine type\n");
		return false;
	};

	if (0 == importRVA) {
		return true;
	}

    IMAGE_IMPORT_DESCRIPTOR *import = (IMAGE_IMPORT_DESCRIPTOR *)RVA(importRVA);
	if (NULL == import) {
		return false;
	}

    while (import->OriginalFirstThunk != 0) {
		char *name = (char *)RVA(import->Name);
        dbg_log("Directory for: %s\n", name);
		dbg_log("OFT: %08x, FT: %08x\n", import->OriginalFirstThunk, import->FirstThunk);
		DWORD *funcs = (DWORD *)RVA(import->OriginalFirstThunk);
		DWORD *iat = (DWORD *)RVA(import->FirstThunk);
        while (*funcs != 0) {
            if (0x80000000 & *funcs) {
                dbg_log("Import by ordinal %d\n", *funcs & 0x7FFF);
				*iat = mapper.FindImport(name, *funcs & 0x7FFF);
            } else {
				char *funcName = (char *)RVA(*funcs + 2);
                dbg_log("Import by name %s\n", funcName);
				*iat = mapper.FindImport(name, funcName);
            }

            funcs += 1;
			iat += 1;
        }

		import += 1;
    }

    return true;
}

bool FloatingPE::LoadPE(FILE *fModule) {
	if (NULL == fModule) {
		dbg_log("File open error!\n");
		return false;
	}

	if (1 != fread(&dosHdr, sizeof(dosHdr), 1, fModule)) {
		dbg_log("Read error\n");
		return false;
	}

	if (dosHdr.e_magic != DOS_MAGIC) {
		dbg_log("dosHdr.e_magic not valid\n");
		return false;
	}

	fseek(fModule, dosHdr.e_lfanew, SEEK_SET);


	if (1 != fread(&peHdr, sizeof(peHdr), 1, fModule)) {
		dbg_log("Read error\n");
		return false;
	}

	if (PE_MAGIC != peHdr.PeSignature) {
		dbg_log("peHdr.PeSignature not valid\n");
		return false;
	}

	//dbg_log("Machine: %d\n", peHdr.Machine);
	//dbg_log("NumberOfSections: %d\n", peHdr.NumberOfSections);
	//dbg_log("SizeOfOptionalHeader: %d\n", peHdr.SizeOfOptionalHeader);
	//dbg_log("Characteristics: %d\n", peHdr.Characteristics);

	DWORD optSize = 0;
	switch (peHdr.Machine) {
		case IMAGE_FILE_MACHINE_I386 :
			optSize = sizeof(optHdr.c) + sizeof(optHdr.w32);
			break;
		case IMAGE_FILE_MACHINE_AMD64 :
			optSize = sizeof(optHdr.c) + sizeof(optHdr.w64);
			break;
		default :
			dbg_log("Unsupported machine type\n");
			return false;
	}

	if (peHdr.SizeOfOptionalHeader != optSize) {
		dbg_log("Unsupported optional header\n");
		return false;
	}

	if (1 != fread(&optHdr, optSize, 1, fModule)) {
		dbg_log("Read error\n");
		return false;
	}


	switch (peHdr.Machine) {
		case IMAGE_FILE_MACHINE_I386:
			if (OPT32_MAGIC != optHdr.c.Magic) {
				dbg_log("opt32Hdr.Magic not valid\n");
				return false;
			}

			dbg_log("AddressOfEntryPoint: %x\n", optHdr.c.AddressOfEntryPoint);
			dbg_log("ImageBase: %08x\n", optHdr.w32.ImageBase);
			dbg_log("SectionAlignment: %d\n", optHdr.w32.SectionAlignment);
			dbg_log("FileAlignment: %d\n", optHdr.w32.FileAlignment);
			dbg_log("MajorSubsystemVersion: %d\n", optHdr.w32.MajorSubsystemVersion);
			dbg_log("SizeOfImage: %d\n", optHdr.w32.SizeOfImage);
			dbg_log("SizeOfHeaders: %d\n", optHdr.w32.SizeOfHeaders);
			dbg_log("Subsystem: %d\n", optHdr.w32.Subsystem);
			dbg_log("NumberOfRvaAndSizes: %d\n", optHdr.w32.NumberOfRvaAndSizes);
			dbg_log("DllCharacteristics: %04x\n", optHdr.w32.DllCharacteristics);

			dbg_log("Relocations: %08x (%08x)\n",
				optHdr.w32.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
				optHdr.w32.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size
			);
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			if (OPT64_MAGIC != optHdr.c.Magic) {
				dbg_log("opt64Hdr.Magic not valid\n");
				return false;
			}

			dbg_log("AddressOfEntryPoint: %x\n", optHdr.c.AddressOfEntryPoint);
			dbg_log("ImageBase: %016x\n", optHdr.w64.ImageBase);
			dbg_log("SectionAlignment: %d\n", optHdr.w64.SectionAlignment);
			dbg_log("FileAlignment: %d\n", optHdr.w64.FileAlignment);
			dbg_log("MajorSubsystemVersion: %d\n", optHdr.w64.MajorSubsystemVersion);
			dbg_log("SizeOfImage: %d\n", optHdr.w64.SizeOfImage);
			dbg_log("SizeOfHeaders: %d\n", optHdr.w64.SizeOfHeaders);
			dbg_log("Subsystem: %d\n", optHdr.w64.Subsystem);
			dbg_log("NumberOfRvaAndSizes: %d\n", optHdr.w64.NumberOfRvaAndSizes);
			dbg_log("DllCharacteristics: %04x\n", optHdr.w64.DllCharacteristics);

			dbg_log("Relocations: %08x (%08x)\n",
				optHdr.w64.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
				optHdr.w64.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size
				); break;
		default:
			dbg_log("Unsupported machine type\n");
			return false;
	}

	

	sections.resize(peHdr.NumberOfSections);

	dbg_log("Name\t V.Size\t V.Addr\t R.Size\t R.Addr\t Characteristics\n");
	for (int i = 0; i < peHdr.NumberOfSections; ++i) {
		if (1 != fread(&sections[i].header, sizeof(sections[i].header), 1, fModule)) {
			dbg_log("read error\n");
			return false;
		}

		dbg_log("%.8s\t %08x %08x %08x %08x %08x\n",
			sections[i].header.Name,
			sections[i].header.VirtualSize,
			sections[i].header.VirtualAddress,
			sections[i].header.SizeOfRawData,
			sections[i].header.PointerToRawData,
			sections[i].header.Characteristics);
	}

	for (int i = 0; i < peHdr.NumberOfSections; ++i) {
		if (!sections[i].Load(fModule)) {
			return false;
		}
	}

	return true;
}

FloatingPE::FloatingPE(const wchar_t *moduleName) {
	FILE *fModule; // = _wfopen_s(moduleName, L"rb");

	if (0 != _wfopen_s(&fModule, moduleName, L"rb")) {
		isValid = false;
		return;
	}

	isValid = LoadPE(fModule);
	fclose(fModule);
}

FloatingPE::FloatingPE(const char *moduleName) {
	FILE *fModule; // = fopen(moduleName, "rb");

	if (0 != fopen_s(&fModule, moduleName, "rb")) {
		isValid = false;
		return;
	}

	isValid = LoadPE(fModule);
	fclose(fModule);
}


FloatingPE::~FloatingPE() {
	//TODO: regular cleanup
	for (int i = 0; i < peHdr.NumberOfSections; ++i) {
		sections[i].Unload();
	}
}

bool FloatingPE::MapPE(AbstractPEMapper &mapr, DWORD &baseAddr) {
	
	
	FixImports(mapr);
    
	
	DWORD maxAddr = 0;
	for (DWORD i = 0; i < sections.size(); ++i) {
		if (sections[i].header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		DWORD maxSec = ((sections[i].header.VirtualAddress + sections[i].header.VirtualSize) + 0xFFF) & (~0xFFF);
		if (maxSec > maxAddr) {
			maxAddr = maxSec;
		}
	}
	

	baseAddr = (DWORD)mapr.CreateSection((void *)(baseAddr), maxAddr, PAGE_READWRITE);
	if (0 == baseAddr) {
		return false;
	}


	Relocate(baseAddr);

	//#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
	//#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
	//#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.
	static const DWORD pVec[] = {
		PAGE_NOACCESS,
		PAGE_EXECUTE,
		PAGE_READONLY,
		PAGE_EXECUTE_READ,

		PAGE_READWRITE, // should be write only
		PAGE_EXECUTE_READWRITE, // should be execute write
		PAGE_READWRITE,
		PAGE_EXECUTE_READWRITE
	};

    for (unsigned int i = 0; i < sections.size(); ++i) {
		if (sections[i].header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		if (sections[i].header.SizeOfRawData) {
			if (!mapr.WriteBytes((void *)(baseAddr + sections[i].header.VirtualAddress), sections[i].data, sections[i].header.SizeOfRawData)) {
				return false;
			}
		}

		DWORD protect = pVec[sections[i].header.Characteristics >> 29]; // get the three most significant bytes
		if (!mapr.ChangeProtect((void *)(baseAddr + sections[i].header.VirtualAddress), sections[i].header.VirtualSize, protect)) {
			return false;
		}
		
    }

	return true;
}

bool PESection::Load(FILE *fModule) {
    data = new unsigned char[header.SizeOfRawData];

    fseek(fModule, header.PointerToRawData, SEEK_SET);
    if (header.SizeOfRawData != fread(data, 1, header.SizeOfRawData, fModule)) {
        dbg_log("read error, in section %s\n", header.Name);
        return false;
    }

    return true;
}

void PESection::Unload() {
    if (data) {
        delete [] data;
    }
}

#ifdef DONOTPRINT
#undef dbg_log
#endif

















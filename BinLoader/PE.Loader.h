#ifndef _PE_LDR_H_
#define _PE_LDR_H_

#include <vector>
#include <functional>
using namespace std;

#include "Types.h"
#include "Abstract.Mapper.h"
#include "Abstract.Loader.h"


struct ImageSectionHeader {
    BYTE    Name[8];
    DWORD   VirtualSize;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
};

struct ImageDosHeader {  // DOS .EXE header
    USHORT e_magic;         // Magic number
    USHORT e_cblp;          // Bytes on last page of file
    USHORT e_cp;            // Pages in file
    USHORT e_crlc;          // Relocations
    USHORT e_cparhdr;       // Size of header in paragraphs
    USHORT e_minalloc;      // Minimum extra paragraphs needed
    USHORT e_maxalloc;      // Maximum extra paragraphs needed
    USHORT e_ss;            // Initial (relative) SS value
    USHORT e_sp;            // Initial SP value
    USHORT e_csum;          // Checksum
    USHORT e_ip;            // Initial IP value
    USHORT e_cs;            // Initial (relative) CS value
    USHORT e_lfarlc;        // File address of relocation table
    USHORT e_ovno;          // Overlay number
    USHORT e_res[4];        // Reserved words
    USHORT e_oemid;         // OEM identifier (for e_oeminfo)
    USHORT e_oeminfo;       // OEM information; e_oemid specific
    USHORT e_res2[10];      // Reserved words
    LONG   e_lfanew;        // File address of new exe header
};

struct ImagePeHeader {
    DWORD   PeSignature;
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
};

struct ImageDataDirectory {
    DWORD   VirtualAddress;
    DWORD   Size;
};

#pragma pack (push, 1)
struct ImageOptionalHeader {
	struct {
		WORD    Magic;
		BYTE    MajorLinkerVersion;
		BYTE    MinorLinkerVersion;
		DWORD   SizeOfCode;
		DWORD   SizeOfInitializedData;
		DWORD   SizeOfUninitializedData;
		DWORD   AddressOfEntryPoint;
		DWORD   BaseOfCode;
	} c;

	union {
		struct {
			DWORD   BaseOfData;
			DWORD   ImageBase;
			DWORD   SectionAlignment;
			DWORD   FileAlignment;
			WORD    MajorOperatingSystemVersion;
			WORD    MinorOperatingSystemVersion;
			WORD    MajorImageVersion;
			WORD    MinorImageVersion;
			WORD    MajorSubsystemVersion;
			WORD    MinorSubsystemVersion;
			DWORD   Win32VersionValue;
			DWORD   SizeOfImage;
			DWORD   SizeOfHeaders;
			DWORD   CheckSum;
			WORD    Subsystem;
			WORD    DllCharacteristics;
			DWORD   SizeOfStackReserve;
			DWORD   SizeOfStackCommit;
			DWORD   SizeOfHeapReserve;
			DWORD   SizeOfHeapCommit;
			DWORD   LoaderFlags;
			DWORD   NumberOfRvaAndSizes;
			ImageDataDirectory DataDirectory[16];
		} w32;

		struct {
			QWORD   ImageBase;
			DWORD   SectionAlignment;
			DWORD   FileAlignment;
			WORD    MajorOperatingSystemVersion;
			WORD    MinorOperatingSystemVersion;
			WORD    MajorImageVersion;
			WORD    MinorImageVersion;
			WORD    MajorSubsystemVersion;
			WORD    MinorSubsystemVersion;
			DWORD   Win32VersionValue;
			DWORD   SizeOfImage;
			DWORD   SizeOfHeaders;
			DWORD   CheckSum;
			WORD    Subsystem;
			WORD    DllCharacteristics;
			QWORD   SizeOfStackReserve;
			QWORD   SizeOfStackCommit;
			QWORD   SizeOfHeapReserve;
			QWORD   SizeOfHeapCommit;
			DWORD   LoaderFlags;
			DWORD   NumberOfRvaAndSizes;
			ImageDataDirectory DataDirectory[16];
		} w64;
	};   
};
#pragma pack (pop)

class PESection {
public :
	ImageSectionHeader header;
	unsigned char *data;

	bool Load(FILE *fModule);
	void Unload();
};

//typedef void (*ExportVerb)(const char *funcName, const DWORD ord, const DWORD rva, const unsigned char *body);

class FloatingPE : public AbstractBinary {
private :
	bool isValid;

	ImageDosHeader dosHdr;
	ImagePeHeader peHdr;
	ImageOptionalHeader optHdr;

	vector<PESection> sections;

	void *RVA(DWORD rva) const;
	bool LoadPE(FILE *fModule);

public :
	FloatingPE(const char *moduleName);
	FloatingPE(const wchar_t *moduleName);
	~FloatingPE();

	bool Relocate(DWORD newAddr);
	bool FixImports(AbstractPEMapper &mapper);

	bool GetExport(const char *funcName, DWORD &funcRVA) const;
	void ForAllExports(std::function<void (const char *, const DWORD, const DWORD, const unsigned char *)> verb) const;
	DWORD GetRequiredSize() const;
	DWORD GetSectionCount() const;
	const PESection *GetSection(DWORD dwIdx) const;

	virtual bool Map(AbstractPEMapper &mapr, DWORD &baseAddr);
	
	virtual bool IsValid() const {
		return isValid;
	}
};

#endif

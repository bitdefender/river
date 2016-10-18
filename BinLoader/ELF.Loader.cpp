#include "ELF.Loader.h"

//#define DONOTPRINT

#ifdef DONOTPRINT
#define dbg_log(fmt,...) ((void)0)
#else
#define dbg_log(fmt,...) printf(fmt, ##__VA_ARGS__)
#endif

#define SHT_NULL	 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB	 2
#define SHT_STRTAB	 3
#define SHT_RELA	 4
#define SHT_HASH	 5
#define SHT_DYNAMIC	 6
#define SHT_NOTE	 7
#define SHT_NOBITS	 8
#define SHT_REL		 9
#define SHT_SHLIB	 10
#define SHT_DYNSYM	 11
#define SHT_GNU_HASH 0x6FFFFFF6
#define SHT_VERNEED	 0x6FFFFFFE
#define SHT_VERSYM	 0x6FFFFFFF
#define SHT_LOPROC   0x70000000
#define SHT_HIPROC   0x7fffffff
#define SHT_LOUSER   0x80000000
#define SHT_HIUSER   0xffffffff

#define SHF_WRITE		0x1
#define SHF_ALLOC		0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

namespace ldr {
	bool ELFSection::Load(FILE *fModule) {
		if (SHT_NOBITS == header.sh_type) {
			return true;
		}

		data = new unsigned char[header.sh_size];

		fseek(fModule, header.sh_offset, SEEK_SET);
		if (header.sh_size != fread(data, 1, header.sh_size, fModule)) {
			dbg_log("read error, in section %s\n", (char *)header.sh_name);
			return false;
		}

		return true;
	}

	void ELFSection::Unload() {
		delete data;
	}

#define ELF_MAGIC 0x464C457F

#define ELFCLASSNONE		0
#define ELFCLASS32			1
#define ELFCLASS64			2

#define ELFDATANONE			0
#define ELFDATA2LSB			1
#define ELFDATA2MSB			2

#define ET_NONE				0
#define ET_REL				1
#define ET_EXEC				2
#define ET_DYN				3
#define ET_CORE				4
#define ET_LOPROC			0xff00
#define ET_HIPROC			0xffff

#define EM_NONE				0
#define EM_M32				1
#define EM_SPARC			2
#define EM_386				3
#define EM_68K				4
#define EM_88K				5
#define EM_860				7
#define EM_MIPS				8 

#define EV_NONE				0
#define EV_CURRENT			1

#define PT_NULL				0
#define PT_LOAD				1
#define PT_DYNAMIC			2
#define PT_INTERP			3
#define PT_NOTE				4
#define PT_SHLIB			5
#define PT_PHDR				6
#define PT_GNU_EH_FRAME		0x6474E550
#define PT_GNU_STACK		0x6474E551
#define PT_GNU_RELRO		0x6474E552
#define PT_LOPROC			0x70000000
#define PT_HIPROC			0x7fffffff

#define PF_R            0x4
#define PF_W            0x2
#define PF_X            0x1

	bool FloatingELF32::LoadELF(FILE *fModule) {
		if (NULL == fModule) {
			dbg_log("File open error!\n");
			return false;
		}

		if (1 != fread(&ident, sizeof(ident), 1, fModule)) {
			dbg_log("Read error\n");
			return false;
		}

		if (ELF_MAGIC != ident.e_magic) {
			dbg_log("ident.e_magic not valid %08x != %08x\n", ELF_MAGIC, ident.e_magic);
			return false;
		}

		if ((ELFCLASS32 != ident.e_class) || (ELFDATA2LSB != ident.e_data)) {
			dbg_log("ident.e_class or ident.e_data not supported\n");
			return false;
		}

		if (1 != fread(&header, sizeof(header), 1, fModule)) {
			dbg_log("Read error\n");
			return false;
		}

		if (EM_386 != header.e_machine) {
			dbg_log("header.e_machine not supported\n");
			return false;
		}

		//dbg_log("Base: %08x\n", header.)

		pHeaders.resize(header.e_phnum);
		fseek(fModule, header.e_phoff, SEEK_SET);
		dbg_log("Type     Offset   V.Addr   P.Addr   F.Size   M.Size   Flags    Align\n");
		for (int i = 0; i < header.e_phnum; ++i) {
			if (1 != fread(&pHeaders[i].header, sizeof(pHeaders[i].header), 1, fModule)) {
				dbg_log("read error\n");
				return false;
			}

			dbg_log("%08x %08x %08x %08x %08x %08x %08x %08x\n",
				pHeaders[i].header.p_type,
				pHeaders[i].header.p_offset,
				pHeaders[i].header.p_vaddr,
				pHeaders[i].header.p_paddr,
				pHeaders[i].header.p_filesz,
				pHeaders[i].header.p_memsz,
				pHeaders[i].header.p_flags,
				pHeaders[i].header.p_align
			);
		}

		sections.resize(header.e_shnum);
		fseek(fModule, header.e_shoff, SEEK_SET);
		dbg_log("\nName                     Type     Flags    Addr     Offset   Size     Link     Info     AddrAlgn EntSize\n");
		for (int i = 0; i < header.e_shnum; ++i) {
			if (1 != fread(&sections[i].header, sizeof(sections[i].header), 1, fModule)) {
				dbg_log("read error\n");
				return false;
			}

			if (SHT_STRTAB == sections[i].header.sh_type) {
				names = &sections[i];
			}
		}

		for (int i = 0; i < header.e_shnum; ++i) {
			if (!sections[i].Load(fModule)) {
				return false;
			}
		}

		for (int i = 0; i < header.e_shnum; ++i) {
			dbg_log("%24s %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
				&sections[header.e_shstrndx].data[sections[i].header.sh_name],
				sections[i].header.sh_type,
				sections[i].header.sh_flags,
				sections[i].header.sh_addr,
				sections[i].header.sh_offset,
				sections[i].header.sh_size,
				sections[i].header.sh_link,
				sections[i].header.sh_info,
				sections[i].header.sh_addralign,
				sections[i].header.sh_entsize
			);
		}

		for (int i = 0; i < header.e_shnum; ++i) {
			char *name = (char *)&sections[header.e_shstrndx].data[sections[i].header.sh_name];

			if (!strcmp(name, ".dynsym")) {
				DWORD symTableEnNo = sections[i].header.sh_size / sections[i].header.sh_entsize;
				dbg_log("For .dynsym found %d entries at %d each\n", symTableEnNo, sections[i].header.sh_entsize);
			}
		}

		return true;
	}

	FloatingELF32::FloatingELF32(const char *moduleName) {
		FILE *fModule = fopen(moduleName, "rb");

		if (nullptr == fModule) {
			isValid = false;
			return;
		}

		isValid = LoadELF(fModule);
		fclose(fModule);
	}

	/*FloatingELF32::FloatingELF32(const wchar_t *moduleName) {
		FILE *fModule = _wfopen(moduleName, L"rb");

		if (nullptr == fModule) {
			isValid = false;
			return;
		}

		isValid = LoadELF(fModule);
		fclose(fModule);
	}*/

	FloatingELF32::~FloatingELF32() {
		for (auto i = sections.begin(); i != sections.end(); ++i) {
			i->Unload();
		}
	}

	bool FloatingELF32::FixImports(AbstractPEMapper &mapper) {
		return false;
	}

	void FloatingELF32::MapSections(AbstractPEMapper &mapr, DWORD base, DWORD originalBase, DWORD startSeg, DWORD stopSeg) {
		for (auto i = sections.begin(); i != sections.end(); ++i) {
			if (SHF_ALLOC & i->header.sh_flags) {
				DWORD cpStart = i->header.sh_addr;
				if (cpStart < startSeg) {
					cpStart = startSeg;
				}

				DWORD cpStop = i->header.sh_addr + i->header.sh_size;
				if (cpStop > stopSeg) {
					cpStop = stopSeg;
				}

				if (cpStart < cpStop) { // we need to do some copying
					if (SHT_NOBITS == i->header.sh_type) {
						unsigned char *buff = new unsigned char[cpStop - cpStart];
						memset(buff, 0, cpStop - cpStart);
						mapr.WriteBytes((void *)(base - originalBase + cpStart), buff, cpStop - cpStart);
						delete buff;
					}
					else {
						mapr.WriteBytes((void *)(base - originalBase + cpStart), &i->data[cpStart - i->header.sh_addr], cpStop - cpStart);
					}
					dbg_log("%s[%d:%d] ", &sections[header.e_shstrndx].data[i->header.sh_name], cpStart - i->header.sh_addr, cpStop - i->header.sh_addr);
				}
			}
		}

		dbg_log("\n");
	}

	bool FloatingELF32::Map(AbstractPEMapper &mapr, DWORD &baseAddr) {
		DWORD oBA = 0xFFFFF000, oNA = 0;

		for (auto i = pHeaders.begin(); i != pHeaders.end(); ++i) {
			DWORD tBA = 0xFFFFF000, tNA = 0;
			switch (i->header.p_type) {
				case PT_LOAD :
					tBA = i->header.p_vaddr & ~(i->header.p_align - 1);
					tNA = (i->header.p_vaddr + i->header.p_memsz + i->header.p_align - 1) & ~(i->header.p_align - 1);
					break;
			}

			if (tBA < oBA) {
				oBA = tBA;
			}

			if (tNA > oNA) {
				oNA = tNA;
			}
		}

		dbg_log("Base: 0x%08x; Size: 0x%08x\n", oBA, oNA - oBA);

		baseAddr = (DWORD)mapr.CreateSection((void *)(baseAddr), oNA - oBA, PAGE_PROTECTION_READ | PAGE_PROTECTION_WRITE);
		if (0 == baseAddr) {
			return false;
		}

		// Relocate(baseAddr);

		static const DWORD prot[] = {
			0, 1, 2, 3, 4, 5, 6, 7
		};

		for (auto i = pHeaders.begin(); i != pHeaders.end(); ++i) {
			DWORD startSegment, stopSegment;
			bool loaded = false;
			switch (i->header.p_type) {
				case PT_LOAD:
					startSegment = i->header.p_vaddr & ~(i->header.p_align - 1);
					stopSegment = (i->header.p_vaddr + i->header.p_memsz + i->header.p_align - 1) & ~(i->header.p_align - 1);
					MapSections(mapr, baseAddr, oBA, startSegment, stopSegment);
					loaded = true;
					break;
			}

			if (loaded) {
				mapr.ChangeProtect((void *)(baseAddr - oBA + i->header.p_vaddr), stopSegment - startSegment, prot[i->header.p_flags]);
			}
		}


		return false;
	}

}; //namespace ldr
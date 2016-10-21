#include "Common.h"
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
	ELFSection::ELFSection() {
		data = nullptr;
		versions = verneed = nullptr;
	}

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

#define ELF32RSYM(i) ((i)>>8)
#define ELF32RTYPE(i) ((unsigned char)(i))
#define ELF32RINFO(s,t) (((s)<<8)+(unsigned char)(t)

#define R_386_NONE			0 
#define R_386_32			1
#define R_386_PC32			2
#define R_386_GOT32			3
#define R_386_PLT32			4
#define R_386_COPY			5
#define R_386_GLOB_DAT		6
#define R_386_JMP_SLOT		7
#define R_386_RELATIVE		8
#define R_386_GOTOFF		9
#define R_386_GOTPC			10

#define DT_NULL				0   
#define DT_NEEDED			1   
#define DT_PLTRELSZ			2   
#define DT_PLTGOT			3   
#define DT_HASH				4   
#define DT_STRTAB			5   
#define DT_SYMTAB			6   
#define DT_RELA				7   
#define DT_RELASZ			8   
#define DT_RELAENT			9   
#define DT_STRSZ			10   
#define DT_SYMENT			11   
#define DT_INIT				12   
#define DT_FINI				13   
#define DT_SONAME			14   
#define DT_RPATH			15   
#define DT_SYMBOLIC			16
#define DT_REL				17 
#define DT_RELSZ			18
#define DT_RELENT			19
#define DT_PLTREL			20
#define DT_DEBUG			21
#define DT_TEXTREL			22
#define DT_JMPREL			23
#define DT_LOPROC			0x70000000
#define DT_HIPROC			0x7fffffff

#define STB_LOCAL			0
#define STB_GLOBAL			1
#define STB_WEAK			2
#define STB_LOPROC			13
#define STB_HIPROC			15

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

#define ELF32STBIND(i) ((i)>>4)
#define ELF32STTYPE(i) ((i)&0xf

struct Elf32Rel{
	DWORD r_offset;
	DWORD r_info;
};

struct Elf32Rela {
	DWORD r_offset;
	DWORD r_info;
	LONG r_addend;
};

struct Elf32Sym {
	DWORD st_name;
	DWORD st_value;
	DWORD st_size;
	unsigned char st_info;
	unsigned char st_other;
	WORD st_shndx;
};

struct Elf32Dyn {
	LONG d_tag;
	DWORD d_un;
};

struct ELF32VerNeed {
	WORD vn_version;
	WORD vn_cnt;
	DWORD vn_file;
	DWORD vn_aux;
	DWORD vn_next;
};

struct Elf32VerAux {
	DWORD vna_hash;
	WORD vna_flags;
	WORD vna_other;
	DWORD vna_name;
	DWORD vna_next;
};

void *FloatingELF32::RVA(DWORD rva) const {
	for (auto i = sections.begin(); i != sections.end(); ++i) {
		if ((i->header.sh_addr <= rva) && (i->header.sh_addr + i->header.sh_size > rva)) {
			return &i->data[rva - i->header.sh_addr];
		}
	}
	return nullptr;
}

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

		moduleBase = 0xFFFFF000;
		pHeaders.resize(header.e_phnum);
		fseek(fModule, header.e_phoff, SEEK_SET);
		dbg_log("Type     Offset   V.Addr   P.Addr   F.Size   M.Size   Flags    Align\n");
		for (int i = 0; i < header.e_phnum; ++i) {
			if (1 != fread(&pHeaders[i].header, sizeof(pHeaders[i].header), 1, fModule)) {
				dbg_log("read error\n");
				return false;
			}

			DWORD tBA = 0xFFFFF000;
			switch (pHeaders[i].header.p_type) {
				case PT_LOAD:
					tBA = pHeaders[i].header.p_vaddr & ~(pHeaders[i].header.p_align - 1);
					break;
			};
			if (tBA < moduleBase) {
				moduleBase = tBA;
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

			switch (sections[i].header.sh_type) {
				case SHT_DYNAMIC :
					ParseDynamic(sections[i]);
					break;
				case SHT_VERSYM :
					sections[sections[i].header.sh_link].versions = &sections[i];
					break;
				case SHT_VERNEED :
					sections[sections[i].header.sh_info].verneed = &sections[i];
					ParseVerNeed(sections[i]);
					break;
			}
		}

		dbg_log("\nName                     Type     Flags    Addr     Offset   Size     Link     Info     AddrAlgn EntSize\n");
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

		return true;
	}

	FloatingELF32::FloatingELF32(const char *moduleName) {
		FILE *fModule = nullptr;
		
		if (0 != FOPEN(fModule, moduleName, "rb")) {
			isValid = false;
			return;
		}

		isValid = LoadELF(fModule);
		fclose(fModule);
	}

	FloatingELF32::FloatingELF32(const wchar_t *moduleName) {
		FILE *fModule = nullptr;
		
		if (0 != W_FOPEN(fModule, moduleName, L"rb")) {
			isValid = false;
			return;
		}

		isValid = LoadELF(fModule);
		fclose(fModule);
	}

	FloatingELF32::~FloatingELF32() {
		for (auto i = sections.begin(); i != sections.end(); ++i) {
			i->Unload();
		}
	}

	bool FloatingELF32::FixImports(AbstractPEMapper &mapper) {
		for (auto i = sections.begin(); i != sections.end(); ++i) {
			if ((SHT_DYNSYM == i->header.sh_type) || (SHT_SYMTAB == i->header.sh_type)) {
				Elf32Sym *symb = (Elf32Sym *)i->data;
				DWORD count = i->header.sh_size / sizeof(*symb);
				for (DWORD j = 0; j < count; ++j) {
					if ((STB_GLOBAL == ELF32STBIND(symb[j].st_info)) && (SHN_UNDEF == symb[j].st_shndx)) {
						dbg_log(
							"%s",
							(char *)&sections[i->header.sh_link].data[symb[j].st_name]
						);

						if (nullptr != i->versions) {
							ELFSymbolVersioning *vers = i->verneed->idxSVers[((WORD *)i->versions->data)[j]];
							if (nullptr != vers) {
								dbg_log(
									"@@%s",
									vers->version.c_str()
								);
							}

							mapper.FindImport(vers->module.c_str(), (char *)&sections[i->header.sh_link].data[symb[j].st_name]);
						} else {

						}
						dbg_log("\n");
					}
				}
			}
		}
		//return false;
	}

	bool FloatingELF32::PrintSymbols() const {
		for (auto i = sections.begin(); i != sections.end(); ++i) {
			if ((SHT_DYNSYM == i->header.sh_type) || (SHT_SYMTAB == i->header.sh_type)) {
				Elf32Sym *symb = (Elf32Sym *)i->data;
				DWORD count = i->header.sh_size / sizeof(*symb);
				for (DWORD j = 0; j < count; ++j) {
					if ((STB_GLOBAL == ELF32STBIND(symb[j].st_info)) && (SHN_UNDEF == symb[j].st_shndx)) {
						dbg_log(
							"%s",
							(char *)&sections[i->header.sh_link].data[symb[j].st_name]
						);

						if (nullptr != i->versions) {
							ELFSymbolVersioning *vers = i->verneed->idxSVers[((WORD *)i->versions->data)[j]];
							if (nullptr != vers) {
								dbg_log(
									"@@%s",
									vers->version.c_str()
								);
							}
						}
						dbg_log("\n");
					}
				}
			}
		}

		return true;
	}

	bool FloatingELF32::RelocateSection(void *r, DWORD count, const ELFSection &symb, const ELFSection &names, DWORD offset) {
		Elf32Rel *rels = (Elf32Rel *)r;
			for (DWORD i = 0; i < count; ++i) {
			dbg_log("Off: 0x%08x, Sym: 0x%06x, Typ: 0x%02x ", rels[i].r_offset, ELF32RSYM(rels[i].r_info), ELF32RTYPE(rels[i].r_info));
			DWORD *addr = (DWORD *)RVA(rels[i].r_offset);
			DWORD oldAddr;
			Elf32Sym *s;

			switch (ELF32RTYPE(rels[i].r_info)) {
				case R_386_NONE :
					dbg_log("$ none");
					break;
				case R_386_32 :
					s = &((Elf32Sym *)symb.data)[ELF32RSYM(rels[i].r_info)];
					oldAddr = *addr;
					*addr = offset + s->st_value;
					dbg_log("$ 0x%08x => 0x%08x; %s", oldAddr, *addr, (char *)&names.data[s->st_name]);
					//set *addr
					break;
				case R_386_PC32:
					s = &((Elf32Sym *)symb.data)[ELF32RSYM(rels[i].r_info)];
					oldAddr = *addr;
					dbg_log("$ %s", (char *)&names.data[s->st_name]);
					//set *addr
					break;
				case R_386_RELATIVE :
					oldAddr = *addr;
					*addr += offset;
					dbg_log("$ 0x%08x => 0x%08x", oldAddr, *addr);
					break;
				case R_386_GLOB_DAT :
					s = &((Elf32Sym *)symb.data)[ELF32RSYM(rels[i].r_info)];
					oldAddr = *addr;
					*addr = s->st_value;
					dbg_log("$ 0x%08x => 0x%08x; %s", oldAddr, *addr, (char *)&names.data[s->st_name]);
					//set *addr
					break;
			};

			dbg_log("\n");
		}
		return true;
	}

	bool FloatingELF32::Relocate(DWORD newBase) {
		DWORD offset = newBase - moduleBase;

		for (auto i = sections.begin(); i != sections.end(); ++i) {
			if (SHT_REL == i->header.sh_type) {
				if (rel) {
					DWORD symbols = i->header.sh_link;
					DWORD symNames = sections[symbols].header.sh_link;
					RelocateSection((Elf32Rel *)rel, relSz / relEnt, sections[symbols], sections[symNames], offset);
				}
			}
		}

		// Do the same for rela sections

		for (auto i = sections.begin(); i != sections.end(); ++i) {
			if (SHF_ALLOC & i->header.sh_flags) {
				i->header.sh_addr += offset;
			}
		}

		for (auto i = pHeaders.begin(); i != pHeaders.end(); ++i) {
			if (PT_LOAD == i->header.p_type) {
				i->header.p_vaddr += offset;
			}
		}

		moduleBase += offset;
		return true;
	}

	void FloatingELF32::MapSections(AbstractPEMapper &mapr, DWORD startSeg, DWORD stopSeg) {
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
						mapr.WriteBytes((void *)(cpStart), buff, cpStop - cpStart);
						delete buff;
					}
					else {
						mapr.WriteBytes((void *)(cpStart), &i->data[cpStart - i->header.sh_addr], cpStop - cpStart);
					}
					dbg_log("%s[%d:%d] ", &sections[header.e_shstrndx].data[i->header.sh_name], cpStart - i->header.sh_addr, cpStop - i->header.sh_addr);
				}
			}
		}

		dbg_log("\n");
	}

	bool FloatingELF32::ParseVerNeed(ELFSection &s) {
		DWORD maxVer = 0;
		s.sVers.clear();
		for (unsigned char *ptr = s.data; ptr < s.data + s.header.sh_size; ) {
			ELF32VerNeed *vn = (ELF32VerNeed *)ptr;
			ptr += sizeof(*vn);

			dbg_log("%s with %d entries\n", &sections[s.header.sh_link].data[vn->vn_file], vn->vn_cnt);

			for (DWORD j = 0; j < vn->vn_cnt; ++j) {
				Elf32VerAux *va = (Elf32VerAux *)ptr;
				ptr += sizeof(*va);

				dbg_log("\t %s -> %d\n", &sections[s.header.sh_link].data[va->vna_name], va->vna_other);
				if (maxVer < va->vna_other) {
					maxVer = va->vna_other;
				}
				s.sVers.push_back(ELFSymbolVersioning(va->vna_other, (char *)&sections[s.header.sh_link].data[va->vna_name], (char *)&sections[s.header.sh_link].data[vn->vn_file]));
			}
		}

		s.idxSVers.resize(maxVer + 1);
		for (DWORD i = 0; i <= maxVer; ++i) {
			s.idxSVers[i] = nullptr;
		}

		for (auto i = s.sVers.begin(); i != s.sVers.end(); ++i) {
			s.idxSVers[i->index] = &(*i);
		}
		return true;
	}

	bool FloatingELF32::ParseDynamic(const ELFSection &s) {
		Elf32Dyn *dyns = (Elf32Dyn *)s.data;
		DWORD cnt = s.header.sh_size / sizeof(dyns[0]);

		for (DWORD j = 0; j < cnt; ++j) {
			DWORD value = dyns[j].d_un;
			DWORD *rValue = nullptr;
			switch (dyns[j].d_tag) {
				case DT_PLTGOT :
				case DT_HASH :
				case DT_STRTAB :
				case DT_SYMTAB :
				case DT_RELA :
				case DT_INIT :
				case DT_FINI :
				case DT_REL :
				case DT_DEBUG :
				case DT_JMPREL :
					rValue = (DWORD *)RVA(value);
			}

			switch (dyns[j].d_tag) {
				case DT_NEEDED :
					dbg_log("Need library %s\n", &sections[s.header.sh_link].data[value]);
					libraries.push_back((char *)&sections[s.header.sh_link].data[value]);
					break;
				case DT_REL :
					rel = rValue;
					break;
				case DT_RELA :
					rela = rValue;
					break;
				case DT_RELSZ :
					relSz = value;
					break;
				case DT_RELASZ :
					relaSz = value;
					break;
				case DT_RELENT :
					relEnt = value;
					break;
				case DT_RELAENT :
					relaEnt = value;
					break;
				case DT_SONAME :
					dbg_log("So name %s\n", &sections[s.header.sh_link].data[value]);
					break;
			}
		}
		
		return true;
	}

	bool FloatingELF32::Map(AbstractPEMapper &mapr, DWORD &baseAddr) {
		DWORD oNA = 0;

		for (auto i = pHeaders.begin(); i != pHeaders.end(); ++i) {
			DWORD tNA = 0;
			switch (i->header.p_type) {
				case PT_LOAD :
					tNA = (i->header.p_vaddr + i->header.p_memsz + i->header.p_align - 1) & ~(i->header.p_align - 1);
					break;
			}

			if (tNA > oNA) {
				oNA = tNA;
			}
		}

		dbg_log("Base: 0x%08x; Size: 0x%08x\n", moduleBase, oNA - moduleBase);

		baseAddr = (DWORD)mapr.CreateSection((void *)(baseAddr), oNA - moduleBase, PAGE_PROTECTION_READ | PAGE_PROTECTION_WRITE);
		if (0 == baseAddr) {
			return false;
		}

		Relocate(baseAddr);

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
					MapSections(mapr, startSegment, stopSegment);
					loaded = true;
					break;
			}

			if (loaded) {
				mapr.ChangeProtect((void *)(i->header.p_vaddr), stopSegment - startSegment, prot[i->header.p_flags]);
			}
		}

		PrintSymbols();


		return false;
	}

}; //namespace ldr
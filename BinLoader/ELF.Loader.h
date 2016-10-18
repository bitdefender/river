#ifndef _ELF_LDR_H_
#define _ELF_LDR_H_

#include "Types.h"
#include "Abstract.Mapper.h"
#include "Abstract.Loader.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#undef _CRT_SECURE_NO_WARNINGS

#include <vector>

namespace ldr {
	struct ElfIdent {
		DWORD e_magic; /* 0x00 */
		BYTE e_class;  /* 0x04 */
		BYTE e_data;   /* 0x05 */
		BYTE e_version;/* 0x06 */
		BYTE e_pad[9]; /* 0x07 */
	};

	struct Elf32Header {
		WORD e_type;
		WORD e_machine;
		DWORD e_version;
		DWORD e_entry;
		DWORD e_phoff;
		DWORD e_shoff;
		DWORD e_flags;
		WORD e_ehsize;
		WORD e_phentsize;
		WORD e_phnum;
		WORD e_shentsize;
		WORD e_shnum;
		WORD e_shstrndx;
	};

#define SHN_UNDEF			0
#define SHN_LORESERVE		0xff00
#define SHN_LOPROC			0xff00
#define SHN_HIPROC			0xff1f
#define SHN_ABS				0xfff1
#define SHN_COMMON			0xfff2
#define SHN_HIRESERVE		0xffff

#define SHT_NULL			0
#define SHT_PROGBITS		1
#define SHT_SYMTAB			2
#define SHT_STRTAB			3
#define SHT_RELA			4
#define SHT_HASH			5
#define SHT_DYNAMIC			6
#define SHT_NOTE			7
#define SHT_NOBITS			8
#define SHT_REL				9
#define SHT_SHLIB			10
#define SHT_DYNSYM			11
#define SHT_LOPROC			0x70000000
#define SHT_HIPROC			0x7fffffff
#define SHT_LOUSER			0x80000000
#define SHT_HIUSER			0xffffffff


	struct Elf32Shdr {
		DWORD sh_name;
		DWORD sh_type;
		DWORD sh_flags;
		DWORD sh_addr;
		DWORD sh_offset;
		DWORD sh_size;
		DWORD sh_link;
		DWORD sh_info;
		DWORD sh_addralign;
		DWORD sh_entsize;
	};

	class ELFSection {
	public:
		Elf32Shdr header;
		unsigned char *data;

		bool Load(FILE *fModule);
		void Unload();
	};

	struct Elf32Phdr {
		DWORD p_type;
		DWORD p_offset;
		DWORD p_vaddr;
		DWORD p_paddr;
		DWORD p_filesz;
		DWORD p_memsz;
		DWORD p_flags;
		DWORD p_align;
	};

	class ELFProgramHeader {
	public:
		Elf32Phdr header;

	};

	class FloatingELF32 : public AbstractBinary {
	private:
		ElfIdent ident;
		Elf32Header header;

		std::vector<ELFProgramHeader> pHeaders;
		std::vector<ELFSection> sections;

		ELFSection *names;

		bool isValid;

		bool LoadELF(FILE *fModule);

		void MapSections(AbstractPEMapper &mapr, DWORD base, DWORD originalBase, DWORD startSeg, DWORD stopSeg);
	public:
		FloatingELF32(const char *moduleName);
		//FloatingELF32(const wchar_t *moduleName);
		~FloatingELF32();

		bool FixImports(AbstractPEMapper &mapper);

		virtual bool Map(AbstractPEMapper &mapr, DWORD &baseAddr);

		virtual bool IsValid() const {
			return isValid;
		}
	};
}; //namespace ldr

#endif


#ifndef ELF_H
#define ELF_H
#include <stdint.h>
struct elf_ident{
	char EI_MAG[4];
	uint8_t EI_CLASS;
	uint8_t EI_DATA;
	uint8_t EI_VERSION;
	uint8_t EI_OSABI;
	uint8_t EI_ABIVERSION;
	uint8_t EI_PAD[7];
};
struct elf_header{
	struct elf_ident e_ident;
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;//Вообще, их размер зависит от разрядности
	uint32_t e_phoff;//объектного файла (поле e_ident.EI_CLASS),
	uint32_t e_shoff;//однако, я не уверен, что эта ОС будет скомпилирована для 64 битных машин
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};
struct program_header{
	uint32_t p_type;
	//uint32_t p_flags; // для 64 битных машин
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags; // для 32 битных машин
	uint32_t p_align;
};
struct section_header{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
};
#endif
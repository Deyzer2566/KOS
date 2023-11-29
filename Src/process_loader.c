#include "process_loader.h"
#include "elf.h"
#include <ff.h>

enum LoadResult loadHeader(FIL* file, struct elf_header* header){
	UINT read;
	FRESULT res = f_read(file, header, sizeof(struct elf_header), &read);
	if(res != FR_OK || read != sizeof(struct elf_header))
		return LOAD_CANT_READ_ELF_HEADER;
	return LOAD_OK;
}

enum LoadResult checkHeader(struct elf_header* header){
	if(header->e_ident.EI_MAG[0] != 0x7f || header->e_ident.EI_MAG[1] != 0x45 || 
		header->e_ident.EI_MAG[2] != 0x4c || header->e_ident.EI_MAG[3] != 0x46)
		return LOAD_BAD_SIGNATURE;
	if(header->e_ident.EI_CLASS != 1)
		return LOAD_BAD_CLASS;
	if(header->e_ident.EI_DATA != 1)
		return LOAD_BAD_BYTE_ORDER;
	if(header->e_ident.EI_VERSION != 1)
		return LOAD_BAD_ELF_HEAD_VERSION;
	if(header->e_ident.EI_OSABI != 0)
		return LOAD_INCOMPATIBLE_OSABI;
	if(header->e_type != 2)
		return LOAD_NOT_EXECUTABLE_FILE;
	if(header->e_machine != 0x28)
		return LOAD_WRONG_ARCH;
	if(header->e_version != 1)
		return LOAD_BAD_ELF_VERSION;
	if(header->e_entry != PROGRAM_ENTRY)
		return LOAD_WRONG_ENTRY;
	return LOAD_OK;
}

enum LoadResult loadProgramHeader(FIL* file, struct program_header* prog_head){
	UINT read;
	FRESULT res = f_read(file, prog_head, sizeof(struct program_header), &read);
	if(res != FR_OK || read != sizeof(struct program_header))
		return LOAD_CANT_READ_PROGRAM_HEADER;
	return LOAD_OK;
}

enum LoadResult loadProcessFromElfFile(char* filename, struct Process* proc){
	FIL file;
	f_open(&file, filename, FA_READ);
	struct elf_header header;
	if(loadHeader(&file, &header) == LOAD_CANT_READ_ELF_HEADER){
		f_close(&file);
		return LOAD_CANT_READ_ELF_HEADER;
	}
	enum LoadResult headerCheckResult = checkHeader(&header);
	if(headerCheckResult != LOAD_OK){
		f_close(&file);
		return headerCheckResult;
	}
	f_lseek(&file, header.e_phoff);
	for(uint16_t i = 0; i < header.e_phnum; i++){
		struct program_header prog_head;
		enum LoadResult loadProgramHeaderResult = loadProgramHeader(&file, &prog_head);
		if(loadProgramHeaderResult != LOAD_OK)
			return loadProgramHeaderResult;
		if(prog_head.p_type == 1){
			if(prog_head.p_vaddr + prog_head.p_memsz - PROGRAM_ENTRY > PROGRAM_SIZE){
				f_close(&file);
				return LOAD_TOO_LARGE;
			}
			DWORD cur = f_tell(&file);
			f_lseek(&file, prog_head.p_offset);
			UINT read;
			f_read(&file, (void*)prog_head.p_vaddr, prog_head.p_filesz, &read);
			f_lseek(&file, cur);
			if(read != prog_head.p_filesz){
				f_close(&file);
				return LOAD_CANT_READ;
			}
			for(uint32_t i = prog_head.p_filesz; i < prog_head.p_memsz; i++)
				proc->data[i+prog_head.p_vaddr] = 0;
		}
	}
	proc->context.pc = header.e_entry;
	f_close(&file);
	return LOAD_OK;
}
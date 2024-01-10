#include "fatfs_extend.h"
FRESULT f_recursiveRemoveDir(const TCHAR* path){
	DIR dir;
	FRESULT res;
	FILINFO fileinfo={0};
	res = f_stat(path, &fileinfo);
	res = f_opendir(&dir, path);
	while(res == FR_OK && f_readdir(&dir, &fileinfo)==FR_OK){
		if(fileinfo.fname[0] == '\0')
			break;
		char fname[64]={0};
		strcat(fname, path);
		strcat(fname, _T("/"));
		strcat(fname, fileinfo.fname);
		if(fileinfo.fattrib & AM_DIR){
			res = f_recursiveRemoveDir(fname);
		}
		else{
			res = f_unlink(fname);
		}
	}
	res = f_closedir(&dir);
	return res;
}
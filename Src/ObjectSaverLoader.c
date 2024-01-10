#include "ObjectSaverLoader.h"
#include <ff.h>
#include <stdio.h>
#include "Scheduler.h"
extern TCHAR* PROC_DIR;
extern volatile struct Scheduler processScheduler;
enum SaveObjectResult saveObjectToFatFS(struct Object* object);
enum LoadObjectResult loadObjectFromFatFS(struct Object* obj);
enum LoadObjectResult loadObjectInfoFromFatFS(struct Object* obj);
enum SaveObjectResult saveObject(struct Object* object, enum Storage to){
	switch(to){
		case FATFS_storage:
			return saveObjectToFatFS(object);
		break;
	}
	return SAVE_OBJECT_INVALID_STORAGE;
}
enum LoadObjectResult loadObject(struct Object* object, enum Storage from){
	switch(from){
		case FATFS_storage:
			return loadObjectFromFatFS(object);
		break;
	}
	return LOAD_OBJECT_INVALID_STORAGE;
}
enum LoadObjectResult loadObjectInfo(struct Object* obj, enum Storage from){
	switch(from){
		case FATFS_storage:
			return loadObjectInfoFromFatFS(obj);
		break;
	}
	return LOAD_OBJECT_INVALID_STORAGE;
}
enum SaveObjectResult saveObjectToFatFS(struct Object* obj){
	FIL file;
	char fname[64];
	sprintf(fname, "%s/%d",PROC_DIR,obj->objId);
	f_open(&file, fname, FA_CREATE_ALWAYS | FA_WRITE);
	
	UINT wrote;
	f_write(&file, obj, sizeof(struct Object), &wrote);
	if(wrote != sizeof(struct Object)){
		f_close(&file);
		return SAVE_OBJECT_CANT_WRITE_OBJECT_INFO;
	}
	
	f_write(&file, obj->process.data, (int)obj->process.context.spStart - (int)(obj->process.data), &wrote);
	if(wrote != (int)obj->process.context.spStart - (int)(obj->process.data)){
		f_close(&file);
		return SAVE_OBJECT_CANT_WRITE_OBJECT_DATA;
	}
	
	f_write(&file, (void*)&processScheduler.handlerDataSize, sizeof(size_t), &wrote);
	if(wrote != sizeof(size_t)){
		f_close(&file);
		return SAVE_OBJECT_CANT_WRITE_OBJECT_HANDLER_DATA_SIZE;
	}
	
	f_write(&file, processScheduler.handlerData, processScheduler.handlerDataSize, &wrote);
	if(wrote != processScheduler.handlerDataSize){
		f_close(&file);
		return SAVE_OBJECT_CANT_WRITE_OBJECT_HANDLER_DATA;
	}
	
	f_close(&file);
	return SAVE_OBJECT_OK;
}
enum LoadObjectResult loadObjectInfoFromFatFS(struct Object* obj){
	FIL file;
	char fname[64];
	sprintf(fname, "%s/%d",PROC_DIR,obj->objId);
	f_open(&file, fname, FA_READ);
	UINT read;
	f_read(&file, obj, sizeof(struct Object),&read);
	f_close(&file);
	if(read != sizeof(struct Object))
		return LOAD_OBJECT_CANT_READ_OBJECT_INFO;
	return LOAD_OBJECT_OK;
}
enum LoadObjectResult loadObjectFromFatFS(struct Object* obj){	
	enum LoadObjectResult res = loadObjectInfo(obj, FATFS_storage);
	if(res != LOAD_OBJECT_OK)
		return res;
	
	FIL file;
	char fname[64];
	sprintf(fname, "%s/%d",PROC_DIR,obj->objId);
	f_open(&file, fname, FA_READ);
	f_lseek(&file, sizeof(struct Object));
	
	UINT read;
	
	f_read(&file, obj->process.data, (int)obj->process.context.spStart - (int)(obj->process.data), &read);
	if(read != (int)obj->process.context.spStart - (int)(obj->process.data)){
		f_close(&file);
		return LOAD_OBJECT_CANT_READ_OBJECT_DATA;
	}
	
	f_read(&file, (void*)&processScheduler.handlerDataSize, sizeof(size_t), &read);
	if(read != sizeof(size_t)){
		f_close(&file);
		return LOAD_OBJECT_CANT_READ_OBJECT_HANDLER_DATA_SIZE;
	}
	
	f_read(&file, processScheduler.handlerData, processScheduler.handlerDataSize, &read);
	if(read != processScheduler.handlerDataSize){
		f_close(&file);
		return LOAD_OBJECT_CANT_READ_OBJECT_HANDLER_DATA;
	}
	
	f_close(&file);
	return LOAD_OBJECT_OK;
}
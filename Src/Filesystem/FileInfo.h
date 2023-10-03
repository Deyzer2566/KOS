#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "Node.h"
enum FileType{File=0, Directory=1, Link=2};
struct FileInfo{
	char header[5];//"KOSFI"
	char name[32];
  bool canRead;
	bool canWrite;
	bool canExecute;
	size_t size; // Количество нод
	struct Node* data;
	enum FileType type;
	time_t creationTime;
	time_t modificationTime;
	time_t accessTime;
};
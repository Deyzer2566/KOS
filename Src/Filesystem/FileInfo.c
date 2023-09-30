#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "Node.c"
enum FileType{File=0, Directory=1};
struct FileInfo{
	char* name;
  bool canRead;
	bool canWrite;
	bool canExecute;
	size_t size; // count of nodes
	struct Node* data;
	enum FileType type;
	time_t creationTime;
	time_t modificationTime;
	time_t accessTime;
};
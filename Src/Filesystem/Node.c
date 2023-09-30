struct Node {
	char header[5]; //"KOSFS"
	char data[503]; 
	struct Node* nextPart; // 4 bytes ??
};
/*
	If the node is a file node, then data is a byte array of file
	If the node is a directory node, then data is a pointer to array of fileinfos
*/
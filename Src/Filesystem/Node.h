struct Node {
	char header[5]; //"KOSFS"
	char data[503];
	struct Node* nextPart; // 4 байта ??
};
/*
	Если нода указывает на файл, то data - содержимое файла (503 байта)
	Если нода указывает на директорию, то data - массив указателей типа FileInfo
*/
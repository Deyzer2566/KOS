#ifndef PROCESS_H
#define PROCESS_H
struct Process {
	void* stack; // адрес вершины стека
	char* data; // выделенная под программу память
	//char* overrideable_segments[15]; // сегменты, которые могут быть изменены
	int id; // ID процесса
	void* sp; // текущее значение sp
	void* pc; // текущее значение pc
};
#endif
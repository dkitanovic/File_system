#pragma once
#include "file.h"
#include "fs.h"
#include "KernelFS.h"
#include "KernelFile.h"
class KernelFile {
private:
	int* info_pos;
	int level_one;           //pamti index prvog nivoa
	int clusters_used;       //pamti koliko je klastera do sada iskorisceno
	int end_of_last_cluster; //pamti dokle smo stigli sa upisom u poslednji zauzeti klaster
	int last_cluster;        //pamti poslednji klaster
	int curr_cluster;
	static HANDLE take;
	BytesCnt size;
	BytesCnt curr;
	bool open;
	KernelFS* myKernel;
	char rw;
	char* content;
public:
	KernelFile(int* info, int level_one, int size,KernelFS* kernel,char rw,int c);

	~KernelFile();   //zatvaranje fajla   

	char write(BytesCnt bytes, char* buffer);

	BytesCnt read(BytesCnt bytes, char* buffer);

	char seek(BytesCnt bytes);   

	BytesCnt filePos();

	char eof();   BytesCnt getFileSize();

	char truncate();  

	void i_want_new_cluster();

	void give_me_next_cluster();

	int fja();
};
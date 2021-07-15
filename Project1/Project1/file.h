#pragma once
#include "part.h"
#include "fs.h"
#include "KernelFS.h"
class KernelFile; 

class File {

public:   
	~File();   //zatvaranje fajla   
    char write (BytesCnt bytes, char* buffer); 
    BytesCnt read (BytesCnt bytes, char* buffer);   
    char seek (BytesCnt bytes);   
	BytesCnt filePos();  
    char eof ();   
	BytesCnt getFileSize ();  
    char truncate (); 
	int fja();
private:   
	friend class FS;  
	friend class KernelFS;   
	File (int* info,int level_one,int last_cluster,KernelFS* kernel,char rw,int c);  //objekat fajla se može kreirati samo otvaranjem   
	KernelFile *myImpl; 
};
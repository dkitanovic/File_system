#pragma once
#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include <windows.h>
#include "part.h"
#include "fs.h"
#include "Def.h"
#include "KernelFile.h"
using namespace std;
class FS;
class KernelFS {

private:
	static const char all_ones;
	static char valid_init[2048];
	Partition* part;
	HANDLE unmount_sem;
	HANDLE closed_sem;
	HANDLE mutex1;
	HANDLE file_closed;
	bool can_open;
	static char root_init[2048];
	int number_of_files;
	int number_of_open;
	int mount_w;
	friend class KernelFile;
public:
	char mount(Partition* partition); //montira particiju          
	  
	char unmount();                   //demontira particiju     
											 // vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha 
	char format();                    //formatira particiju;  
											 // vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha 
	FileCnt readRootDir();            // vraca -1 u slucaju neuspeha ili broj fajlova u slucaju uspeha 
     
	char doesExist(char* fname);      //argument je naziv fajla sa            
	 
	File* open(char* fname, char mode);

	char deleteFile(char* fname);
	 
	bool get_valid(int clusterNo);
	
	int first_free_cluster();
	
	int* where_is_file(char* fname);

	int* find_free_space();

	void readCluster(int ClusterNo, char* buffer);

	void writeCluster(int ClusterNo, char* buffer);
	
	void free_clusters(int index1, int entry1, int entry2, int size);

	void set_v(int i);

	void close_File();
	
	void free_cluster(int ClusterNo);
	KernelFS();
};
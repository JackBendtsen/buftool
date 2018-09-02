#ifndef BUFTOOL_H
#define BUFTOOL_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
 #include <windows.h>
#else
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <sys/ptrace.h>
 #include <sys/wait.h>
 #include <fcntl.h>
#endif

#define MODE_DEC  0
#define MODE_AUTO 1
#define MODE_HEX  2

#define MEM_READ  false
#define MEM_WRITE true

#define BIG    false
#define LITTLE true

typedef unsigned char u8;

struct buffer {
	std::string name;
	u8 *buf = nullptr;
	int size;

	buffer(std::string& n, int sz = 0);
	~buffer();

	void checkup(void);
	int available(int off, int& len);
	int extend(int& off, int len);
	void resize(int sz, bool init = true);

	void copy(buffer& src, int dst_off = 0, int src_off = 0, int len = 0);
	void fill(u8 byte = 0, int off = 0, int len = 0);
	void patch(int off, std::vector<u8>& b);
	void print(int off, std::string& msg);
	void view(int off = 0, int len = 0);
	//void search(); // search by regex & binary, create/update buffer of results
	void load(std::string& file, int pos = 0, int len = 0, int off = 0);
	void save(std::string& file, int pos = 0, int len = 0, int off = 0, bool insert = true);
	int memacc(bool method, int pid, long addr, int len = 0, int off = 0);
};

#endif

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

#endif
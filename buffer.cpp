#include "buftool.h"

buffer::buffer(std::string& n, int sz) {
	name = n;
	buf = nullptr;
	size = 0;
	if (sz > 0)
		resize(sz);
}

buffer::~buffer() {
	delete[] buf;
	buf = nullptr;
}

void buffer::checkup(void) {
	if (buf == nullptr && size != 0)
		size = 0;

	else if (buf != nullptr && size <= 0) {
		delete[] buf;
		buf = nullptr;
		size = 0;
	}
}

int buffer::available(int off, int& len) {
	checkup();

	if (buf == nullptr || off < 0 || off >= size || len < 0)
		return -1;

	if (len == 0)
		len = size;
	if (off + len > size)
		len = size - off;

	return len;
}

int buffer::extend(int& off, int len) {
	checkup();

	// If there is no given length, we don't know how to extend the buffer
	if (len <= 0)
		return -1;

	// An offset of <0 means the offset is put at the end of the buffer
	if (off < 0)
		off = size;

	// If the given offset and length go beyond the size of the current buffer,
	//  we extend the buffer to match
	if (off + len > size)
		resize(off + len);

	return size;
}

void buffer::resize(int sz, bool init) {
	checkup();

	if (sz < 1)
		return;

	u8 *b = new u8[sz];

	if (buf != nullptr) {
		if (sz > size) {
			memcpy(b, buf, size);
			if (init)
				memset(b+size, 0, sz-size);
		}
		else
			memcpy(b, buf, sz);

		delete[] buf;
	}

	buf = b;
	size = sz;
}

void buffer::copy(buffer& src, int dst_off, int src_off, int len) {
	if (src.buf == nullptr ||
	    src_off < 0 || src_off >= src.size)
		return;

	if (len <= 0)
		len = src.size;

	if (src_off + len > src.size)
		len = src.size - src_off;

	if (extend(dst_off, len) > 0)
		memcpy(buf + dst_off, src.buf + src_off, len);
}

void buffer::fill(u8 byte, int off, int len) {
	if (extend(off, len) > 0 || available(off, len) > 0)
		memset(buf + off, byte, len);
}

void buffer::patch(int off, std::vector<u8>& b) {
	if (extend(off, b.size()) <= 0)
		return;

	int i;
	for (i = 0; i < b.size(); i++)
		buf[off+i] = b[i];
}

void buffer::print(int off, std::string& msg) {
	if (extend(off, msg.size()) > 0) {
		char *p = (char*)(buf+off);
		for (auto c:msg) *p++ = c;
	}
}

void buffer::view(int off, int len) {
	if (available(off, len) <= 0)
		return;

	int pos = off;
	int left = len;
	while (left > 0) {
		int row = left < 16 ? left : 16;
		int i;
		for (i = 0; i < row; i++) {
			// if it's the start of a row (i == 0), print the offset and separator
			if (i == 0)
				std::cout << std::setw(8) << std::right << std::setfill(' ') << std::hex
				          << pos+i << " | ";

			// print the byte as 2 characters, lower case, no prefix (eg. 0a)
			std::cout << std::hex << std::setfill('0') << std::setw(2)
			          << (int)buf[pos+i] << " ";

			// at each quarter of the row, print a divider
			if (i % 4 == 3 && i != row-1)
				std::cout << "- ";
		}

		if (row < 16) {
			// (2 + 1) represents the space taken by each printed byte
			int sp = 56 - (row * (2 + 1));
			int divs = (row - 1) / 4; // multiplied by 2, as 2 == strlen("- ")
			std::cout << std::setw(sp - (divs * 2)) << std::setfill(' ');
		}

		std::cout << "| ";

		for (i = 0; i < row; i++) {
			// printable ascii range
			if (buf[pos+i] >= ' ' && buf[pos+i] < '~')
				std::cout << buf[pos+i];
			else
				std::cout << ".";	
		}

		std::cout << "\n";

		left -= row;
		pos += row;
	}

	std::cout << "\n";
}

void buffer::load(std::string& file, int pos, int len, int off) {
	std::fstream in;
	in.open(file, std::fstream::in | std::fstream::binary | std::fstream::ate);
	int file_sz = in.tellg();

	if (!in.is_open() || file_sz <= 0) {
		std::cout << "Error: could not open " << file << "\n";
		return;
	}

	if (pos < 0)
		pos = file_sz - -pos;

	if (pos < 0 || pos >= file_sz) {
		std::cout << "Error: invalid file position " << std::to_string(pos)
		          << " (file size: " << std::to_string(file_sz) << ")\n";
		in.close();
		return;
	}

	int space = file_sz - pos;
	if (len <= 0)
		len = space;

	int read_sz = len < space ? len : space;

	if (extend(off, len) <= 0) {
		in.close();
		return;
	}

	in.seekg(in.beg);
	in.read((char*)(buf + off), read_sz);
	in.close();
}

void buffer::save(std::string& file, int pos, int len, int off, bool insert) {
	if (extend(off, len) <= 0 && available(off, len) <= 0)
		return;

	std::fstream in;
	in.open(file, std::fstream::in | std::fstream::binary | std::fstream::ate);
	int file_sz = in.tellg();

	if (in.is_open() && file_sz <= 0) {
		in.close();
		remove(file.c_str());
	}
	
	if (!in.is_open()) {
		if (pos < 0)
			return;

		std::fstream out;
		out.open(file, std::fstream::out | std::fstream::binary);

		if (pos > 0) {
			out.width(pos);
			out.fill('\0');
		}

		out.write((char*)(buf + off), len);
		out.close();
		return;
	}

	if (pos < 0)
		pos = file_sz - -pos;

	if (pos < 0 || (!insert && pos >= file_sz)) {
		in.close();
		return;
	}

	std::string b(file_sz, '\0');
	in.seekg(in.beg);
	in.read(&b[0], file_sz);
	in.close();

	if (insert) {
		if (pos > file_sz)
			b.insert(file_sz, pos - file_sz, '\0');

		b.insert(pos, (char*)(buf+off), len);
	}
	else {
		if (len > file_sz - pos)
			len = file_sz - pos;

		b.replace(pos, len, (char*)(buf+off));
	}

	std::fstream out;
	out.open(file, std::fstream::out | std::fstream::binary);
	out.write(&b[0], b.size());
	out.close();
}

#ifdef _WIN32

int buffer::memacc(bool mode, int pid, long addr, int len, int off) {
	if (extend(off, len) <= 0 && available(off, len) <= 0)
		return -1;

	HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	if (proc == nullptr) {
		std::cout << "Error: could not open process " << pid << "\n";
		return -2;
	}

	unsigned long r = 0;
	if (!mode)
		ReadProcessMemory(proc, (void*)addr, buf+off, len, &r);
	else {
		if (WriteProcessMemory(proc, (void*)addr, buf+off, len, &r))
			FlushInstructionCache(proc, (void*)addr, len);
	}

	CloseHandle(proc);
	return (int)r;
}

#else

int open_process(int pid, int mode) {
	std::string pn = "/proc/" + std::to_string(pid) + "/mem";
	int proc = open(pn.c_str(), mode ? O_RDWR : O_RDONLY);

	if (proc < 0) {
		std::string msg = "Could not open process from " + pn;
		perror(msg.c_str());
		return -1;
	}

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
		perror("Could not attach to process");
		close(proc);
		return -2;
	}

	if (waitpid(pid, NULL, 0) != pid) {
		perror("Could not wait for process"); // Too excited
		close(proc);
		return -3;
	}

	return proc;
}

int buffer::memacc(bool mode, int pid, long addr, int len, int off) {
	if (extend(off, len) <= 0 && available(off, len) <= 0)
		return -1;

	int proc = open_process(pid, mode);
	if (proc < 0)
		return -2;

	lseek(proc, addr, SEEK_SET);
	int r = mode ? write(proc, buf + off, len) :
	               read(proc, buf + off, len);

	ptrace(PTRACE_DETACH, pid, NULL, NULL);
	close(proc);

	return r;
}

#endif

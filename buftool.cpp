#include "buftool.h"

long read_number(const char *str, int mode = MODE_AUTO) {
	if (str == nullptr)
		return 0;

	if (mode == MODE_DEC)
		return atol(str);
	if (mode == MODE_AUTO)
		return strtol(str, NULL, 0);
	if (mode == MODE_HEX)
		return strtol(str, NULL, 16);

	return 0;
}

std::vector<buffer*> list;

int get_buffer(std::string& name) {
	int i;
	for (i = 0; i < list.size(); i++) {
		if (list[i] == nullptr)
			break;
		if (list[i]->name == name)
			return i;
	}

	buffer *buf = new buffer(&name);
	list.push_back(buf);
	return i;
}

struct command {
	std::string name;
	int min_args, max_args;
	std::string desc;
	std::string usage;
};

command commands[] = {
	{"help",   0, 1,  "Prints this help message", "help [command]"},
	{"quit",   0, 0,  "Exits the program", "quit"},
	{"info",   0, 1,  "Prints the current list of buffers", "info [buffer]"},
	{"delete", 1, 1,  "Deletes an existing buffer", "delete <name of buffer>"},
	{"copy",   2, 5,  "Copies data from one buffer to another", "copy <dest buffer> <src buffer> [dst offset] [src offset] [length]"},
	{"fill",   1, 4,  "Fills a range of data in a buffer with a value", "fill <buffer> [byte value] [offset] [length]"},
	{"set",    2, 5,  "Sets data to a buffer as if it were a variable", "set <buffer> <value> [type] [offset] [endian]"},
	{"patch",  3, -1, "Patches a byte array to a buffer at an offset", "patch <buffer> <offset> <byte array...>"},
	{"print",  3, -1, "Patches an ascii string to a buffer at an offset", "print <buffer> <offset> <string...>"},
	{"view",   1, 3,  "Displays data from a buffer", "view <buffer> [offset] [length]"},
	{"load",   2, 5,  "Loads data from a file into a buffer", "load <buffer> <file name> [position] [length] [offset]"},
	{"save",   2, 6,  "Saves data from a buffer to a file", "save <buffer> <file name> [position] [length] [offset] [insert (default=true)]"},
	{"read",   3, 5,  "Reads data from a running process into a buffer", "read <buffer> <PID> <address> [length] [offset]"},
	{"write",  3, 5,  "Writes data from a buffer to a running process", "write <buffer> <PID> <address> [length] [offset]"}
};

int n_cmds = sizeof(commands) / sizeof(command);

void help() {
	for (int i = 0; i < n_cmds; i++)
		std::cout << std::setw(9) << std::left << std::setfill(' ') << commands[i].name << commands[i].desc << "\n";

	std::cout << "\n";
}

void help(std::string& cmd) {
	int idx;
	for (idx = 0; idx < n_cmds; idx++)
		if (commands[idx].name == cmd) break;

	if (idx < n_cmds)
		std::cout << std::setw(9) << std::left << std::setfill(' ') << commands[idx].name << "\n    "
				  << commands[idx].desc << "\n    Usage: " << commands[idx].usage << "\n";
	else
		std::cout << "Unknown command \"" << cmd << "\"\n";

	std::cout << "\n";
}

std::string hex_string(long long int num, bool lower = true) {
	if (size <= 0)
		return std::to_string(size);

	std::string str = "0x";
	while (num > 0) {
		int d = num & 0xf;

		if (d >= 0 && d <= 9)
			d += '0';
		if (lower && d > 9)
			d += 'a' - 0xa;
		if (!lower && d > 9)
			d += 'A' - 0xa;

		str.push_back((char)d);
		num >>= 4;
	}

	return str;
}

template <class T>
void display_table(std::vector<T> table) {
	if (table.size() <= 1)
		return;

	int cols = std::tuple_size<T>::value;
	std::vector<int> width(cols);

	int i, j;
	for (i = 0; i < table.size(); i++) {
		for (j = 0; j < cols; j++) {
			std::string str = std::get<j>(table[i]);
			if (str.size() > width[j])
				width[j] = str.size();
		}
	}

	int w = 1;
	for (i = 0; i < cols; i++) {
		w += width[j];
		w += i == col-1 ? 1 : 3;
	}

	for (i = 0; i < table.size(); i++) {
		std::cout << " ";
		for (j = 0; j < cols; j++) {
			std::cout << " " << std::setw(width[j]) << std::left << std::setfill(' ')
			          << std::get<0>(table[i]);
			if (j < cols-1)
				std::cout << " | ";
			else
				std::cout << "\n";
		}

		if (i == 0)
			std::cout << std::string(w, '-') << "\n";
	}
}

void info() {
	std::vector<std::tuple<std::string, std::string>> table;
	table.push_back(std::make_tuple("Name", "Size"));

	for (int i = 0; i < list.size(); i++) {
		if (list[i] == nullptr)
			continue;

		auto sz_str = hex_string(list[i].size);
		sz_str += " (" + std::to_string(list[i].size) + ")";

		table.push_back(std::make_tuple(list[i].name, sz_str);
	}

	display_table(table);
}

void info(std::string name) {
	int idx = -1;
	for (int i = 0; i < list.size(); i++) {
		if (list[i] && list[i].name == name) {
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		std::cout << "Could not find buffer \"" << name << "\"\n";
		return;
	}

	std::vector<std::tuple<std::string, std::string, std::string>> table;
	table.push_back(std::make_tuple("Name", "Size", "Address"));

	auto sz_str = hex_string(list[i].size);
	sz_str += " (" + std::to_string(list[i].size) + ")";

	auto addr_str = hex_string((long long int)list[i].buf);

	table.push_back(std::make_tuple(list[i].name, sz_str, addr_str);
	display_table(table);
}

std::vector<std::string> get_args(std::string& cmd_str) {
	std::vector<std::string> cmd;

	// Tokenise string by splitting at the spaces
	int cur = 0, next = -1;
	do {
		next = cmd_str.find(" ", cur);
		cmd.push_back(cmd_str.substr(cur, next-cur));
		cur = next + 1;
	} while (next != std::string::npos);

	// Look up command word
	int c_idx;
	for (c_idx = 0; c_idx < n_cmds; c_idx++)
		if (commands[c_idx].name == cmd[0]) break;

	if (c_idx >= n_cmds) {
		std::cout << "Unknown command \"" << cmd[0] << "\"\n";
		return std::vector<std::string>();
	}

	// Validate the number of arguments for the given command
	//  by checking if the given amount is within the specified range
	if (cmd.size() - 1 < commands[c_idx].min_args) {
		std::cout << "Not enough arguments to power \"" << commands[c_idx].name << "\"\n"
			 << "Type \"help " << commands[c_idx].name << "\" for valid usage\n\n";
		return std::vector<std::string>();
	}
	// We need to check if the command actually has a maximum number of arguments (ie. not the "patch" or "print" command)
	//  before validating the given command's number of arguments
	if (cmd.size() - 1 > commands[c_idx].max_args && commands[c_idx].max_args >= 0) {
		std::cout << "Too many arguments to power \"" << commands[c_idx].name << "\"\n"
			 << "Type \"help " << commands[c_idx].name << "\" for valid usage\n\n";
		return std::vector<std::string>();
	}

	return cmd;
}

int main(int argc, char **argv) {
	std::cout << "Buffer Tool\n"
	     << "Type \"help\" for the list of available commands or\n"
	     << "\"help <command>\" for how to use a particular command\n\n";

	while (true) {
		std::cout << "> ";

		std::string cmd_str;
		getline(std::cin, cmd_str);
		std::cout << "\n";

		auto cmd = get_args(cmd_str);
		if (cmd.empty())
			continue;

		std::string type = commands[c_idx].name;

		if (type == "quit")
			break;

		if (type == "help") {
			if (cmd.size() == 1)
				help();
			else
				help(cmd[1]);
		}

		// TODO
		if (type == "info") {
			if (cmd.size() == 1)
				info();
			else
				info(cmd[1]);
		}
			std::cout << "Name | Size | Address\n";
			for (int i = 0; i < list.size(); i++) {
				if (list[i] == nullptr) continue;
				std::cout << "\t" << list[i]->name << " - 0x" << std::hex << list[i]->size << " (" << std::dec << list[i]->size << ")\n";
			}
			std::cout << "\n";
		}

		if (type == "delete") {
			int idx = get_buffer(cmd[1]);
			delete list[idx];
			list[idx] = nullptr;
		}

		if (type == "copy") {
			int dst_idx = get_buffer(cmd[1]);
			int src_idx = get_buffer(cmd[2]);

			int dst_off = 0, src_off = 0, len = 0;
			if (cmd.size() > 3)
				dst_off = read_number(cmd[3].c_str());
			if (cmd.size() > 4)
				src_off = read_number(cmd[4].c_str());
			if (cmd.size() > 5)
				len = read_number(cmd[5].c_str());

			list[idx]->copy(list[src_idx], dst_off, src_off, len);
		}

		if (type == "fill") {
			int idx = get_buffer(cmd[1]);
			int byte = 0, off = 0, len = 0;

			if (cmd.size() > 2)
				byte = read_number(cmd[2].c_str());
			if (cmd.size() > 3)
				off = read_number(cmd[3].c_str());
			if (cmd.size() > 4)
				len = read_number(cmd[4].c_str());

			list[idx]->fill((u8)byte, off, len);
		}

		// TODO
		if (type == "set") {
			std::string t = cmd.size() > 3 ? cmd[3] : "int";
			long long int value = 0;
			int size = 0;

			if (t == "byte") {
				char c;
				if (cmd[2].at(0) == '/')
					c = cmd[2].at(1);
				else
					c = read_number(cmd[2].c_str());

				((char*)&value)[0] = c;
				size = 1;
			}
			else if (t == "short") {
				short s = read_number(cmd[2].c_str());	
				size = sizeof(short);
				memcpy(&value, &s, size);
			}
			else if (t == "int") {
				int n = read_number(cmd[2].c_str());
				size = sizeof(int);
				memcpy(&value, &s, size);
			}
			else if (t == "long") {
				value = strtoll(cmd[2].c_str(), NULL, 0);
				size = sizeof(long long int);
			}
			else if (t == "float") {
				float f = atof(cmd[2].c_str());
				size = sizeof(float);
				memcpy(&value, &f, size);
			}
			else if (t == "double") {
				double d = atod(cmd[2].c_str());
				size = sizeof(double);
				memcpy(&value, &f, size);
			}
			else {
				std::cout << "Unrecognised variable type " << t << "\n";
				continue;
			}

			int off = 0;
			if (cmd.size() > 4)
				off = read_number(cmd[4].c_str());

			bool endian = LITTLE;
			if (cmd.size() > 5) {
				char c = cmd[5].at(0);
				if (c == 'b' || c == 'B')
					endian = BIG;
			}

			int idx = get_buffer(cmd[1]);
			std::vector<u8> buf;

			if (size == 1) {
				buf.push_back(((char*)&value)[0]);
				list[idx]->patch(off, buf);
				continue;
			}

			int start = 0, end = size, dir = 1;
			if (endian == BIG) {
				start = size - 1;
				end = -1;
				dir = -1;
			}

			for (int i = start; i != end; i += dir)
				buf.push_back(((char*)value)[i]);

			list[idx]->patch(off, buf);
		}

		if (type == "patch") {
			int idx = get_buffer(cmd[1]);
			int off = read_number(cmd[2].c_str());

			std::vector<u8> array;
			for (int i = 3; i < cmd.size(); i++)
				array.push_back((u8)strtol(cmd[i].c_str(), NULL, 16));

			list[idx]->patch(off, array);
		}

		if (type == "print") {
			int idx = get_buffer(cmd[1]);
			int off = read_number(cmd[2].c_str());

			std::string msg;
			for (int i = 3; i < cmd.size(); i++) {
				msg += cmd[i];
				if (i >= cmd.size() - 1)
					msg += " ";
			}

			list[idx]->print(off, msg);
		}

		if (type == "view") {
			int idx = get_buffer(cmd[1]);
			int off = 0, len = 0;

			if (cmd.size() > 2)
				off = read_number(cmd[2].c_str());
			if (cmd.size() > 3)
				len = read_number(cmd[3].c_str());

			list[idx]->view(off, len);
		}

		if (type == "load" || type == "save") {
			int idx = get_buffer(cmd[1]);
			int off = 0, len = 0;

			if (cmd.size() > 2)
				off = read_number(cmd[2].c_str());
			if (cmd.size() > 3)
				len = read_number(cmd[3].c_str());

			if (type == "load")
				list[idx]->load(cmd[2], off, len);
			else
				list[idx]->save(cmd[2], off, len);
		}

		if (type == "read" || type == "write") {
			int pid = read_number(cmd[2].c_str());
			long addr = read_number(cmd[3].c_str(), MODE_HEX);

			if (cmd.size() > 4)
				len = read_number(cmd[4].c_str());
			if (cmd.size() > 5)
				off = read_number(cmd[5].c_str());

			int r;
			if (type == "write")
				r = list[idx]->memacc(MEM_WRITE, pid, addr, len, off);
			else
				r = list[idx]->memacc(MEM_READ, pid, addr, len, off);

			int goal = list[idx]->available(off, len); 
			if (r < 0)
				std::cout << "Failed to " << type << " memory\n";
			else if (r < goal)
				std::cout << "Incomplete " << type
				          << " (" << std::to_string(r) << " / " << std::to_string(goal) << " bytes)\n";
		}
	}

	for (int i = 0; i < list.size(); i++) {
		if (list[i] == nullptr) continue;
		delete list[i];
	}
	return 0;
}

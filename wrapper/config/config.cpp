#include "config.h"
#include <fstream>
#include <streambuf>
#include <Windows.h>

#include "util.h"

void find_and_replace(std::string& source, std::string const& find, std::string const& replace) {
	for (std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;) {
		source.replace(i, find.length(), replace);
		i += replace.length();
	}
}

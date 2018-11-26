#include "util.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <windows.h>
#include "external/murmur/MurmurHash3.h"
#include "config/config.h"

#define EXPORT extern "C" __declspec(dllexport) 

static char * getLogFilename() {
	static char tmp[256] = "intercept.log";
	GetEnvironmentVariableA("LOGFILE", tmp, sizeof(tmp));
	return tmp;
}
static char * log_filename = getLogFilename();

LogLevel min_log_level = INFO;
EXPORT void _log(int l, const char * s) {
	static std::ofstream o(log_filename);
	time_t mytime = time(NULL);
	tm mytm;
	localtime_s(&mytm, &mytime);
	char stamp[128] = { 0 };
	strftime(stamp, sizeof(stamp), "%H:%M:%S", &mytm);
	if (l >= min_log_level) {
		o << "[ " << stamp << " ] " << s;
		o.flush();
	}
	{
		static HANDLE console = 0;
		if (!console) {
			AllocConsole();
			SetConsoleTitle("Intercept Log");
			console = GetStdHandle(STD_OUTPUT_HANDLE);
		}
		SetConsoleTextAttribute(console, FOREGROUND_GREEN);
		WriteConsole(console, "[ ", 2, nullptr, 0);
		WriteConsole(console, stamp, (DWORD)strlen(stamp), nullptr, 0);
		WriteConsole(console, " ] ", 3, nullptr, 0);

		if (l == INFO)
			SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
		if (l == WARN)
			SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		if (l == ERR)
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
		WriteConsole(console, s, (DWORD)strlen(s), nullptr, 0);
		SetConsoleTextAttribute(console, 0);
	}
}

LOG::~LOG() {
	s_ << std::endl;
	s_.flush();
	_log((int)l_, s_.str().c_str());
}

std::string strip(const std::string & i) {
	size_t a = 0, b = i.size();
	while (a < b && isblank(i[a])) a++;
	while (b > 1 && isblank(i[b - 1])) b--;
	return i.substr(a, b);
}
std::vector<std::string> split(const std::string & s, char c) {
	std::vector<std::string> r;
	for (size_t i = 0; i < s.size(); i++) {
		size_t j = s.find(c, i);
		r.push_back(s.substr(i, j));
		if (j == s.npos) break;
		i = j;
	}
	return r;
}

size_t hash_combine(size_t a, size_t b) {
	return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
}

std::ostream & operator<<(std::ostream & s, const Timer & t) {
	std::vector<std::string> k;
	for (const auto & i : t.time)
		k.push_back(i.first);
	std::sort(k.begin(), k.end());
	double tot = 0;
	for (const auto & i : k) {
		auto j = t.time.find(i);
		if (j != t.time.end()) {
			s << i << " : " << j->second / t.it << std::endl;
			tot += j->second / t.it;
		}
	}
	s << "---" << std::endl << "Total " << tot;
	return s;
}

__int64 Timer::tic() {
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return t.QuadPart;
}

void Timer::toc(const std::string & n, __int64 t) {
	static LARGE_INTEGER freq = { 0 };
	if (freq.QuadPart == 0)
		QueryPerformanceFrequency(&freq);
	std::lock_guard<std::mutex> guard(mutex);
	time[n] += 1.0 * (tic() - t) / freq.QuadPart;
}

void Timer::clear() {
	std::lock_guard<std::mutex> guard(mutex);
	time.clear();
	it = 0;
}

Timer::operator int() const {
	return it;
}

Timer & Timer::operator++() {
	it++;
	return *this;
}

void murmur3(const void * data, uint32_t len, uint32_t * out) {
	MurmurHash3_x64_128(data, len, 0x6384BA69, out);
}

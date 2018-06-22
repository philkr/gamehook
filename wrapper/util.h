#pragma once
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

enum LogLevel {
	INFO = 0,
	WARN = 1,
	ERR = 2
};
extern LogLevel min_log_level;

struct LOG {
protected:
	std::stringstream s_;
	LogLevel l_;
	LOG(const LOG & o) = delete;
	LOG& operator=(const LOG & o) = delete;
public:
	LOG(LogLevel l) :l_(l) {}
	~LOG();
	template<class T>
	LOG &operator<<(const T &x) {
		s_ << x;
		return *this;
	}
	LOG &operator<<(const std::wstring &s) {
		return *this << std::string(s.begin(), s.end());
	}
	LOG &operator<<(const wchar_t *s) {
		return *this << std::wstring(s);
	}
};

template<typename T>
LOG &operator<<(LOG &o, const std::vector<T> &v) {
	o << "[";
	for (size_t i = 0; i < v.size(); i++)
		o << v[i] << (i + 1 < v.size() ? ", " : "]");
	return o;
}

#define ASSERT(x) {if (!(x)) LOG(ERR) << __FILE__ << ":" << __LINE__ << "    Assertion failed! " << #x; }

std::string strip(const std::string & i);
std::vector<std::string> split(const std::string & s, char c);
size_t hash_combine(size_t a, size_t b);

struct Timer {
	std::mutex mutex;
	int it = 0;
	std::unordered_map<std::string, double> time;
	static __int64 tic();
	void toc(const std::string & n, __int64 tic);
	void clear();
	operator int() const;
	Timer &operator++();
};
std::ostream & operator<<(std::ostream & s, const Timer & t);

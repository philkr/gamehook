#pragma once
#include <sstream>
#include <vector>

extern "C" __declspec(dllimport) void _log(int, const char *);

enum LogLevel {
	INFO = 0,
	WARN = 1,
	ERR = 2
};

struct LOG {
protected:
	std::stringstream s_;
	LogLevel l_;
	LOG(const LOG & o) = delete;
	LOG& operator=(const LOG & o) = delete;
public:
	LOG(LogLevel l) :l_(l) {}
	~LOG() {
		s_ << std::endl;
		s_.flush();
		_log((int)l_, s_.str().c_str());
	}
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
	template<typename T>
	LOG &operator<<(const std::vector<T> &v) {
		s_ << "[";
		for (size_t i = 0; i < v.size(); i++) {
			s_ << v[i];
			if (i + 1 < v.size()) s_ << ", ";
		}
		s_ << "]";
		return *this;
	}
};

#define ASSERT(x) {if (!(x)) LOG(ERR) << __FILE__ << ":" << __LINE__ << "    Assertion failed! " << #x; }

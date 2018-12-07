#pragma once
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef IMPORT
#define IMPORT __declspec(dllimport)
#endif

typedef std::vector<uint8_t> ByteCode;

struct ShaderHash {
	uint32_t h[4] = { 0 };
	bool operator==(const ShaderHash & o) const;
	bool operator!=(const ShaderHash & o) const;
	operator bool() const;
	operator std::string() const;
	void reset();
	ShaderHash & operator=(uint32_t s);
	ShaderHash & operator=(const std::string & s);
	ShaderHash(const std::string & s);
	ShaderHash() = default;
};

class Shader {
public:
	enum Type {
		UNKNOWN = -1,
		VERTEX = 0,
		PIXEL = 1,
		COMPUTE = 2,
	};
	struct Buffer {
		struct Variable {
			std::string name, type;
			uint32_t offset = 0;
		};
		std::string name;
		uint32_t bind_point;
		std::vector<Variable> variables;
	};
	struct Binding {
		std::string name;
		uint32_t bind_point;
	};
protected:
	Shader() = default;
	Shader(const Shader &) = delete;
	Shader& operator=(const Shader &) = delete;
public:
	virtual ~Shader() = default;
	
	// name_remap is used to map output names (e.g. SV_Target7) to semantically meaningful names (e.g. disparity)
	static IMPORT std::shared_ptr<Shader> create(const ByteCode & src, const std::unordered_map<std::string, std::string> & name_remap = std::unordered_map<std::string, std::string>());

	// Append an other shader to this shader
	virtual std::shared_ptr<Shader> append(const std::shared_ptr<Shader> other) const = 0;

	// Shader subset and OI renaming
	virtual std::shared_ptr<Shader> subset(const std::vector<uint32_t> & outputs) const = 0;
	virtual std::shared_ptr<Shader> subset(const std::vector<std::string> & named_outputs) const = 0;
	virtual void renameCBuffer(const std::string & old_name, const std::string & new_name, int new_slot = -1) = 0;
	virtual void renameOutput(const std::string & old_name, const std::string & new_name, int sys_id = 0) = 0;

	virtual std::string disassemble() const = 0;

	// Shader info
	virtual Type type() const = 0;
	virtual const ShaderHash & hash() const = 0;

	virtual const std::vector<Binding> & inputs() const = 0;
	virtual const std::vector<Binding> & outputs() const = 0;
	virtual const std::vector<Buffer> & cbuffers() const = 0;
	virtual const std::vector<Buffer> & sbuffers() const = 0;
	virtual const std::vector<Binding> & textures() const = 0;

	// Shader bytecode and length
	virtual const uint8_t * data() const = 0;
	virtual size_t size() const = 0;
};

// IO
IMPORT std::ostream & operator<<(std::ostream & o, const ShaderHash & h);
IMPORT std::istream & operator>>(std::istream & o, ShaderHash & h);
IMPORT std::ostream & operator<<(std::ostream & o, const Shader::Buffer & h);
IMPORT std::ostream & operator<<(std::ostream & o, const Shader::Buffer::Variable & h);


/******************/
/* Implementation */
/******************/

inline bool ShaderHash::operator==(const ShaderHash & o) const {
	for (int i = 0; i < 4; i++)
		if (h[i] != o.h[i])
			return false;
	return true;
}
inline bool ShaderHash::operator!=(const ShaderHash & o) const {
	for (int i = 0; i < 4; i++)
		if (h[i] != o.h[i])
			return true;
	return false;
}
inline ShaderHash::operator bool() const {
	return h[0] || h[1] || h[2] || h[3];
}
inline ShaderHash::operator std::string() const {
	std::ostringstream o;
	o << *this;
	return o.str();
}
inline void ShaderHash::reset() {
	h[0] = h[1] = h[2] = h[3] = 0;
}
inline ShaderHash & ShaderHash::operator=(uint32_t c) {
	h[0] = h[1] = h[2] = h[3] = c;
	return *this;
}
inline ShaderHash & ShaderHash::operator=(const std::string & s) {
	std::istringstream(s) >> *this;
	return *this;
}
inline ShaderHash::ShaderHash(const std::string & s) { *this = s; }

namespace std {
	template<> struct hash<ShaderHash> {
		size_t operator()(const ShaderHash & h) const {
			std::hash<uint32_t> hasher;
			size_t r = 0;
			for (int i = 0; i<4; i++)
				r ^= hasher(h.h[i]) + 0x9e3779b9 + (r << 6) + (r >> 2);
			return r;
		}
	};
}

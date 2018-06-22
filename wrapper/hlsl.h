#pragma once
#include <memory>
#include <string>
#include <vector>

typedef uint32_t ChunkName;

struct HLSL {
	enum ShaderType {
		INVALID_SHADER = -1,
		PIXEL_SHADER,
		VERTEX_SHADER,
		GEOMETRY_SHADER,
		HULL_SHADER,
		DOMAIN_SHADER,
		COMPUTE_SHADER,
	};

	struct Buffer {
		struct Variable {
			std::string name, type;
			uint32_t offset;
		};
		std::string name;
		uint32_t bind_point;
		std::vector<Variable> variables;
	};
	struct Binding {
		std::string name;
		uint32_t bind_point;
	};
	typedef std::vector<uint8_t> ByteCode;
	std::vector<std::pair<ChunkName, ByteCode> > chunks_;

	HLSL() {}
	HLSL(const ByteCode & code) { parse(code); }
	void parse(const ByteCode & code);
	ByteCode write() const;
	bool empty() const;
	bool add_asm(const std::vector<std::string> &code);
	bool usesDiscard() const;
	bool writesDepth() const;
	void setEarlyDepthStencil(bool v);
	void addRetAfterDiscard();

	ShaderType type() const;
	std::vector<Binding> listInput() const;
	std::vector<Binding> listOutput() const;
	std::vector<Buffer> listCBuffers() const;
	std::vector<Buffer> listSBuffers() const;
	std::vector<Buffer> listBuffers(uint32_t mask) const;
	std::vector<Binding> listTextures() const;

	HLSL subset(const std::vector<uint32_t> & outputs);
	HLSL subset(const std::vector<std::string> & named_outputs);
	void renameOutput(const std::string & old_name, const std::string & new_name, int sys_id = 0);
	void renameCBuffer(const std::string & old_name, const std::string & new_name, int new_slot=-1);
};

bool merge(const HLSL & base, const HLSL & inject, HLSL * out, bool inject_after=false);

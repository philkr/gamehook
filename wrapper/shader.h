#pragma once
#include "api.h"

struct ID3D11VertexShader;
struct ID3D11PixelShader;

class ShaderImp : public Shader {
public:
	ByteCode code_;
	ShaderHash hash_;
	std::vector<Binding> inputs_, outputs_, textures_;
	std::vector<Buffer> cbuffers_, sbuffers_;
	std::unordered_map<std::string, std::string> name_remap_;
public:
	ShaderImp(const ByteCode & code, const std::unordered_map<std::string, std::string> & name_remap);

	virtual std::string disassemble() const override;

	virtual std::shared_ptr<Shader> subset(const std::vector<uint32_t> & outputs) const override;
	virtual std::shared_ptr<Shader> subset(const std::vector<std::string> & named_outputs) const override;
	virtual void renameCBuffer(const std::string & old_name, const std::string & new_name, int new_slot = -1) override;
	virtual void renameOutput(const std::string & old_name, const std::string & new_name, int sys_id) override;

	virtual const std::vector<Binding> & inputs() const override { return inputs_; }
	virtual const std::vector<Binding> & outputs() const override { return outputs_; }
	virtual const std::vector<Buffer> & cbuffers() const override { return cbuffers_; }
	virtual const std::vector<Buffer> & sbuffers() const override { return sbuffers_; }
	virtual const std::vector<Binding> & textures() const override { return textures_; }
	virtual const ShaderHash & hash() const override { return hash_; }

	virtual const uint8_t * data() const { return code_.data(); }
	virtual size_t size() const { return code_.size(); }
};

class VertexShader : public ShaderImp {
public:
	VertexShader(const ByteCode & code, const std::unordered_map<std::string, std::string> & name_remap = std::unordered_map<std::string, std::string>()) :ShaderImp(code, name_remap) {}
	virtual Type type() const override { return VERTEX; }
};

class PixelShader : public ShaderImp {
public:
	PixelShader(const ByteCode & code, const std::unordered_map<std::string, std::string> & name_remap = std::unordered_map<std::string, std::string>()) :ShaderImp(code, name_remap) {}
	virtual Type type() const override { return PIXEL; }
};

class ComputeShader : public ShaderImp {
public:
	ComputeShader(const ByteCode & code, const std::unordered_map<std::string, std::string> & name_remap = std::unordered_map<std::string, std::string>()) :ShaderImp(code, name_remap) {}
	virtual Type type() const override { return COMPUTE; }
};

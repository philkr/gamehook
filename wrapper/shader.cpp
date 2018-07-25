#include "api.h"
#include <iomanip>
#include <sstream>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include "util.h"
#include "shader.h"
#include "hlsl.h"

#pragma comment(lib,"d3dcompiler.lib")

std::ostream & operator<<(std::ostream & o, const ShaderHash & h) {
	std::ios::fmtflags f(o.flags());
	return o << std::hex << std::setfill('0') << std::setw(8) << h.h[0] << ":" << std::setw(8) << h.h[1] << ":" << std::setw(8) << h.h[2] << ":" << std::setw(8) << h.h[3] << std::setiosflags(f);
}
std::istream & operator>>(std::istream & i, ShaderHash & h) {
	std::string v;
	for (int k = 0; k < 4; k++)
		if (std::getline(i, v, ':'))
			std::istringstream(v) >> std::hex >> h.h[k];
	return i;
}

std::ostream & operator<<(std::ostream & o, const Shader::Buffer::Variable & h) {
	return o << h.type << " " << h.name << " (" << h.offset << ")";
}
std::ostream & operator<<(std::ostream & o, const std::vector<Shader::Buffer::Variable> &v) {
	o << "[";
	for (size_t i = 0; i < v.size(); i++) {
		o << v[i];
		if (i + 1 < v.size()) o << ", ";
	}
	return o << "]";
}
std::ostream & operator<<(std::ostream & o, const Shader::Buffer & h) {
	return o << h.name << "(" << h.bind_point << ")" << h.variables;
}

std::shared_ptr<Shader> Shader::create(const ByteCode & src, const std::unordered_map<std::string, std::string> & name_remap) {
	HLSL s(src);
	if (s.type() == HLSL::PIXEL_SHADER)
		return std::make_shared<PixelShader>(src, name_remap);
	if (s.type() == HLSL::VERTEX_SHADER)
		return std::make_shared<VertexShader>(src, name_remap);
	if (s.type() == HLSL::COMPUTE_SHADER)
		return std::make_shared<ComputeShader>(src, name_remap);
	LOG(WARN) << "Unsupported shader type";
	return nullptr;
}
std::shared_ptr<Shader> ShaderImp::append(const std::shared_ptr<Shader> other) const {
	std::shared_ptr<ShaderImp> o = std::dynamic_pointer_cast<ShaderImp>(other);
	ASSERT(o);
	HLSL ha(code_), hb(o->code_), hr;
	
	if (ha.usesDiscard() || ha.writesDepth()) {
		bool has_uav = hb.listBuffers(1 << 4).size();
		if (has_uav) {
			// TODO: Fancy merge (including a depth test) if at all possible
			LOG(ERR) << "You cannot use discard or depth writes with UAVs in injected shaders. Things will go wrong, and the UAV will contain the wrong values due to depth testing issues! Change the UAV to a pixel shader output instead!";

			// Disable early depth stencil testing
			hb.setEarlyDepthStencil(false);

			// Change all discard in ha to discard+retc
			ha.addRetAfterDiscard();
		}
	}
	std::unordered_map<std::string, std::string> name_remap;
	name_remap.insert(name_remap_.begin(), name_remap_.end());
	name_remap.insert(o->name_remap_.begin(), o->name_remap_.end());

	if (!::merge(ha, hb, &hr, true))
		return nullptr;
	ByteCode src = hr.write();

	if (hr.type() == HLSL::PIXEL_SHADER)
		return std::make_shared<PixelShader>(src, name_remap);
	if (hr.type() == HLSL::VERTEX_SHADER)
		return std::make_shared<VertexShader>(src, name_remap);
	if (hr.type() == HLSL::COMPUTE_SHADER)
		return std::make_shared<ComputeShader>(src, name_remap);
	return nullptr;
}
template<typename T1, typename T2, typename ...ARGS>
std::vector<T1> C(const std::vector<T2> & b, ARGS... args) {
	std::vector<T1> r;
	r.reserve(b.size());
	for (const auto & i : b)
		r.push_back(C(i, args...));
	return r;
}
Shader::Buffer::Variable C(const HLSL::Buffer::Variable & v) {
	return { v.name, v.type, v.offset };
}
Shader::Buffer C(const HLSL::Buffer & b) {
	return { b.name, b.bind_point, C<Shader::Buffer::Variable>(b.variables) };
}
Shader::Binding C(const HLSL::Binding & b, const std::unordered_map<std::string, std::string> & name_remap = std::unordered_map<std::string, std::string>()) {
	auto i = name_remap.find(b.name);
	if (i != name_remap.end())
		return { i->second, b.bind_point };
	return { b.name, b.bind_point };
}

ShaderImp::ShaderImp(const ByteCode & code, const std::unordered_map<std::string, std::string> & name_remap) :code_(code), name_remap_(name_remap){
	HLSL hlsl(code);
	inputs_ = C<Shader::Binding>(hlsl.listInput(), name_remap);
	outputs_ = C<Shader::Binding>(hlsl.listOutput(), name_remap);
	cbuffers_ = C<Shader::Buffer>(hlsl.listCBuffers());
	sbuffers_ = C<Shader::Buffer>(hlsl.listSBuffers());
	textures_ = C<Shader::Binding>(hlsl.listTextures());
	memcpy(hash_.h, code_.data() + sizeof(uint32_t), sizeof(hash_.h));
}

std::string ShaderImp::disassemble() const {
	ID3DBlob * disass;
	HRESULT hr = D3DDisassemble(code_.data(), code_.size(), D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, NULL, &disass);
	if (FAILED(hr)) {
		LOG(WARN) << "Failed to disassemble the shader code";
	}
	return std::string((const char*)disass->GetBufferPointer());
}

std::shared_ptr<Shader> ShaderImp::subset(const std::vector<uint32_t>& outputs) const {
	HLSL hlsl(code_);
	HLSL new_hlsl = hlsl.subset(outputs);

	if (type() == PIXEL)
		return std::make_shared<PixelShader>(new_hlsl.write());
	if (type() == VERTEX)
		return std::make_shared<VertexShader>(new_hlsl.write());
	if (type() == COMPUTE)
		return std::make_shared<ComputeShader>(new_hlsl.write());
	LOG(WARN) << "Unknown shader type " << type();
	return nullptr;
}

std::shared_ptr<Shader> ShaderImp::subset(const std::vector<std::string> & named_outputs) const {
	HLSL hlsl(code_);
	HLSL new_hlsl = hlsl.subset(named_outputs);

	if (type() == PIXEL)
		return std::make_shared<PixelShader>(new_hlsl.write());
	if (type() == VERTEX)
		return std::make_shared<VertexShader>(new_hlsl.write());
	if (type() == COMPUTE)
		return std::make_shared<ComputeShader>(new_hlsl.write());
	LOG(WARN) << "Unknown shader type " << type();
	return nullptr;
}

void ShaderImp::renameCBuffer(const std::string & old_name, const std::string & new_name, int new_slot) {
	HLSL hlsl(code_);
	hlsl.renameCBuffer(old_name, new_name, new_slot);
	code_ = hlsl.write();
}

void ShaderImp::renameOutput(const std::string & old_name, const std::string & new_name, int sys_id) {
	HLSL hlsl(code_);
	hlsl.renameOutput(old_name, new_name, sys_id);
	code_ = hlsl.write();
}

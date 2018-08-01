#include "rendertarget.h"
#include "util.h"
#include "shaders/compiled_fx_vs.h"
#include "shaders/compiled_resize_ps.h"
#include "shaders/compiled_resize_ps_i.h"
#include "shaders/compiled_resize_ps_u.h"

bool is_sint(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R8_SINT:
		return true;
	}
	return false;
}
bool is_uint(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8_UINT:
		return true;
	}
	return false;
}
DXGI_FORMAT uavFormat(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case DXGI_FORMAT_R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
	case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;

	case DXGI_FORMAT_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case DXGI_FORMAT_R16G16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
	case DXGI_FORMAT_R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;

	case DXGI_FORMAT_R32G32B32A32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
	case DXGI_FORMAT_R32G32_UINT: return DXGI_FORMAT_R32G32_UINT;
	case DXGI_FORMAT_R32_UINT: return DXGI_FORMAT_R32_UINT;

	case DXGI_FORMAT_R32G32B32A32_SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
	case DXGI_FORMAT_R32G32_SINT: return DXGI_FORMAT_R32G32_SINT;
	case DXGI_FORMAT_R32_SINT: return DXGI_FORMAT_R32_SINT;

	case DXGI_FORMAT_R16G16B16A16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
	case DXGI_FORMAT_R16G16_UINT: return DXGI_FORMAT_R16G16_UINT;
	case DXGI_FORMAT_R16_UINT: return DXGI_FORMAT_R16_UINT;

	case DXGI_FORMAT_R16G16B16A16_SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
	case DXGI_FORMAT_R16G16_SINT: return DXGI_FORMAT_R16G16_SINT;
	case DXGI_FORMAT_R16_SINT: return DXGI_FORMAT_R16_SINT;

	case DXGI_FORMAT_R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
	case DXGI_FORMAT_R16G16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
	case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UNORM;
	case DXGI_FORMAT_R16_UNORM: return DXGI_FORMAT_R16_UNORM;
	case DXGI_FORMAT_R16G16B16A16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
	case DXGI_FORMAT_R16G16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
	case DXGI_FORMAT_R16_SNORM: return DXGI_FORMAT_R16_SNORM;

	case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_R8G8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
	case DXGI_FORMAT_R8_UNORM: return DXGI_FORMAT_R8_UNORM;
	case DXGI_FORMAT_A8_UNORM: return DXGI_FORMAT_R8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
	case DXGI_FORMAT_R8G8_UINT: return DXGI_FORMAT_R8G8_UINT;
	case DXGI_FORMAT_R8_UINT: return DXGI_FORMAT_R8_UINT;

	case DXGI_FORMAT_R8G8B8A8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
	case DXGI_FORMAT_R8G8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
	case DXGI_FORMAT_R8_SNORM: return DXGI_FORMAT_R8_SNORM;

	case DXGI_FORMAT_R8G8B8A8_SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
	case DXGI_FORMAT_R8G8_SINT: return DXGI_FORMAT_R8G8_SINT;
	case DXGI_FORMAT_R8_SINT: return DXGI_FORMAT_R8_SINT;
	}
	return DXGI_FORMAT_UNKNOWN;
}
bool canMip(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_A8_UNORM:
		return true;
	}
	return false;
}
bool isSRGB(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return true;
	}
	return false;
}
bool isUnorm(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return true;
	}
	return false;
}

class ResizeShader: public FXShader {
	ID3D11VertexShader * vs;
	ID3D11PixelShader * ps, *ps_i, *ps_u;

	ID3D11Buffer * quad;
	ID3D11InputLayout * layout;
	D3D11Hook * h;
	ID3D11BlendState * no_blend;
	ID3D11RasterizerState * raster_state;
	ResizeShader(D3D11Hook * h):FXShader(h) {
		h->D3D11Hook::CreatePixelShader(::resize_ps, sizeof(::resize_ps), nullptr, &ps);
		h->D3D11Hook::CreatePixelShader(::resize_ps_i, sizeof(::resize_ps_i), nullptr, &ps_i);
		h->D3D11Hook::CreatePixelShader(::resize_ps_u, sizeof(::resize_ps_u), nullptr, &ps_u);
	}
	void _resize(int W, int H, ID3D11RenderTargetView * output, ID3D11ShaderResourceView * input, bool srgb = false, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) {
		// Setup the resize shader
		ID3D11PixelShader * _ps = ps;
		if (is_sint(format))
			_ps = ps_i;
		else if (is_uint(format))
			_ps = ps_u;
		call(W, H, { input }, { output }, ps);
	}
public:
template<typename ...ARGS>
	static void resize(D3D11Hook * h, ARGS... args) {
		static std::unordered_map<D3D11Hook *, std::shared_ptr<ResizeShader> > sm;
		if (!sm.count(h))
			sm[h] = std::shared_ptr<ResizeShader>(new ResizeShader(h));
		sm[h]->_resize(args...);
	}
};



struct RenderTargetOutput {
	D3D11Hook * h_;

	RenderTargetOutput(const RenderTargetOutput &) = delete;
	RenderTargetOutput& operator=(const RenderTargetOutput &) = delete;
	RenderTargetOutput(D3D11Hook * h): h_(h), tex_(h), stage_tex_(h) {}
	virtual ~RenderTargetOutput() {
	}

	RWTexture2D tex_;
	D3DTexture2D stage_tex_;

	D3D11_MAPPED_SUBRESOURCE mapped = { 0 };
	bool is_srgb = false;
	bool written_ = false;
	bool active = false;
	int W_ = 0, H_ = 0;
	bool setup(int W, int H, DXGI_FORMAT format) {
		DXGI_FORMAT uav_format = uavFormat(format);
		if (uav_format == DXGI_FORMAT_UNKNOWN) {
			LOG(ERR) << "Incompatible output texture format. Texture of type " << format << " cannot be resized.";
			return false;
		}
		tex_.setup(W, H, uav_format);
		is_srgb = isSRGB(format);
		stage_tex_.setup(W, H, uav_format, 1, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ);

		written_ = false;
		active = true;
		W_ = W;
		H_ = H;
		return true;
	}

	void fetch(ID3D11ShaderResourceView * v) {

		// Resize
		//ResizeShader::resize(h_, W_, H_, (ID3D11UnorderedAccessView*)tex_, v, is_srgb, tex_.format());
		ResizeShader::resize(h_, W_, H_, (ID3D11RenderTargetView*)tex_, v, is_srgb, tex_.format());

		// Copy over to staging
		h_->D3D11Hook::CopyResource(stage_tex_, tex_);

		written_ = true;
	}

	void map() {
		if (written_) {
			HRESULT hr = h_->D3D11Hook::Map(stage_tex_, 0, D3D11_MAP_READ, 0, &mapped);
			if (FAILED(hr) || !mapped.pData)
				LOG(ERR) << "Failed to map read texture! hr = " << std::hex << hr << std::dec;
		}
	}
	void unmap() {
		if (mapped.pData)
			h_->D3D11Hook::Unmap(stage_tex_, 0);
		mapped.pData = 0;
		active = written_ = false;
	}

	int W() const {
		return W_;
	}
	int H() const {
		return H_;
	}
	bool read(void * r, int C, DataType t) {
		if (stage_tex_.type() != t) {
			LOG(WARN) << "type mismatch got " << t << ", expected " << stage_tex_.type();
			return false;
		}
		if (C != stage_tex_.channels()) {
			LOG(WARN) << "Wrong number of channels provided " << C << ", expected " << stage_tex_.channels();
			return false;
		}
		if (!mapped.pData) {
			LOG(WARN) << "Output not written";
			return false;
		}
		size_t RS = W()*stage_tex_.channels()*dataSize(t), h = H();
		memset(r, 0, h*RS);
		for (size_t i = 0; i < h; i++)
			memcpy((uint8_t*)r + i*RS, (uint8_t*)mapped.pData + i*mapped.RowPitch, RS);
		return true;
	}
template<typename T>
	bool read(T * r, int C) {
		if (sizeof(T) != dataSize(stage_tex_.type())) {
			LOG(WARN) << "data size does not match";
			return false;
		}
		return read(r, C, stage_tex_.type());
	}
	template<typename T>
	bool read(T * r) {
		return read<T>(r, stage_tex_.channels());
	}
};


BaseRenderTarget::BaseRenderTarget(D3D11Hook * h): h_(h) {
}
BaseRenderTarget::~BaseRenderTarget() {
}
void BaseRenderTarget::mapOutputs() {
	for (auto o : outputs_)
		if (o.second->active)
			o.second->map();
}
void BaseRenderTarget::unmapOutputs() {
	for (auto o : outputs_)
		if (o.second->active)
			o.second->unmap();

	// Ready to be written again
	was_written = false;
}

void BaseRenderTarget::copyFrom(ID3D11View * view, DXGI_FORMAT hint) {
	ID3D11Resource * r;
	view->GetResource(&r);
	D3D11_RESOURCE_DIMENSION d;
	r->GetType(&d);
	if (d == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
		copyFrom((ID3D11Texture2D*)r, hint);
	else
		LOG(WARN) << "copyFrom: View doesn't hold a ID3D11Texture2D";
	r->Release();
}
void BaseRenderTarget::copyFrom(ID3D11RenderTargetView * view) {
	D3D11_RENDER_TARGET_VIEW_DESC desc;
	view->GetDesc(&desc);
	copyFrom((ID3D11View*)view, desc.Format);
}

void BaseRenderTarget::copyOutputFrom(ID3D11ShaderResourceView * view, DXGI_FORMAT format) {
	if (canMip(format))
		h_->D3D11Hook::GenerateMips(view);

	for (auto o : outputs_) 
		if (o.second->active) {
			o.second->setup(o.first.W, o.first.H, format);
			o.second->fetch(view);
		}
	// We just wrote the targets content
	was_written = true;
}

void BaseRenderTarget::addOutput(int W, int H) {
	if (!outputs_.count({ W, H }))
		outputs_[{W, H}] = std::make_shared<RenderTargetOutput>(h_);
	outputs_[{W, H}]->active = true;
}

bool BaseRenderTarget::read(int W, int H, int C, DataType T, void * d) {
	auto o = outputs_.find({ W, H });
	if (o != outputs_.end())
		return o->second->read(d, C, T);
	LOG(WARN) << "Output of size " << W << " x " << H << " not requested. Reading failed!";
	return false;
}
#define READ(T) bool BaseRenderTarget::read(int W, int H, int C, T * d) {\
	auto o = outputs_.find({W, H});\
	if (o != outputs_.end())\
		return o->second->read(d, C);\
	LOG(WARN) << "Output of size " << W << " x " << H << " not requested. Reading failed!";\
	return false;\
}
DO_ALL_TYPE(READ);
#undef READ
ID3D11Texture2D * BaseRenderTarget::tex() const {
	return nullptr;
}



RenderTarget::RenderTarget(D3D11Hook * h, DXGI_FORMAT hint) :BaseRenderTarget(h), tex_(h), hint_(hint) {}
RenderTarget::~RenderTarget() {
	if (view_) view_->Release();
}

void BaseRenderTarget::copyFrom(const BaseRenderTarget & rt) {
	copyFrom(rt.tex());
}

void RenderTarget::copyFrom(ID3D11Texture2D * tex, DXGI_FORMAT hint) {
	D3D11_TEXTURE2D_DESC t_desc = { 0 };
	tex->GetDesc(&t_desc);
	if (hint_ != DXGI_FORMAT_UNKNOWN)
		t_desc.Format = hint_;
	else if (hint != DXGI_FORMAT_UNKNOWN)
		t_desc.Format = hint;

	bool use_mip = canMip(t_desc.Format);
	LOG(INFO) << "Setup " << t_desc.Width << " " << t_desc.Height << " " << t_desc.Format << " " << use_mip;
	if (tex_.setup(t_desc.Width, t_desc.Height, t_desc.Format, use_mip ? 5 : 1, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | (use_mip ? D3D11_BIND_RENDER_TARGET : 0), 0, use_mip ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0)) {
		if (view_) view_->Release();
		HRESULT hr;
		D3D11_SHADER_RESOURCE_VIEW_DESC fmt = { t_desc.Format, D3D11_SRV_DIMENSION_TEXTURE2D };
		fmt.Texture2D.MipLevels = -1;
		fmt.Texture2D.MostDetailedMip = 0;
		hr = h_->D3D11Hook::CreateShaderResourceView(tex_, &fmt, &view_);
		if (FAILED(hr))
			LOG(ERR) << "Failed to create copy view. hr = " << std::hex << hr << std::dec;
	}

	if (t_desc.SampleDesc.Count > 1)
		h_->D3D11Hook::ResolveSubresource(tex_, 0, tex, 0, t_desc.Format);
	else
		h_->D3D11Hook::CopySubresourceRegion(tex_, 0, 0, 0, 0, tex, 0, nullptr);

	copyOutputFrom(view_, t_desc.Format);
}

DataType RenderTarget::type() const {
	return tex_.type();
}
int RenderTarget::channels() const {
	return tex_.channels();
}
TargetType RenderTarget::format() const {
	return (TargetType)tex_.format();
}

ID3D11Texture2D * RenderTarget::tex() const {
	return tex_;
}

RWTextureTarget::RWTextureTarget(D3D11Hook * h, DXGI_FORMAT format) :BaseRenderTarget(h), tex_(h), format_(format) {}
RWTextureTarget::~RWTextureTarget() {
	if (view_) view_->Release();
}
void RWTextureTarget::copyFrom(ID3D11Texture2D * tex, DXGI_FORMAT hint) {
	LOG(ERR) << "Cannot copy output to custom render targets!";
}
void RWTextureTarget::setup(int W, int H, DXGI_FORMAT format) {
	if (format == DXGI_FORMAT_UNKNOWN) format = format_;
	bool use_mip = canMip(format);
	if (tex_.setup(W, H, format, use_mip ? 5 : 1, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, use_mip ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0)) {
		if (view_) view_->Release();
		HRESULT hr = h_->D3D11Hook::CreateShaderResourceView(tex_, nullptr, &view_);
		if (FAILED(hr))
			LOG(ERR) << "Failed to create RWTextureTarget view. hr = " << std::hex << hr << std::dec;
	}
}
void RWTextureTarget::fetch() {
	copyOutputFrom(view_, tex_.format());
}
DataType RWTextureTarget::type() const {
	return dataType(format_);
}
int RWTextureTarget::channels() const {
	return ::channels(format_);
}
TargetType RWTextureTarget::format() const {
	return (TargetType)format_;
}

ID3D11Texture2D * RWTextureTarget::tex() const {
	return tex_;
}

void PipelineState::fetch(D3D11Hook * h) {
	h->D3D11Hook::VSGetShader(&old_vs, old_vs_ci, &old_vs_ni);
	h->D3D11Hook::PSGetShader(&old_ps, old_ps_ci, &old_ps_ni);
	h->D3D11Hook::PSGetShaderResources(0, 1, &old_ps_v);
	h->D3D11Hook::IAGetInputLayout(&old_il);
	h->D3D11Hook::IAGetVertexBuffers(0, 1, &old_vb, &old_stride, &old_offset);
	h->D3D11Hook::IAGetPrimitiveTopology(&old_topo);
	h->D3D11Hook::RSGetViewports(&old_n_vp, old_vp);
	h->D3D11Hook::RSGetState(&rs);
	h->D3D11Hook::OMGetRenderTargets(old_nrtv, old_rtv, &old_dsv);
	while (old_nrtv > 0 && !old_rtv[old_nrtv - 1]) old_nrtv--;
	h->D3D11Hook::OMGetBlendState(&old_bs, old_bf, &old_sm);
}

void PipelineState::restore(D3D11Hook * h) {
	h->D3D11Hook::VSSetShader(old_vs, old_vs_ci, old_vs_ni);
	h->D3D11Hook::PSSetShader(old_ps, old_ps_ci, old_ps_ni);
	for (UINT i = 0; i < old_vs_ni; i++) if (old_vs_ci[i]) old_vs_ci[i]->Release();
	for (UINT i = 0; i < old_ps_ni; i++) if (old_ps_ci[i]) old_ps_ci[i]->Release();
	if (old_ps) old_ps->Release();
	if (old_vs) old_vs->Release();

	h->D3D11Hook::PSSetShaderResources(0, 1, &old_ps_v);
	if (old_ps_v) old_ps_v->Release();

	h->D3D11Hook::IASetInputLayout(old_il);
	if (old_il) old_il->Release();

	h->D3D11Hook::IASetVertexBuffers(0, 1, &old_vb, &old_stride, &old_offset);
	if (old_vb) old_vb->Release();

	h->D3D11Hook::IASetPrimitiveTopology(old_topo);
	h->D3D11Hook::RSSetViewports(old_n_vp, old_vp);
	h->D3D11Hook::RSSetState(rs);
	if (rs) rs->Release();

	h->D3D11Hook::OMSetRenderTargetsAndUnorderedAccessViews(old_nrtv, old_rtv, old_dsv, 0, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, nullptr, nullptr);
	for (UINT i = 0; i < old_nrtv; i++) if (old_rtv[i]) old_rtv[i]->Release();

	h->D3D11Hook::OMSetBlendState(old_bs, old_bf, old_sm);
	if (old_bs) old_bs->Release();
}

FXShader::FXShader(D3D11Hook * h):h(h) {
	h->D3D11Hook::CreateVertexShader(::fx_vs, sizeof(::fx_vs), nullptr, &vs);
	float data[8] = { 0,0,1,0,0,1,1,1 };
	D3D11_SUBRESOURCE_DATA sr = { data, 0, 0 };
	D3D11_BUFFER_DESC desc = { 8 * sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	h->D3D11Hook::CreateBuffer(&desc, &sr, &quad);
	D3D11_INPUT_ELEMENT_DESC ied[] = { { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, };
	h->D3D11Hook::CreateInputLayout(ied, 1, ::fx_vs, sizeof(::fx_vs), &layout);

	D3D11_BLEND_DESC BlendState = { 0, 0 };
	BlendState.RenderTarget[0].BlendEnable = FALSE;
	BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	h->D3D11Hook::CreateBlendState(&BlendState, &no_blend);

	D3D11_RASTERIZER_DESC rs_desc = { D3D11_FILL_SOLID , D3D11_CULL_NONE, TRUE, 0, 0.f, 0.f, FALSE, FALSE, FALSE, FALSE };
	h->D3D11Hook::CreateRasterizerState(&rs_desc, &raster_state);
}

void FXShader::call(int W, int H, const std::vector<ID3D11ShaderResourceView *> & inputs, const std::vector<ID3D11RenderTargetView *> & outputs, ID3D11PixelShader * ps) {
	// Store the state
	PipelineState s;
	s.fetch(h);

	// Setup the resize shader
	h->D3D11Hook::VSSetShader(vs, NULL, 0);
	h->D3D11Hook::PSSetShader(ps, NULL, 0);

	UINT stride = 2 * sizeof(float), offset = 0;

	h->D3D11Hook::PSSetShaderResources(0, (UINT)inputs.size(), inputs.data());
	h->D3D11Hook::IASetInputLayout(layout);
	h->D3D11Hook::IASetVertexBuffers(0, 1, &quad, &stride, &offset);
	h->D3D11Hook::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	h->D3D11Hook::OMSetRenderTargetsAndUnorderedAccessViews((UINT)outputs.size(), outputs.data(), nullptr, 0, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, nullptr, nullptr);
	h->D3D11Hook::OMSetBlendState(no_blend, NULL, 0xffffffff);

	D3D11_VIEWPORT vp = { 0.f, 0.f, float(W), float(H), 0.f, 1.f };
	h->D3D11Hook::RSSetState(raster_state);
	h->D3D11Hook::RSSetViewports(1, &vp);
	h->D3D11Hook::Draw(4, 0);

	// Restore the state
	s.restore(h);
}

FXShader::~FXShader() {
	if (vs) vs->Release();
	if (quad) quad->Release();
	if (layout) layout->Release();
	if (no_blend) no_blend->Release();
	if (raster_state) raster_state->Release();
}

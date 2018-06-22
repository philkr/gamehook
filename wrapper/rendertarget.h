#pragma once
#include "api.h"
#include "dx11/d3d11.h"
#include "resource.h"
#include <memory>
#include <unordered_map>

struct RenderTargetOutput;

struct WH {
	int W, H;
	bool operator==(const WH & o) const { return W == o.W && H == o.H; }
};
namespace std {
	template <> struct hash<WH> {
		size_t operator()(const WH & x) const {
			return (((size_t)x.W) << (4*sizeof(size_t))) + x.H;
		}
	};
}

struct PipelineState {
	PipelineState() = default;
	PipelineState(const PipelineState&) = delete;
	PipelineState operator=(const PipelineState&) = delete;
	ID3D11PixelShader * old_ps = nullptr;
	ID3D11VertexShader * old_vs = nullptr;
	ID3D11ClassInstance * old_ps_ci[256] = { 0 }, *old_vs_ci[256] = { 0 };
	UINT old_ps_ni = 256, old_vs_ni = 256;
	ID3D11ShaderResourceView * old_ps_v = nullptr;
	ID3D11InputLayout * old_il = nullptr;
	ID3D11Buffer * old_vb;
	UINT old_stride = 0, old_offset = 0;
	D3D11_PRIMITIVE_TOPOLOGY old_topo;
	D3D11_VIEWPORT old_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT old_n_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	ID3D11RasterizerState * rs = nullptr;
	ID3D11RenderTargetView * old_rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { 0 };
	ID3D11DepthStencilView * old_dsv;
	UINT old_nrtv = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	ID3D11BlendState * old_bs;
	FLOAT old_bf[4];
	UINT old_sm;
	void fetch(D3D11Hook * h);
	void restore(D3D11Hook * h);
};
struct FXShader {
	FXShader(const FXShader&) = delete;
	FXShader operator=(const FXShader&) = delete;

	ID3D11VertexShader * vs = nullptr;

	ID3D11Buffer * quad = nullptr;
	ID3D11InputLayout * layout = nullptr;
	D3D11Hook * h = nullptr;
	ID3D11BlendState * no_blend = nullptr;
	ID3D11RasterizerState * raster_state = nullptr;
	FXShader(D3D11Hook * h);
	void call(int W, int H, const std::vector<ID3D11ShaderResourceView *> & inputs, const std::vector<ID3D11RenderTargetView *> & outputs, ID3D11PixelShader * ps);
	~FXShader();
};

struct BaseRenderTarget {
	D3D11Hook * h_ = nullptr;
	bool was_written = false;

	BaseRenderTarget(const BaseRenderTarget &) = delete;
	BaseRenderTarget& operator=(const BaseRenderTarget &) = delete;
	BaseRenderTarget(D3D11Hook * h);
	virtual ~BaseRenderTarget();

	void mapOutputs();
	void unmapOutputs();

	virtual DataType type() const = 0;
	virtual int channels() const = 0;
	virtual TargetType format() const = 0;

	virtual void copyFrom(ID3D11RenderTargetView * view);
	virtual void copyFrom(ID3D11View * view, DXGI_FORMAT hint = DXGI_FORMAT_UNKNOWN);
	virtual void copyFrom(const BaseRenderTarget & rt);
	virtual void copyFrom(ID3D11Texture2D * tex, DXGI_FORMAT hint = DXGI_FORMAT_UNKNOWN) = 0;

	void addOutput(int W, int H);
	bool read(int W, int H, int C, DataType T, void * d);
	bool read(int W, int H, int C, uint8_t * d);
	bool read(int W, int H, int C, uint16_t * d);
	bool read(int W, int H, int C, uint32_t * d);
	bool read(int W, int H, int C, half * d);
	bool read(int W, int H, int C, float * d);

	virtual ID3D11Texture2D * tex() const = 0;
	virtual ID3D11ShaderResourceView * rtv() const = 0;
protected:
	std::unordered_map<WH, std::shared_ptr<RenderTargetOutput> > outputs_;
	void copyOutputFrom(ID3D11ShaderResourceView * view, DXGI_FORMAT format);
};

struct RenderTarget: public BaseRenderTarget {
	D3DTexture2D tex_;
	ID3D11ShaderResourceView * view_ = nullptr;

	RenderTarget(const RenderTarget &) = delete;
	RenderTarget& operator=(const RenderTarget &) = delete;
	RenderTarget(D3D11Hook * h);
	virtual ~RenderTarget();

	using BaseRenderTarget::copyFrom;
	virtual void copyFrom(ID3D11Texture2D * tex, DXGI_FORMAT hint = DXGI_FORMAT_UNKNOWN);

	DataType type() const;
	int channels() const;
	TargetType format() const;
	virtual ID3D11Texture2D * tex() const;
	operator ID3D11ShaderResourceView *() const { return view_; }
	virtual ID3D11ShaderResourceView * rtv() const { return view_; }
};

struct RWTextureTarget: public BaseRenderTarget {
	DXGI_FORMAT format_;
	RWTexture2D tex_;
	ID3D11ShaderResourceView * view_ = nullptr;

	RWTextureTarget(const RenderTarget &) = delete;
	RWTextureTarget& operator=(const RWTextureTarget &) = delete;
	RWTextureTarget(D3D11Hook * h, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
	virtual ~RWTextureTarget();

	virtual void copyFrom(ID3D11Texture2D * tex, DXGI_FORMAT hint = DXGI_FORMAT_UNKNOWN);
	operator bool() const { return tex_; }
	operator ID3D11UnorderedAccessView *() const { return tex_; }
	operator ID3D11RenderTargetView *() const { return tex_; }
	operator ID3D11ShaderResourceView *() const { return view_; }

	void setup(int W, int H, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
	void fetch();

	DataType type() const;
	int channels() const;
	TargetType format() const;
	virtual ID3D11Texture2D * tex() const;
	virtual ID3D11ShaderResourceView * rtv() const { return view_; }
};

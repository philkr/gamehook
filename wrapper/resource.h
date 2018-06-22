#pragma once
#include <vector>
#include "dx11/d3d11.h"
#include "api.h"

class D3DBuffer {
protected:
	D3D11Hook * h_;

	D3D11_BUFFER_DESC desc_;
	ID3D11Buffer * buf_ = nullptr;
public:
	D3DBuffer(const D3DBuffer &) = delete;
	D3DBuffer& operator=(const D3DBuffer &) = delete;
	D3DBuffer(D3D11Hook * h, const D3D11_BUFFER_DESC & d = { 0 });
	virtual ~D3DBuffer();

	operator ID3D11Buffer *() const { return buf_; }
	operator bool() const { return !!buf_; }
	// Create or resize the buffer (must be called before read or cast)
	bool resize(size_t s);
	size_t read(void * data, size_t n);
	size_t write(const void * data, size_t n);
};

class D3DTexture2D {
protected:
	D3D11Hook * h_ = nullptr;

	D3D11_TEXTURE2D_DESC desc_;
	ID3D11Texture2D * tex_ = nullptr;
public:
	D3DTexture2D(const D3DTexture2D &) = delete;
	D3DTexture2D& operator=(const D3DTexture2D &) = delete;
	D3DTexture2D(D3D11Hook * h, const D3D11_TEXTURE2D_DESC & d = { 0 });
	virtual ~D3DTexture2D();
	
	DataType type() const;
	int channels() const;
	DXGI_FORMAT format() const;

	// Create or resize the texture (must be called before read or cast)
	virtual bool setup(uint32_t W, uint32_t H, DXGI_FORMAT format, uint32_t mip_level=1, D3D11_USAGE usage=D3D11_USAGE_DEFAULT, uint32_t bind_flags=0, uint32_t cpu_access=0, uint32_t misc_flags=0);
	operator ID3D11Texture2D *() const { return tex_; }
};

class RWTexture2D: public D3DTexture2D {
protected:
	ID3D11UnorderedAccessView * uav_ = nullptr;
	ID3D11RenderTargetView * rtv_ = nullptr;
public:
	RWTexture2D(const RWTexture2D &) = delete;
	RWTexture2D& operator=(const RWTexture2D &) = delete;
	RWTexture2D(D3D11Hook * h);
	virtual ~RWTexture2D();

	virtual bool setup(uint32_t W, uint32_t H, DXGI_FORMAT format, uint32_t mip_level = 1, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, uint32_t bind_flags = 0, uint32_t cpu_access = 0, uint32_t misc_flags = 0);
	operator ID3D11Texture2D *() const { return tex_; }
	operator ID3D11UnorderedAccessView *() const { return uav_; }
	operator ID3D11RenderTargetView *() const { return rtv_; }
	operator bool() const { return rtv_ && uav_; }
};

class GPUMemoryManager {
protected:
	struct BufferCache;
	friend struct DelayedGPUMemory;
	D3D11Hook * h_ = nullptr;
	D3DBuffer buf_, fetch_buf_;

	std::vector<uint8_t> cpu_memory_;
	std::vector<std::shared_ptr<DelayedGPUMemory> > current_delayed_memory_;
	std::shared_ptr<BufferCache> cache_;
	size_t fetch(D3DBuffer * b, size_t, std::vector<uint8_t> * r);

	size_t clear_id_ = 0, current_size_ = 0;
public:
	GPUMemoryManager(const GPUMemoryManager &) = delete;
	GPUMemoryManager& operator=(const GPUMemoryManager &) = delete;
	GPUMemoryManager(D3D11Hook * h);

	std::shared_ptr<GPUMemory> read(ID3D11Buffer * r, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate=false);

	void fetchDelayed();

	void clear();

	void cacheBuffer(ID3D11Buffer * buffer);
	void uncacheBuffer(ID3D11Buffer * buffer);
	void map(ID3D11Resource * r, D3D11_MAPPED_SUBRESOURCE *pMappedResource);
	void unmap(ID3D11Resource * r);
};

int channels(DXGI_FORMAT format);
DataType dataType(DXGI_FORMAT format);

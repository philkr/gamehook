#include "resource.h"
#include "util.h"
#include <mutex>

DataType dataType(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DataType::DT_FLOAT;

	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16_FLOAT:
		return DataType::DT_HALF;


	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32_UINT:
		return DataType::DT_UINT32;

	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R32G32B32_SINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32_SINT:
		return DataType::DT_UINT32;

	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16_UINT:
		return DataType::DT_UINT16;

	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R16_SINT:
		return DataType::DT_UINT16;

	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16_SNORM:
		return DataType::DT_UINT16;

	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DataType::DT_UINT8;

	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8_UINT:
		return DataType::DT_UINT8;

	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8_SNORM:
		return DataType::DT_UINT8;

	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R8_SINT:
		return DataType::DT_UINT8;
	}
	return DataType::DT_UNKNOWN;
}
int channels(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return 4;

	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 3;

	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
		return 2;

	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return 1;
	}
	return 0;
}

D3DBuffer::D3DBuffer(D3D11Hook * h, const D3D11_BUFFER_DESC & d) :h_(h), desc_(d) {}

D3DBuffer::~D3DBuffer() {
	if (buf_) buf_->Release();
}

bool D3DBuffer::resize(size_t s) {
	if (!buf_ || desc_.ByteWidth < s) {
		size_t old_size = desc_.ByteWidth;
		desc_.ByteWidth = max(desc_.ByteWidth, (2 * s + 16) & (~0xf));

		// Create a larger D3DBuffer
		ID3D11Buffer * new_buf = nullptr;
		HRESULT hr = h_->D3D11Hook::CreateBuffer(&desc_, nullptr, &new_buf);
		if (FAILED(hr)) {
			LOG(WARN) << "Failed to create D3DBuffer!";
			return false;
		}

		if (old_size && buf_) {
			D3D11_BOX bx = { 0, 0, 0, (UINT)old_size, 1, 1 };
			h_->D3D11Hook::CopySubresourceRegion(new_buf, 0, 0, 0, 0, buf_, 0, &bx);
		}
		if (buf_) buf_->Release();
		buf_ = new_buf;
		return true;
	}
	return false;
}

size_t D3DBuffer::read(void * data, size_t n) {
	if (!buf_) return 0;
	if (n > desc_.ByteWidth) n = desc_.ByteWidth;

	D3D11_MAPPED_SUBRESOURCE res;
	HRESULT hr = h_->D3D11Hook::Map(buf_, 0, D3D11_MAP_READ, 0, &res);
	if (FAILED(hr) || !res.pData) {
		LOG(WARN) << "Buffer: Failed to map for reading! hr = " << std::hex << hr;
		return 0;
	}
	memcpy(data, res.pData, n);
	h_->D3D11Hook::Unmap(buf_, 0);
	return n;

}

size_t D3DBuffer::write(const void * data, size_t n) {
	if (n > desc_.ByteWidth || !buf_) resize(n);

	D3D11_MAPPED_SUBRESOURCE res;
	HRESULT hr = h_->D3D11Hook::Map(buf_, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	if (FAILED(hr) || !res.pData) {
		LOG(WARN) << "Buffer: Failed to map for writing! hr = " << std::hex << hr;
		return 0;
	}
	memcpy(res.pData, data, n);
	h_->D3D11Hook::Unmap(buf_, 0);
	return n;
}


struct ReferenceMemory : public GPUMemory {
	const uint8_t * memory;
	size_t s;
	ReferenceMemory(const uint8_t * m, size_t s) :memory(m), s(s) {}

	virtual const void * data() const final { return memory; }
	virtual size_t size() const final { return s; }
	virtual std::shared_ptr<GPUMemory> sub(size_t O, size_t N) {
		ASSERT(O + N <= s);
		return std::make_shared<ReferenceMemory>(memory+O, N);
	}
};

struct ImmediateGPUMemory : public GPUMemory {
	std::vector<uint8_t> memory;
	ImmediateGPUMemory(const std::vector<uint8_t> & m) :memory(m) {}
	ImmediateGPUMemory(size_t s = 0) :memory(s) {}

	virtual const void * data() const final { return memory.data(); }
	virtual size_t size() const final { return memory.size(); }
	virtual std::shared_ptr<GPUMemory> sub(size_t O, size_t N) {
		ASSERT(O + N <= memory.size());
		return std::make_shared<ImmediateGPUMemory>(std::vector<uint8_t>(memory.begin() + O, memory.begin() + O + N));
	}
};

struct DelayedGPUMemory : public GPUMemory {
	GPUMemoryManager * that;
	size_t o, s, clear_id;
	DelayedGPUMemory(GPUMemoryManager * m, size_t o, size_t s, size_t i) :that(m), o(o), s(s), clear_id(i) {}
	virtual const void * data() const final { if (!that || clear_id != that->clear_id_ || o + s > that->cpu_memory_.size()) return 0; return that->cpu_memory_.data() + o; }
	virtual size_t size() const final { if (!that || clear_id != that->clear_id_ || o + s > that->cpu_memory_.size()) return 0; return s; }
	virtual std::shared_ptr<GPUMemory> sub(size_t O, size_t N) {
		LOG(ERR) << "Sub not supported for DelatedGPUMemory, this can cause serious issues if misused";
		ASSERT(O + N <= s);
		return std::make_shared<DelayedGPUMemory>(that, o + O, N, clear_id);
	}
};
//
//
//class __declspec(uuid("05ab9669-5951-45b5-bb0a-0051992e5c9d")) CachedBuffer: public ID3D11Buffer {
//protected:
//	ID3D11Buffer * b;
//public:
//	CachedBuffer(ID3D11Buffer * b) :b(b) {}
//	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) {
//		return b->QueryInterface(riid, ppvObject);
//	}
//	virtual ULONG STDMETHODCALLTYPE AddRef(void) {
//		return b->AddRef();
//	}
//	virtual ULONG STDMETHODCALLTYPE Release(void) {
//		ULONG r = b->Release();
//		if (!r) delete this;
//	}
//	virtual void STDMETHODCALLTYPE GetDesc(_Out_  D3D11_BUFFER_DESC *pDesc) {
//		b->GetDesc(pDesc);
//	}
//	virtual void STDMETHODCALLTYPE GetType(_Out_  D3D11_RESOURCE_DIMENSION *pResourceDimension) {
//		b->GetType(pResourceDimension);
//	}
//	virtual void STDMETHODCALLTYPE SetEvictionPriority(_In_  UINT EvictionPriority) {
//		b->SetEvictionPriority(EvictionPriority);
//	}
//	virtual UINT STDMETHODCALLTYPE GetEvictionPriority(void) {
//		return b->GetEvictionPriority();
//	}
//	virtual void STDMETHODCALLTYPE GetDevice(_Outptr_  ID3D11Device **ppDevice) {
//		b->GetDevice(ppDevice);
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
//		/* [annotation] */
//		_In_  REFGUID guid,
//		/* [annotation] */
//		_Inout_  UINT *pDataSize,
//		/* [annotation] */
//		_Out_writes_bytes_opt_(*pDataSize)  void *pData) = 0;
//
//	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
//		/* [annotation] */
//		_In_  REFGUID guid,
//		/* [annotation] */
//		_In_  UINT DataSize,
//		/* [annotation] */
//		_In_reads_bytes_opt_(DataSize)  const void *pData) = 0;
//
//	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
//		/* [annotation] */
//		_In_  REFGUID guid,
//		/* [annotation] */
//		_In_opt_  const IUnknown *pData) = 0;
//
//
//
//	std::vector<uint8_t> data;
//	D3D11_MAPPED_SUBRESOURCE * p_mapped_resource = nullptr;
//	void * original_data = nullptr;
//	void map(D3D11_MAPPED_SUBRESOURCE * pMappedResource) {
//		p_mapped_resource = pMappedResource;
//		//LOG(INFO) << "MSR " << data.size() << " " << p_mapped_resource->RowPitch << " " << p_mapped_resource->DepthPitch;
//		if (data.size() < p_mapped_resource->RowPitch)
//			data.resize(p_mapped_resource->RowPitch, 0);
//		if (data.size() < p_mapped_resource->DepthPitch)
//			data.resize(p_mapped_resource->DepthPitch, 0);
//		original_data = p_mapped_resource->pData;
//		p_mapped_resource->pData = data.data();
//
//		//LOG(INFO) << "MP " << write_data.size() << " " << data.size() << " " << p_mapped_resource->RowPitch << " " << p_mapped_resource->DepthPitch << " " << original_data;
//	}
//	void unmap() {
//		if (p_mapped_resource && original_data) {
//			const uint8_t * wdata = data.data();
//			uint8_t * mdata = (uint8_t*)original_data;
//			memcpy(mdata, wdata, data.size());
//			p_mapped_resource = nullptr;
//			original_data = nullptr;
//		}
//	}
//	void clear() {
//		p_mapped_resource = nullptr;
//		original_data = nullptr;
//	}
//	std::shared_ptr<GPUMemory> read(const std::vector<size_t>& offset, const std::vector<size_t>& n) {
//		if (offset.size() == 1 && offset[0] + n[0] <= data.size()) return std::make_shared<ReferenceMemory>(data.data() + offset[0], n[0]);
//		auto r = std::make_shared<ImmediateGPUMemory>();
//		for (size_t i = 0; i < offset.size(); i++) {
//			if (offset[i] + n[i] > data.size())
//				return std::shared_ptr<GPUMemory>();
//			r->memory.insert(r->memory.end(), data.cbegin() + offset[i], data.cbegin() + offset[i] + n[i]);
//		}
//		return r;
//	}
//};

struct CachedBuffer {
	D3D11_MAPPED_SUBRESOURCE * p_mapped_resource = nullptr;
	void * original_data = nullptr;
	std::vector<uint8_t> data;
	CachedBuffer(size_t s = 0) :data(s, 0) {}
	void map(D3D11_MAPPED_SUBRESOURCE * pMappedResource) {
		p_mapped_resource = pMappedResource;
		//LOG(INFO) << "MSR " << data.size() << " " << p_mapped_resource->RowPitch << " " << p_mapped_resource->DepthPitch;
		if (data.size() < p_mapped_resource->RowPitch)
			data.resize(p_mapped_resource->RowPitch, 0);
		if (data.size() < p_mapped_resource->DepthPitch)
			data.resize(p_mapped_resource->DepthPitch, 0);
		original_data = p_mapped_resource->pData;
		p_mapped_resource->pData = data.data();

		//LOG(INFO) << "MP " << write_data.size() << " " << data.size() << " " << p_mapped_resource->RowPitch << " " << p_mapped_resource->DepthPitch << " " << original_data;
	}
	void unmap() {
		if (p_mapped_resource && original_data) {
			const uint8_t * wdata = data.data();
			uint8_t * mdata = (uint8_t*)original_data;
			memcpy(mdata, wdata, data.size());
			p_mapped_resource = nullptr;
			original_data = nullptr;
		}
	}
	void clear() {
		p_mapped_resource = nullptr;
		original_data = nullptr;
	}
	std::shared_ptr<GPUMemory> read(const std::vector<size_t>& offset, const std::vector<size_t>& n) {
		if (offset.size() == 1 && offset[0] + n[0] <= data.size()) return std::make_shared<ReferenceMemory>(data.data() + offset[0], n[0]);
		auto r = std::make_shared<ImmediateGPUMemory>();
		for (size_t i = 0; i < offset.size(); i++) {
			if (offset[i] + n[i] > data.size())
				return std::shared_ptr<GPUMemory>();
			r->memory.insert(r->memory.end(), data.cbegin() + offset[i], data.cbegin() + offset[i] + n[i]);
		}
		return r;
	}
};

class __declspec(uuid("05ab9669-5951-45b5-bb0a-0051992e5c9d")) BufferExpiration : public IUnknown {
protected:
	std::shared_ptr<std::mutex> mutex;
	std::shared_ptr<std::unordered_map<ID3D11Buffer *, std::shared_ptr<CachedBuffer> > > cache;
	ID3D11Buffer * buf;
	ULONG ref = 1;
public:
	BufferExpiration(std::shared_ptr<std::unordered_map<ID3D11Buffer *, std::shared_ptr<CachedBuffer> > > c, std::shared_ptr<std::mutex> m, ID3D11Buffer * buf) :cache(c), mutex(m), buf(buf) {}
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override {
		return S_FALSE;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override {
		return ++ref;
	}
	virtual ULONG STDMETHODCALLTYPE Release(void) override {
		if (ref-- == 0) {
			std::lock_guard<std::mutex> lock(*mutex);
			cache->erase(buf);
			delete this;
		}
		return ref;
	}
};

struct GPUMemoryManager::BufferCache {
	// Let IUnknown erase objects
	std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();

	// The main cache
	std::shared_ptr<std::unordered_map<ID3D11Buffer *, std::shared_ptr<CachedBuffer> > > cache = std::make_shared<std::unordered_map<ID3D11Buffer *, std::shared_ptr<CachedBuffer> > >();

	// Fetching objects
	std::shared_ptr<CachedBuffer> get(ID3D11Buffer * r) {
		std::lock_guard<std::mutex> lock(*mutex);
		auto i = cache->find(r);
		if (i != cache->end())
			return i->second;
		return std::shared_ptr<CachedBuffer>();
	}
	void map(ID3D11Buffer * b, D3D11_MAPPED_SUBRESOURCE * pMappedResource) {
		auto i = get(b);
		if(i) i->map(pMappedResource);
	}
	void unmap(ID3D11Buffer * b) {
		auto i = get(b);
		if (i) i->unmap();
	}
	bool has(ID3D11Buffer * b) {
		return !!get(b);
	}
	void add(ID3D11Buffer * b) {
		// Let the object die with the buffer
		BufferExpiration * ex = new BufferExpiration(cache, mutex, b);
		b->SetPrivateDataInterface(__uuidof(BufferExpiration), ex);
		ex->Release();

		// Add the element to the cache
		D3D11_BUFFER_DESC d;
		b->GetDesc(&d);
		std::lock_guard<std::mutex> lock(*mutex);
		(*cache)[b] = std::make_shared<CachedBuffer>(d.ByteWidth);
	}
	void remove(ID3D11Buffer * b) {
		std::lock_guard<std::mutex> lock(*mutex);
		if (cache->count(b)) {
			cache->erase(b);
			// Remote the expiration object
			b->SetPrivateDataInterface(__uuidof(BufferExpiration), nullptr);
		}
	}
	std::shared_ptr<GPUMemory> read(ID3D11Buffer * b, const std::vector<size_t>& offset, const std::vector<size_t>& n) {
		auto i = get(b);
		if (i) return i->read(offset, n);
		return std::shared_ptr<GPUMemory>();
	}
};

static const D3D11_BUFFER_DESC fetch_buffer_desc = { 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ };
GPUMemoryManager::GPUMemoryManager(D3D11Hook * h) :h_(h), buf_(h, fetch_buffer_desc), fetch_buf_(h, fetch_buffer_desc) {
	cache_ = std::make_shared<BufferCache>();
}

size_t GPUMemoryManager::fetch(D3DBuffer * b, size_t n, std::vector<uint8_t>* r) {
	if (!b || !*b || !r) return 0;
	r->resize(n);
	return b->read(r->data(), n);
}

std::shared_ptr<GPUMemory> GPUMemoryManager::read(ID3D11Buffer * r, const std::vector<size_t>& offset, const std::vector<size_t>& n, bool immediate) {
	D3D11_BUFFER_DESC desc;
	r->GetDesc(&desc);
	size_t tn = 0;
	for (size_t i = 0; i < offset.size(); i++) {
		tn += n[i];
		if (offset[i] + n[i] > desc.ByteWidth) return std::shared_ptr<GPUMemory>();
	}
	std::shared_ptr<GPUMemory> cr = cache_->read(r, offset, n);
	if (cr) return cr;

	D3DBuffer * buf = nullptr;
	size_t o = 0;
	if (immediate) {
		//LOG(INFO) << "Using immediate buffers is inefficient";
		fetch_buf_.resize(tn);
		buf = &fetch_buf_;
		o = 0;
	} else {
		buf_.resize(current_size_ + tn);
		buf = &buf_;
		o = current_size_;
	}
	for (size_t i = 0; i < offset.size(); i++) {
		D3D11_BOX bx = { (UINT)offset[i], 0, 0, (UINT)(offset[i] + n[i]), 1, 1 };
		h_->D3D11Hook::CopySubresourceRegion(*buf, 0, (UINT)o, 0, 0, r, 0, &bx);
		o += n[i];
	}

	if (immediate) {
		std::shared_ptr<ImmediateGPUMemory> r = std::make_shared<ImmediateGPUMemory>(tn);
		if (!fetch(buf, tn, &r->memory)) return std::shared_ptr<GPUMemory>();
		return r;
	} else {
		auto r = std::make_shared<DelayedGPUMemory>(this, current_size_, tn, clear_id_);
		current_size_ += tn;
		current_delayed_memory_.push_back(r);
		return r;
	}
}

void GPUMemoryManager::fetchDelayed() {
	if (buf_ && current_size_)
		fetch(&buf_, current_size_, &cpu_memory_);
}

void GPUMemoryManager::clear() {
	clear_id_++;
	current_size_ = 0;
	cpu_memory_.clear();
	// Reset the current delated memory pointers
	for (auto d : current_delayed_memory_)
		d->that = nullptr;
	current_delayed_memory_.clear();
}

void GPUMemoryManager::cacheBuffer(ID3D11Buffer * buffer) {
	cache_->add(buffer);
}
void GPUMemoryManager::uncacheBuffer(ID3D11Buffer * buffer) {
	cache_->remove(buffer);
}

void GPUMemoryManager::map(ID3D11Resource * r, D3D11_MAPPED_SUBRESOURCE * pMappedResource) {
	D3D11_RESOURCE_DIMENSION type;
	r->GetType(&type);
	if (type == D3D11_RESOURCE_DIMENSION_BUFFER)
		cache_->map((ID3D11Buffer*)r, pMappedResource);
}

void GPUMemoryManager::unmap(ID3D11Resource * r) {
	D3D11_RESOURCE_DIMENSION type;
	r->GetType(&type);
	if (type == D3D11_RESOURCE_DIMENSION_BUFFER)
		cache_->unmap((ID3D11Buffer*)r);
}

D3DTexture2D::D3DTexture2D(D3D11Hook * h, const D3D11_TEXTURE2D_DESC & d): h_(h), desc_(d) {
}

D3DTexture2D::~D3DTexture2D() {
	if (tex_) tex_->Release();
}

DataType D3DTexture2D::type() const {
	return dataType(desc_.Format);
}

int D3DTexture2D::channels() const {
	return ::channels(desc_.Format);
}

DXGI_FORMAT D3DTexture2D::format() const {
	return desc_.Format;
}

bool D3DTexture2D::setup(uint32_t W, uint32_t H, DXGI_FORMAT format, uint32_t mip_level, D3D11_USAGE usage, uint32_t bind_flags, uint32_t cpu_access, uint32_t misc_flags) {
	if (!tex_ || desc_.Width != W || desc_.Height != H || desc_.Format != format || desc_.MipLevels != mip_level || desc_.Usage != usage || desc_.BindFlags != bind_flags || desc_.CPUAccessFlags != cpu_access || desc_.MiscFlags  != misc_flags) {
		if (tex_) {
			LOG(ERR) << "Texture format and size should not change after creation! Things might go wrong!";
			tex_->Release();
		}
		desc_ = { W, H, mip_level, 1, format, {1,0}, usage, bind_flags, cpu_access, misc_flags };
		HRESULT hr = h_->D3D11Hook::CreateTexture2D(&desc_, nullptr, &tex_);
		if (FAILED(hr)) {
			LOG(ERR) << "Failed to create texture hr = " << std::hex << hr;
			return false;
		}
		return true;
	}
	return false;
}

RWTexture2D::RWTexture2D(D3D11Hook * h) : D3DTexture2D(h) {
}

RWTexture2D::~RWTexture2D() {
	if (uav_) uav_->Release();
	if (rtv_) rtv_->Release();
}

bool RWTexture2D::setup(uint32_t W, uint32_t H, DXGI_FORMAT format, uint32_t mip_level, D3D11_USAGE usage, uint32_t bind_flags, uint32_t cpu_access, uint32_t misc_flags) {
	if (D3DTexture2D::setup(W, H, format, mip_level, usage, bind_flags | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET, cpu_access, misc_flags)) {
		if (uav_) uav_->Release();
		if (rtv_) rtv_->Release();
		HRESULT hr = h_->D3D11Hook::CreateUnorderedAccessView(tex_, nullptr, &uav_);
		if (FAILED(hr)) {
			LOG(ERR) << "Failed to CreateUnorderedAccessView hr = " << std::hex << hr;
			return false;
		}
		hr = h_->D3D11Hook::CreateRenderTargetView(tex_, nullptr, &rtv_);
		if (FAILED(hr)) {
			LOG(ERR) << "Failed to CreateRenderTargetView hr = " << std::hex << hr;
			return false;
		}
		return true;
	}
	return false;
}

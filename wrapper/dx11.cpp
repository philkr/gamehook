#include "dx11.h"
#include "dx11/d3d11.h"
#include "dxgi/dxgi.h"
#include "dxgi/dxgiswapchain.h"
#include "io/io.h"
#include "hook.h"
#include "util.h"
#include "resource.h"
#include "shader.h"
#include "speed.h"
#include "maingamecontroller.h"
#include "rendertarget.h"
#include "config/config.h"
#include "cbuffer.h"
#include <unordered_map>
#include <unordered_set>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#pragma warning( disable : 4250 ) // Domiance warnings

//#define _DEBUG
//#define TIMING

#ifdef TIMING
// Note: Timing is slow in QEMU
#define TIC __int64 __tic__ = timer[0].tic();
#define TOC timer[current_recording_type].toc(__FUNCTION__,__tic__);
#define PRINT_TIMER {++timer[current_recording_type]; for(int i=0; i<4; i++) if ((int)timer[i] >= 10) { LOG(INFO) << "Timer[ " << i << " ]\n" << timer[i]; timer[i].clear(); }}
static Timer timer[4];
#else
#define TIC
#define TOC
#define PRINT_TIMER
#endif

template<typename O, typename T> O makeIndex(size_t i, const T & t) {
	return O{ (uint32_t)i };
}
template<typename O> O makeIndex(size_t i, ID3D11RenderTargetView * const & t) {
	O r = { (uint32_t)i };

	// Get the texture dimension
	ID3D11Resource * tr;
	t->GetResource(&tr);
	D3D11_RESOURCE_DIMENSION dim;
	tr->GetType(&dim);
	if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		D3D11_TEXTURE2D_DESC tdesc;
		((ID3D11Texture2D*)tr)->GetDesc(&tdesc);
		r.W = tdesc.Width;
		r.H = tdesc.Height;
	}
	tr->Release();
	return r;
}

template<typename T, typename O> struct IndexHandler {
	std::unordered_map<T, O> index_map;
	std::vector<T> reverse_id;
	O operator()(const T & t) {
		if (!t) return 0;
		auto i = index_map.find(t);
		if (i == index_map.end()) {
			reverse_id.push_back(t);
			i = index_map.insert(std::make_pair(t, makeIndex<O>(reverse_id.size(), t))).first;
		}
		return i->second;
	}
	T operator()(O id) {
		if (0 < id.id && id.id <= reverse_id.size())
			return reverse_id[id.id-1];
		return 0;
	}
	void operator()(std::vector<O> & v, const T * t, size_t n, size_t offset) {
		if (offset + n > v.size())
			v.resize(offset + n);
		for (size_t i = 0; i < n; i++)
			v[offset + i] = (*this)(t[i]);
	}
	std::vector<O> operator()(const T * t, size_t n) {
		if (!t) return std::vector<O>();
		std::vector<O> r(n);
		(*this)(r, t, n, 0);
		return r;
	}
	std::vector<O> operator()(const std::vector<T> & t) {
		return (*this)(t.data(), t.size());
	}
	std::vector<T> operator()(const std::vector<O> & id) {
		std::vector<T> r;
		r.reserve(id.size());
		for (const auto & i : id)
			r.push_back((*this)(i));
		return r;
	}
	void update(const T & t) {
		current = -1;
		if (t)
			current = (*this)(t);
	}
};

// The GameHook implementation is split into multiple base classes (to make the code more managable)

// GameHookIO does all the IO handling
struct GameHookIO : virtual public IOHookHigh, virtual public MainGameController {
	// IOHookHigh -> GameController
	virtual bool handleKeyDown(unsigned char key, unsigned char special_status) {
		return onKeyDown(key, special_status);
	}
	virtual bool handleKeyUp(unsigned char key) {
		return onKeyUp(key);
	}
	// GameController -> IOHookHigh
	virtual void keyDown(unsigned char key) {
		IOHookHigh::sendKeyDown(key);
	}
	virtual void keyUp(unsigned char key) {
		IOHookHigh::sendKeyUp(key);
	}
	virtual const std::vector<uint8_t> & keyState() const {
		return key_state;
	}
	virtual void mouseDown(float x, float y, uint8_t button) {
		IOHookHigh::sendMouseDown(uint16_t((x + 1) / 2 * (defaultWidth() - 1)), uint16_t((1 - y) / 2 * (defaultHeight() - 1)), button);
	}
	virtual void mouseUp(float x, float y, uint8_t button) {
		IOHookHigh::sendMouseUp(uint16_t((x + 1) / 2 * (defaultWidth() - 1)), uint16_t((1 - y) / 2 * (defaultHeight() - 1)), button);
	}
};

// GameHookBuffer does all the CBuffer, vertex and index buffer memory management
struct GameHookBuffer : virtual public D3D11Hook, virtual public MainGameController {
	GPUMemoryManager memory_manager;
	IndexHandler<ID3D11Buffer*, Buffer> buffer_id;
	BufferInfo * buffer_info;
	std::unordered_map<std::string, std::shared_ptr<CBufferImp>> bound_cbuffers;

	GameHookBuffer(BufferInfo * buffer_info) :memory_manager(this), buffer_info(buffer_info) {
		ASSERT(buffer_info != NULL);
	}

	virtual HRESULT Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource) override {
		TIC;
		HRESULT hr = D3D11Hook::Map(pResource, Subresource, MapType, MapFlags, pMappedResource);
		if (Subresource == 0 && MapType != D3D11_MAP_READ) {
			memory_manager.map(pResource, pMappedResource);
		}
		TOC;
		return hr;
	}
	virtual void Unmap(ID3D11Resource *pResource, UINT Subresource) override {
		TIC;
		if (Subresource == 0)
			memory_manager.unmap(pResource);
		D3D11Hook::Unmap(pResource, Subresource);
		TOC;
	}

	virtual HRESULT CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer) {
		TIC;
		HRESULT hr = D3D11Hook::CreateBuffer(pDesc, pInitialData, ppBuffer);
		if (SUCCEEDED(hr) && ppBuffer && *ppBuffer && pDesc && pDesc->BindFlags | D3D11_BIND_CONSTANT_BUFFER && pDesc->ByteWidth <= 256)
			memory_manager.cacheBuffer(*ppBuffer);
		TOC;
		return hr;
	}

	virtual std::shared_ptr<GPUMemory> readBuffer(Buffer b, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate) {
		TIC;
		std::shared_ptr<GPUMemory> r;
		ID3D11Buffer * bf = buffer_id(b);
		if (bf)
			r = memory_manager.read(bf, offset, n, immediate);
		TOC;
		return r;
	}
	virtual size_t bufferSize(Buffer b) {
		ID3D11Buffer * bf = buffer_id(b);
		D3D11_BUFFER_DESC d;
		bf->GetDesc(&d);
		return d.ByteWidth;
	}
	virtual void IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets) {
		if (StartSlot == 0)
			buffer_info->vertex = buffer_id(ppVertexBuffers[0]);
		D3D11Hook::IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
	}
	virtual void IASetIndexBuffer(ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset) {
		buffer_info->index = buffer_id(pIndexBuffer);
		D3D11Hook::IASetIndexBuffer(pIndexBuffer, Format, Offset);
	}
	virtual void PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
		buffer_id(buffer_info->pixel_constant, ppConstantBuffers, NumBuffers, StartSlot);
		D3D11Hook::PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
	}
	virtual void PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
		LOG(ERR) << "Cannot intercept 'PSSetConstantBuffers1'";
		D3D11Hook::PSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
	}
	virtual void VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
		buffer_id(buffer_info->vertex_constant, ppConstantBuffers, NumBuffers, StartSlot);
		D3D11Hook::VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
	}
	virtual void VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
		LOG(ERR) << "Cannot intercept 'VSSetConstantBuffers1'";
		D3D11Hook::VSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
	}

	virtual std::shared_ptr<CBuffer> createCBuffer(const std::string & name, size_t max_size) override {
		auto r = std::make_shared<CBufferImp>(name, max_size, this);
		memory_manager.uncacheBuffer(r->get());
		return r;
	}
	virtual void bindCBuffer(std::shared_ptr<CBuffer> b) {
		std::shared_ptr<CBufferImp> bi = std::dynamic_pointer_cast<CBufferImp>(b);
		if (bi) bound_cbuffers[bi->name] = bi;
	}
};

// GameHookShader does all the shader management
struct GameHookShader : virtual public D3D11Hook, virtual public MainGameController {
public: // Types
	typedef std::unordered_map<std::string, uint32_t> RegInfo;
	template<typename DX_S> struct CurrentShaderInfo {
		DX_S * dx_shader = nullptr;
		std::vector<ID3D11ClassInstance*> class_instances;
		std::shared_ptr<Shader> shader;
		// Update the DX part of the shader info
		void update(DX_S * s, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
			if (dx_shader) dx_shader->Release();
			dx_shader = s;
			if (dx_shader) dx_shader->AddRef();
			for (auto c : class_instances) if (c) c->Release();
			class_instances.clear();
			class_instances.insert(class_instances.begin(), ppClassInstances, ppClassInstances + NumClassInstances);
			for (auto c : class_instances) if (c) c->AddRef();
		}
	};
	template<typename DX_S> struct CustomShaderInfo {
		std::shared_ptr<Shader> shader;
		DX_S * dx_shader_;
		RegInfo uav, rtv;
	};
public: // Variables
	IndexHandler<ID3D11ShaderResourceView*, Texture2D> srv_id;
	std::unordered_map<ID3D11PixelShader*, std::shared_ptr<Shader> > pixel_shader_;
	std::unordered_map<ID3D11VertexShader*, std::shared_ptr<Shader> > vertex_shader_;

	std::unordered_map<ShaderHash, std::shared_ptr<CustomShaderInfo<ID3D11PixelShader> > > custom_pixel_shader_;
	std::unordered_map<ShaderHash, std::shared_ptr<CustomShaderInfo<ID3D11VertexShader> > > custom_vertex_shader_;

	CurrentShaderInfo<ID3D11PixelShader> current_pixel_shader_;
	CurrentShaderInfo<ID3D11VertexShader> current_vertex_shader_;

	std::shared_ptr<CustomShaderInfo<ID3D11PixelShader> > current_custom_pixel_shader_;
	std::shared_ptr<CustomShaderInfo<ID3D11VertexShader> > current_custom_vertex_shader_;

	std::unordered_set<std::string> custom_render_target_set_;

	ShaderInfo * shader_info;

	bool pixel_shader_updated_ = false, vertex_shader_updated_ = false;
public: // D3D11Hook functions
	GameHookShader(ShaderInfo * shader_info) :shader_info(shader_info) {
		ASSERT(shader_info != NULL);
	}
	virtual void initController(std::shared_ptr<GameController> c) override {
		// Fetch a list of all custom render targets (used for auto-binding of PS outputs)
		for (auto t : providedCustomTargets(c))
			custom_render_target_set_.insert(t.name);
	}
	virtual void startControllers() override {
		// Feed a new controller some shaders
		MainGameController::startControllers();
		for (auto s : pixel_shader_) onCreateShader(s.second);
		for (auto s : vertex_shader_) onCreateShader(s.second);
	}

	virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader) {
		TIC;
		HRESULT r = D3D11Hook::CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
		if (SUCCEEDED(r)) {
			auto shader = vertex_shader_[*ppVertexShader] = std::make_shared<VertexShader>(ByteCode((const char *)pShaderBytecode, (const char *)pShaderBytecode + BytecodeLength));
			onCreateShader(shader);
		}
		TOC;
		return r;
	}
	virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader) {
		TIC;
		HRESULT r = D3D11Hook::CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
		if (SUCCEEDED(r)) {
			auto shader = pixel_shader_[*ppPixelShader] = std::make_shared<PixelShader>(ByteCode((const char *)pShaderBytecode, (const char *)pShaderBytecode + BytecodeLength));
			onCreateShader(shader);
		}
		TOC;
		return r;
	}

	virtual void PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
		// Unbind the current custom shader
		unbindShader(Shader::PIXEL);

		// Update the DX portion of the current shader
		current_pixel_shader_.update(pPixelShader, ppClassInstances, NumClassInstances);

		// See if we know the shader (should always be true)
		auto i = pixel_shader_.find(pPixelShader);
		if (i != pixel_shader_.end()) {
			current_pixel_shader_.shader = i->second;
			shader_info->pixel = i->second->hash();
			onBindShader(i->second);
		} else {
			shader_info->pixel = ShaderHash();
			current_pixel_shader_.shader.reset();
		}
		if (!current_custom_pixel_shader_) {
			pixel_shader_updated_ = true;
			D3D11Hook::PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);
		}
	}
	virtual void VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
		// Unbind the current custom shader
		unbindShader(Shader::VERTEX);

		// Update the DX portion of the current shader
		current_vertex_shader_.update(pVertexShader, ppClassInstances, NumClassInstances);

		// See if we know the shader (should always be true)
		auto i = vertex_shader_.find(pVertexShader);
		if (i != vertex_shader_.end()) {
			current_vertex_shader_.shader = i->second;
			shader_info->vertex = i->second->hash();
			onBindShader(i->second);
		} else {
			current_vertex_shader_.shader.reset();
			shader_info->vertex = ShaderHash();
		}

		if (!current_custom_vertex_shader_) {
			vertex_shader_updated_ = true;
			D3D11Hook::VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);
		}
	}
	RegInfo uavMap(std::shared_ptr<Shader> shader) const {
		RegInfo r;
		for (const auto & s : shader->sbuffers())
			if (custom_render_target_set_.count(s.name))
				r[s.name] = s.bind_point;
		return r;
	}
	RegInfo rtvMap(std::shared_ptr<Shader> shader) const {
		RegInfo r;
		for (const auto & s : shader->outputs()) {
			std::string name = s.name;
			if (custom_render_target_set_.count(name))
				r[name] = s.bind_point;
			name = name.substr(0, name.size() - 1);
			if (custom_render_target_set_.count(name))
				r[name] = s.bind_point;
		}
		return r;
	}
	virtual void buildShader(std::shared_ptr<Shader> shader) final {
		if (shader->type() == Shader::PIXEL && !custom_pixel_shader_.count(shader->hash())) {
			RegInfo uav = uavMap(shader), rtv = rtvMap(shader);
			if (!uav.size() && !rtv.size()) {
				LOG(WARN) << "Shader does not contain any outputs! Shader not built!";
				return;
			}
			ID3D11PixelShader * s = nullptr;
			HRESULT hr = D3D11Hook::CreatePixelShader(shader->data(), shader->size(), NULL, &s);
			if (FAILED(hr)) {
				LOG(WARN) << "Failed to build shader " << shader->hash() << " : " << std::hex << hr << std::dec;
				LOG(INFO) << shader->disassemble();
			}
			else {
				custom_pixel_shader_[shader->hash()] = std::shared_ptr<CustomShaderInfo<ID3D11PixelShader> >(new CustomShaderInfo<ID3D11PixelShader>{ shader, s, uav, rtv });
			}
		}
		if (shader->type() == Shader::VERTEX && !custom_vertex_shader_.count(shader->hash())) {
			ID3D11VertexShader * s = nullptr;
			HRESULT hr = D3D11Hook::CreateVertexShader(shader->data(), shader->size(), NULL, &s);
			if (FAILED(hr)) {
				LOG(WARN) << "Failed to build shader " << shader->hash() << " : " << std::hex << hr << std::dec;
				LOG(INFO) << shader->disassemble();
			} else {
				custom_vertex_shader_[shader->hash()] = std::shared_ptr<CustomShaderInfo<ID3D11VertexShader> >(new CustomShaderInfo<ID3D11VertexShader>{ shader, s });
			}
		}
	}

	virtual void bindShader(std::shared_ptr<Shader> shader) final {
		buildShader(shader);
		if (shader->type() == Shader::PIXEL) {
			auto it = custom_pixel_shader_.find(shader->hash());
			if (it != custom_pixel_shader_.end()) {
				current_custom_pixel_shader_ = it->second;
				D3D11Hook::PSSetShader(it->second->dx_shader_, current_pixel_shader_.class_instances.data(), (UINT)current_pixel_shader_.class_instances.size());
				pixel_shader_updated_ = true;
			}
		}
		if (shader->type() == Shader::VERTEX) {
			auto it = custom_vertex_shader_.find(shader->hash());
			if (it != custom_vertex_shader_.end()) {
				current_custom_vertex_shader_ = it->second;
				D3D11Hook::VSSetShader(it->second->dx_shader_, current_vertex_shader_.class_instances.data(), (UINT)current_vertex_shader_.class_instances.size());
				vertex_shader_updated_ = true;
			}
		}
	}

	virtual void unbindShader(Shader::Type type) final {
		if (type == Shader::PIXEL && current_custom_pixel_shader_) {
			current_custom_pixel_shader_.reset();
			// Bind the original pixel shader
			D3D11Hook::PSSetShader(current_pixel_shader_.dx_shader, current_pixel_shader_.class_instances.data(), (UINT)current_pixel_shader_.class_instances.size());
		}
		if (type == Shader::VERTEX && current_custom_vertex_shader_) {
			current_custom_vertex_shader_.reset();
			// Bind the original vertex shader
			D3D11Hook::VSSetShader(current_vertex_shader_.dx_shader, current_vertex_shader_.class_instances.data(), (UINT)current_vertex_shader_.class_instances.size());
		}
	}

	virtual void PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
		if (StartSlot == 0)
			shader_info->ps_texture_id = srv_id(ppShaderResourceViews[0]);
		D3D11Hook::PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
	}
};


// GameHookRenderTarget manages and copies all the render targets
struct GameHookRenderTarget : virtual public D3D11Hook, virtual public MainGameController {
public: // Data
	// Render targets
	std::unordered_map<std::string, std::shared_ptr<BaseRenderTarget> > targets_, visible_targets_;
	std::unordered_map<std::string, std::shared_ptr<RWTextureTarget> > custom_render_targets_;
	IndexHandler<ID3D11DepthStencilView*, DepthStencilView> dsv_id;
	IndexHandler<ID3D11RenderTargetView*, RenderTargetView> rtv_id;
	ID3D11DepthStencilView * current_DepthStencilView = nullptr;
	std::vector<ID3D11RenderTargetView *> current_RenderTargetViews;
	std::vector<ID3D11UnorderedAccessView *> current_UnorderedAccessViews;

	RenderTargetInfo * render_target_info;

	bool render_targets_updated_ = false;
public: // Implementation
	GameHookRenderTarget(RenderTargetInfo * render_target_info): render_target_info(render_target_info) {
		ASSERT(render_target_info != NULL);
	}

	virtual void OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView) {
		TIC;
		render_target_info->outputs = rtv_id(ppRenderTargetViews, NumViews);
		render_target_info->depth = dsv_id(pDepthStencilView);
		current_RenderTargetViews = std::vector<ID3D11RenderTargetView *>(ppRenderTargetViews, ppRenderTargetViews + NumViews);
		current_UnorderedAccessViews.clear();
		current_DepthStencilView = pDepthStencilView;
		D3D11Hook::OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
		render_targets_updated_ = true;
		TOC;
	}

	virtual void OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) {
		TIC;
		if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL) {
			render_target_info->outputs = rtv_id(ppRenderTargetViews, NumRTVs);
			render_target_info->depth = dsv_id(pDepthStencilView);
			current_RenderTargetViews = std::vector<ID3D11RenderTargetView *>(ppRenderTargetViews, ppRenderTargetViews + NumRTVs);
			current_DepthStencilView = pDepthStencilView;
		}
		if (NumUAVs != D3D11_KEEP_UNORDERED_ACCESS_VIEWS)
			current_UnorderedAccessViews = std::vector<ID3D11UnorderedAccessView *>(ppUnorderedAccessViews, ppUnorderedAccessViews + NumUAVs);
		D3D11Hook::OMSetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
		render_targets_updated_ = true;
		TOC;
	}

	virtual void initController(std::shared_ptr<GameController> c) override {
		// Fetch all valid render targets
		const auto & targets = providedTargets(c);
		for (const auto & t : targets)
			if (!targets_.count(t.name)) {
				targets_[t.name] = std::make_shared<RenderTarget>(this);
				if (!t.hidden)
					visible_targets_[t.name] = targets_[t.name];
			}

		const auto & custom_targets = providedCustomTargets(c);
		for (const auto & t : custom_targets)
			if (!custom_render_targets_.count(t.name)) {
				if (targets_.count(t.name))
					LOG(WARN) << "Render target '" << t.name << "' already exists! Overwriting the old target...";
				targets_[t.name] = custom_render_targets_[t.name] = std::make_shared<RWTextureTarget>(this, (DXGI_FORMAT)t.type);
				if (!t.hidden)
					visible_targets_[t.name] = targets_[t.name];
			}
	}

	// Output handling
	virtual void copyTarget(const std::string & to, const std::string & from) final {
		auto j = targets_.find(from), i = targets_.find(to);
		ASSERT(i != targets_.end());
		ASSERT(j != targets_.end());
		if (!i->second) {
			// Create the target
			i->second = std::make_shared<RenderTarget>(this);
		}
		if (j->second)
			i->second->copyFrom(*j->second);

	}
	virtual void copyTarget(const std::string & name, const RenderTargetView & view_id) final {
		TIC;
		auto i = targets_.find(name);
		ASSERT(i != targets_.end());
		auto t = rtv_id(view_id);
		if (!i->second) {
			// Create the target
			i->second = std::make_shared<RenderTarget>(this);
		}
		i->second->copyFrom(t);
		TOC;
	}
	virtual bool hasTarget(const std::string & name) const {
		return visible_targets_.count(name);
	}
	virtual std::vector<std::string> listTargets() const {
		std::vector<std::string> r;
		for (auto & i : visible_targets_)
			r.push_back(i.first);
		return r;
	}
	virtual TargetType targetType(const std::string & name) const {
		auto i = targets_.find(name);
		if (i != targets_.end() && i->second)
			return i->second->format();
		return UNKNOWN;
	}
	virtual void requestOutput(const std::string & name) {
		requestOutput(name, defaultWidth(), defaultHeight());
	}
	virtual void requestOutput(const std::string & name, int W, int H) {
		auto t = targets_.find(name);
		if (t == targets_.end())
			LOG(WARN) << "Target '" << name << "' not found!";
		else
			t->second->addOutput(W, H);
	}

	// What type does a certain output channel have?
	virtual DataType outputType(const std::string & name) const {
		auto t = targets_.find(name);
		if (t != targets_.end())
			return t->second->type();
		LOG(WARN) << "Target '" << name << "' not found!";
		return DT_UNKNOWN;
	}
	virtual int outputChannels(const std::string & name) const {
		auto t = targets_.find(name);
		if (t != targets_.end())
			return t->second->channels();
		LOG(WARN) << "Target '" << name << "' not found!";
		return 0;
	}

	virtual bool targetAvailable(const std::string & name) const {
		auto t = targets_.find(name);
		if (t != targets_.end())
			return t->second->was_written;
		LOG(WARN) << "Target '" << name << "' not found!";
		return false;
	}

	virtual bool readTarget(const std::string & name, int W, int H, int C, DataType T, void * data) {
		auto t = targets_.find(name);
		if (t != targets_.end())
			return t->second->read(W, H, C, T, data);
		LOG(WARN) << "Target '" << name << "' not found!";
		return false;
	}
#define READ_TARGET(T) virtual bool readTarget(const std::string & name, int W, int H, int C, T * data) {\
		auto t = targets_.find(name);\
		if (t != targets_.end())\
			return t->second->read(W, H, C, data);\
		LOG(WARN) << "Target '" << name << "' not found!";\
		return false;\
	}
	DO_ALL_TYPE(READ_TARGET)
#undef READ_TARGET
};

struct GameHook : public GameHookIO, public GameHookBuffer, public GameHookRenderTarget, public GameHookShader {
public: /***  Global Data  ***/
	// Frame and drawing information
	uint32_t frame_count = 0, draw_count = 0;
	uint32_t width = 0, height = 0;
	// Recording information
	RecordingType current_recording_type = NONE, next_recording_type = NONE;

	DrawInfo draw_info;
public: /***  GameController  ***/
	GameHook() : GameHookBuffer(&draw_info.buffer), GameHookRenderTarget(&draw_info.target), GameHookShader(&draw_info.shader) {
		startControllers();
	}
	// Recording logic
	virtual RecordingType currentRecordingType() const {
		return current_recording_type;
	}
	virtual void recordNextFrame(RecordingType type) {
		next_recording_type = type;
	}
	virtual void initController(std::shared_ptr<GameController> c) override {
		GameHookRenderTarget::initController(c);
		GameHookShader::initController(c);
	}

public: /***  IO Handling  ***/
	bool reload_dlls = false;
	virtual bool onKeyDown(unsigned char key, unsigned char special_status) {
		// Reload keyboard shortcut (CTLR + F10)
		if (key == VK_F10 && (special_status & 7) == CTRL) {
			reload_dlls = true;
			return true;
		}
		return MainGameController::onKeyDown(key, special_status);
	}


public: // Main rendering loop
	virtual HRESULT Present(unsigned int SyncInterval, unsigned int Flags) override {
		// Present serves as the main control loop in GameHook

		TIC;
		if (!Flags) {
			// Get width and height of the main window (this might change over time)
			{
				DXGI_SWAP_CHAIN_DESC desc;
				GetDesc(&desc);
				width = desc.BufferDesc.Width;
				height = desc.BufferDesc.Height;
			}

			GetLastPresentCount(&frame_count);

			// Save the current frame
			if (frame_count > 0) {
				// Fetch all delayed buffers
				memory_manager.fetchDelayed();

				// Post process
				MainGameController::onPostProcess(frame_count);

				// Fetch all custom render targets
				for (const auto & t : custom_render_targets_)
					t.second->fetch();

				// Map all targets
				for (const auto & t : targets_)
					if (t.second)
						t.second->mapOutputs();

				MainGameController::onEndFrame(frame_count);

				// Unmap all targets
				for (const auto & t : targets_)
					if (t.second)
						t.second->unmapOutputs();

				// Reset buffers
				memory_manager.clear();
			}

			// Present the current frame
			HRESULT r = D3D11Hook::Present(SyncInterval, Flags);

			MainGameController::onPresent(frame_count);

			// Should we reload all plugins
			if (reload_dlls) {
				// Remove all the old controllers
				clearControllers();
				custom_render_targets_.clear();
				targets_.clear();
				visible_targets_.clear();

				// Reload the plugins
				reloadAPI(true, false);

				// Fire up the next controllers
				startControllers();
				reload_dlls = false;
			}

			// Setup all the custon render targets
			for (const auto & t : custom_render_targets_) {
				t.second->setup(width, height);
				UINT v[4] = { 0 };
				D3D11Hook::ClearUnorderedAccessViewUint(*t.second, v);
			}

			// Start next frame
			draw_count = 0;
			current_recording_type = next_recording_type;
			next_recording_type = NONE;
			TOC;
			PRINT_TIMER;
			TIC;
			MainGameController::onBeginFrame(frame_count + 1);

			TOC;
			return r;
		}
		TOC;
		return D3D11Hook::Present(SyncInterval, Flags);
	}


public: /***  PostFX  ***/
	struct PostFXShader {
		ID3D11PixelShader* shader = nullptr;
		std::unordered_map<std::string, uint32_t> output, inputs;
		std::vector<ShaderImp::Buffer> cbuffers;
	};
	std::unordered_map<ShaderHash, PostFXShader> postfx_shaders;
	std::shared_ptr<FXShader> fx_shader;

	virtual void callPostFx(std::shared_ptr<Shader> shader) {
		TIC;
		if (!postfx_shaders.count(shader->hash())) {
			// Compile the postprocessing shader
			PostFXShader r;
			HRESULT hr = D3D11Hook::CreatePixelShader(shader->data(), shader->size(), nullptr, &r.shader);
			if (FAILED(hr)) {
				LOG(ERR) << "Failed to create PostFX shader " << std::hex << hr;
			}
			else {
				r.cbuffers = shader->cbuffers();
				for (const auto & s : shader->textures()) {
					if (targets_.count(s.name))
						r.inputs[s.name] = s.bind_point;
				}
				for (const auto & s : shader->outputs())
					if (custom_render_targets_.count(s.name))
						r.output[s.name] = s.bind_point;
			}
			postfx_shaders[shader->hash()] = r;
		}
		PostFXShader info = postfx_shaders[shader->hash()];
		if (info.shader) {
			// Set the cbuffers
			for (const auto & c : info.cbuffers)
				if (bound_cbuffers.count(c.name)) {
					ID3D11Buffer * new_b = bound_cbuffers[c.name]->get();
					D3D11Hook::PSSetConstantBuffers(c.bind_point, 1, &new_b);
				}
			// Set the Inputs
			std::vector<ID3D11ShaderResourceView*> inputs;
			for (const auto & i : info.inputs) {
				size_t j = i.second;
				if (inputs.size() <= j)
					inputs.resize(j + 1, 0);
				if (targets_.count(i.first))
					inputs[j] = targets_[i.first]->rtv();
			}
			// Set the UAVs
			std::vector<ID3D11RenderTargetView*> outputs;
			for (const auto & i : info.output) {
				size_t j = i.second;
				if (outputs.size() <= j)
					outputs.resize(j + 1);
				outputs[j] = *custom_render_targets_[i.first];
			}
			if (!fx_shader) fx_shader = std::make_shared<FXShader>(this);
			fx_shader->call(defaultWidth(), defaultHeight(), inputs, outputs, info.shader);
		}
		TOC;
	}


public: /* Drawing calls */
	bool hide_draw = false;

	virtual void hideDraw(bool hide) {
		hide_draw = hide;
	}

	// Render state for injection
	ID3D11Buffer *old_vs_cbuffer[20] = { 0 }, *old_ps_cbuffer[20] = { 0 };
	UINT old_ps_nci = 0, old_vs_nci = 0;
	ID3D11ClassInstance * old_ps_ci[256], * old_vs_ci[256];
	void beginDraw(DrawInfo::Type type, uint16_t instances, uint32_t n, uint32_t start_index, uint32_t start_vertex, uint32_t start_instance) {
		TIC;

		// Call startDraw of all plugins
		draw_info.type = type;
		draw_info.instances = instances;
		draw_info.n = n;
		draw_info.start_index = start_index;
		draw_info.start_vertex = start_vertex;
		draw_info.start_instance = start_instance;

		// Reset the cbuffer bindings
		bound_cbuffers.clear();
		hide_draw = false;

		onBeginDraw(draw_info);

		if (hide_draw) return;

#ifdef TIMING
		timer[current_recording_type].toc("GameHook::beginDrawCall.1", __tic__);
		__tic__ = timer[0].tic();
#endif
		// Is a custom pixel shader bound?
		if (current_custom_pixel_shader_ && bound_cbuffers.size()) {
			// Let's feed it some cbuffers if needed
			for (const auto & b : current_custom_pixel_shader_->shader->cbuffers()) {
				const auto & i = bound_cbuffers.find(b.name);
				if (i != bound_cbuffers.end()) {
					// Bind the cbuffer
					D3D11Hook::PSGetConstantBuffers(b.bind_point, 1, old_ps_cbuffer + b.bind_point);
					ID3D11Buffer * new_b = i->second->get();
					D3D11Hook::PSSetConstantBuffers(b.bind_point, 1, &new_b);
				}
			}
		}

		// Is a custom vertex shader bound?
		if (current_custom_vertex_shader_ && bound_cbuffers.size()) {
			// Let's feed it some cbuffers if needed
			for (const auto & b : current_custom_vertex_shader_->shader->cbuffers()) {
				const auto & i = bound_cbuffers.find(b.name);
				if (i != bound_cbuffers.end()) {
					// Bind the cbuffer
					D3D11Hook::VSGetConstantBuffers(b.bind_point, 1, old_vs_cbuffer + b.bind_point);
					ID3D11Buffer * new_b = i->second->get();
					D3D11Hook::VSSetConstantBuffers(b.bind_point, 1, &new_b);
				}
			}
		}

		// Do we need to take care of new outputs?
		if (current_custom_pixel_shader_) {
			// I'm too lazy to think. To future self: use pixel_shader_updated_ || render_targets_updated_ to avoid flipping render targets too often
			std::vector<ID3D11RenderTargetView*> rtvs = current_RenderTargetViews;
			for (const auto & i : current_custom_pixel_shader_->rtv) {
				size_t j = i.second;
				if (rtvs.size() <= j)
					rtvs.resize(j + 1);
				rtvs[j] = *custom_render_targets_[i.first];
			}


			size_t NRT = rtvs.size();
			std::vector<ID3D11UnorderedAccessView*> uavs = current_UnorderedAccessViews;
			for (const auto & i : current_custom_pixel_shader_->uav) {
				if (i.second >= NRT) {
					size_t j = i.second - NRT;
					if (uavs.size() <= j)
						uavs.resize(j + 1);
					uavs[j] = *custom_render_targets_[i.first];
				}
			}

			D3D11Hook::OMSetRenderTargetsAndUnorderedAccessViews((UINT)rtvs.size(), rtvs.data(), current_DepthStencilView, (UINT)NRT, (UINT)uavs.size(), uavs.data(), nullptr);
		}
		TOC;
	}
	void endDraw() {
		TIC;
		onEndDraw(draw_info);
		for (int i = 0; i < 20; i++) {
			if (old_vs_cbuffer[i]) {
				D3D11Hook::VSSetConstantBuffers(i, 1, old_vs_cbuffer + i);
				old_vs_cbuffer[i] = 0;
			}
			if (old_ps_cbuffer[i]) {
				D3D11Hook::PSSetConstantBuffers(i, 1, old_ps_cbuffer + i);
				old_ps_cbuffer[i] = 0;
			}
		}
		if (current_custom_pixel_shader_)
			D3D11Hook::OMSetRenderTargetsAndUnorderedAccessViews((UINT)current_RenderTargetViews.size(), current_RenderTargetViews.data(), current_DepthStencilView, (UINT)current_RenderTargetViews.size(), (UINT)current_UnorderedAccessViews.size(), current_UnorderedAccessViews.data(), nullptr);
		
		draw_count += draw_info.instances ? draw_info.instances : 1;
		TOC;
	}
	virtual void Draw(UINT VertexCount, UINT StartVertexLocation) {
		beginDraw(DrawInfo::VERTEX, 0, VertexCount, 0, StartVertexLocation, 0);
		if (!hide_draw)
			D3D11Hook::Draw(VertexCount, StartVertexLocation);
		endDraw();
	}
	virtual void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
		beginDraw(DrawInfo::INDEX, 0, IndexCount, StartIndexLocation, BaseVertexLocation, 0);
		if (!hide_draw)
			D3D11Hook::DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
		endDraw();
	}

	virtual void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {
		beginDraw(DrawInfo::INDEX, InstanceCount, IndexCountPerInstance, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
		if (!hide_draw)
			D3D11Hook::DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
		endDraw();
	}
	virtual void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
		beginDraw(DrawInfo::VERTEX, InstanceCount, VertexCountPerInstance, 0, StartVertexLocation, StartInstanceLocation);
		if (!hide_draw)
			D3D11Hook::DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
		endDraw();
	}
	virtual void DrawAuto() {
		LOG(INFO) << "Not hooking DrawAuto";
		D3D11Hook::DrawAuto();
	}
	virtual void DrawIndexedInstancedIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) {
		LOG(INFO) << "Not hooking DrawIndexedInstancedIndirect";
		D3D11Hook::DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
	}
	virtual void DrawInstancedIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) {
		LOG(INFO) << "Not hooking DrawInstancedIndirect";
		D3D11Hook::DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
	}

	virtual int defaultWidth() const {
		return width;
	}
	virtual int defaultHeight() const {
		return height;
	}
};


void hookDevice(ID3D11Device** ppDevice, ID3D11DeviceContext** ppImmediateContext) {
	if (ppDevice && *ppDevice) {
		LOG(INFO) << "Hooking the device and device context";
		std::shared_ptr<GameHook> hook = std::make_shared<GameHook>();
		if (!wrapDeviceAndContext(ppDevice, ppImmediateContext, hook))
			LOG(WARN) << "Failed to wrap the device!";
	}
}
void hookSwapChain(IUnknown* pDevice, HWND wnd, IDXGISwapChain** ppSwapChain) {
	if (ppSwapChain && *ppSwapChain && pDevice) {
		D3D11Device *device_d3d11 = nullptr;
		if (SUCCEEDED(pDevice->QueryInterface(&device_d3d11))) {
			LOG(INFO) << "Hooking the swap chain";
			if (!wrapSwapChain(ppSwapChain, device_d3d11))
				LOG(WARN) << "Failed to wrap the swap chain";

			LOG(INFO) << "Hooking the io";
			std::shared_ptr<GameHook> hook = std::dynamic_pointer_cast<GameHook>(device_d3d11->h_);
			if (!hook || !wrapIO(wnd, hook))
				LOG(WARN) << "Failed to wrap the IO";
		}
	}
}

static uint32_t hooking_device = 0, hooking_swapchain = 0;
Hook<HRESULT, IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, CONST D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**> hD3D11CreateDevice;
HRESULT WINAPI nD3D11CreateDevice(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
	UINT Flags, CONST D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
	ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext) {
	if (!hooking_device) {
#if defined(_DEBUG)
		Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		hooking_device++;
		HRESULT r = hD3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
		if (ppDevice && *ppDevice)
			hookDevice(ppDevice, ppImmediateContext);
		hooking_device--;
		return r;
	} else {
		return hD3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
	}
}

Hook<HRESULT, IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT , CONST D3D_FEATURE_LEVEL*, UINT, UINT, CONST DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**> hD3D11CreateDeviceAndSwapChain;
HRESULT WINAPI nD3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
	UINT Flags, CONST D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext) {
	// Create the device and swapchain
	if (!hooking_device) {
#if defined(_DEBUG)
		Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		hooking_device++;
		bool hs = !(hooking_swapchain++);
		HRESULT r = hD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
		// Hook into them
		if (ppDevice && *ppDevice)
			hookDevice(ppDevice, ppImmediateContext);
		if (ppDevice && *ppDevice && ppSwapChain && hs)
			hookSwapChain(*ppDevice, pSwapChainDesc->OutputWindow, ppSwapChain);
		hooking_device--;
		hooking_swapchain--;
		return r;
	}
	else {
		return hD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
	}
}
template<typename T> struct SwapChainHook {
	static Hook<HRESULT, T *, IUnknown *, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **> hook;
	static HRESULT WINAPI DXGICreateSwapChain(T *_this, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGISwapChain **ppSwapChain) {
		if (!hooking_swapchain) {
			hooking_swapchain++;
			HRESULT r = hook(_this, pDevice, pDesc, ppSwapChain);
			if (ppSwapChain)
				hookSwapChain(pDevice, pDesc->OutputWindow, ppSwapChain);
			hooking_swapchain--;
			return r;
		}
		return hook(_this, pDevice, pDesc, ppSwapChain);
	}
	static void wrap(REFIID riid, void **p) {
		if (p && *p) {
			if (riid == __uuidof(T)) {
				if (!hook.func_) {
					// Setup the hook
					uintptr_t* pInterfaceVTable = (uintptr_t*)*(uintptr_t*)*p;
					hook.setup((HRESULT(WINAPI *)(T*, IUnknown *, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **))pInterfaceVTable[10], DXGICreateSwapChain);// 10: IDXGIFactory::CreateSwapChain
				}
			}
		}
	}
};
template<typename T>
Hook<HRESULT, T *, IUnknown *, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **> SwapChainHook<T>::hook;

Hook<HRESULT, REFIID, void**> hCreateDXGIFactory;
HRESULT WINAPI nCreateDXGIFactory(REFIID riid, void **ppFactory) {
	HRESULT r = hCreateDXGIFactory(riid, ppFactory);
	if (ppFactory && *ppFactory) SwapChainHook<IDXGIFactory>::wrap(riid, ppFactory);
	return r;
}
Hook<HRESULT, REFIID, void**> hCreateDXGIFactory1;
HRESULT WINAPI nCreateDXGIFactory1(REFIID riid, void **ppFactory) {
	HRESULT r = hCreateDXGIFactory1(riid, ppFactory);
	if (ppFactory && *ppFactory) SwapChainHook<IDXGIFactory1>::wrap(riid, ppFactory);
	return r;
}
Hook<HRESULT, UINT, REFIID, void**> hCreateDXGIFactory2;
HRESULT WINAPI nCreateDXGIFactory2(UINT   Flags, REFIID riid, _Out_ void   **ppFactory) {
	HRESULT r = hCreateDXGIFactory2(Flags, riid, ppFactory);
	if (ppFactory && *ppFactory) SwapChainHook<IDXGIFactory2>::wrap(riid, ppFactory);
	return r;
}


void hookD3D11() {
	LOG(INFO) << "Hooking d3d11 functions";
	hD3D11CreateDeviceAndSwapChain.setup(D3D11CreateDeviceAndSwapChain, nD3D11CreateDeviceAndSwapChain);
	hD3D11CreateDevice.setup(D3D11CreateDevice, nD3D11CreateDevice);
	hCreateDXGIFactory.setup(CreateDXGIFactory, nCreateDXGIFactory);
	hCreateDXGIFactory1.setup(CreateDXGIFactory1, nCreateDXGIFactory1);
	hCreateDXGIFactory2.setup(CreateDXGIFactory2, nCreateDXGIFactory2);
	hookIO();
	hookSpeed();
}
void unhookD3D11() {
	LOG(INFO) << "Good bye";
	hD3D11CreateDeviceAndSwapChain.disable();
	hD3D11CreateDevice.disable();
	hCreateDXGIFactory.disable();
	hCreateDXGIFactory1.disable();
	hCreateDXGIFactory2.disable();
	unhookIO();
	unhookSpeed();
}

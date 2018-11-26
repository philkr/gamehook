#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "shader.h"
#include "types.h"

#ifndef IMPORT
#define IMPORT __declspec(dllimport)
#endif

// A few notes on memory handling:
//  Sometimes callback functions are asked to return an object,
//  don't worry about that object leaking, the memory handling
//  will be taken care of in the background. In fact, please do
//  not delete any of these objects yourself, nor keep pointers
//  to these objects in your code! Copy them if needed.


typedef ShaderHash BufferHash;
typedef ShaderHash TextureHash;

/********************************************************/
/** Constant Buffer object                             **/
/********************************************************/

class CBuffer {
protected:
	virtual void write(const void * data, size_t offset, size_t size) = 0;
	CBuffer() = default;
	CBuffer(const CBuffer &) = delete;
	CBuffer& operator=(const CBuffer &) = delete;
public:
	virtual ~CBuffer() {}
	template<typename T> void set(T v, size_t offset = 0) {
		write(&v, offset, sizeof(T));
	}
	template<typename T> void set(const T* v, size_t n, size_t offset = 0) {
		write(v, offset, sizeof(T)*n);
	}
	template<typename T> void set(const std::vector<T>& v, size_t offset = 0) {
		write(v.data(), offset, sizeof(T)*v.size());
	}
};

/********************************************************/
/** Shader recoures                                    **/
/********************************************************/

struct Resource {
	uint32_t id = 0;
	Resource(uint32_t id = 0) :id(id) {}
	operator bool() const { return id; }
	operator uint32_t() const { return id; }
	Resource & operator=(uint32_t i) { id = i; return *this; }
};
struct Texture2D : public Resource { using Resource::Resource; };
struct RenderTargetView : public Resource { using Resource::Resource; uint16_t W=0, H=0; };
struct DepthStencilView : public Resource { using Resource::Resource; uint16_t W = 0, H = 0; };
struct Buffer : public Resource { using Resource::Resource; };


/********************************************************/
/** Memory objects                                     **/
/********************************************************/

// GPUMemory objects are used to fetch GPU data during a render pass, there are two different types of GPUMemory objects:
//  * Immediate objects read the contents of the memory as soon as they are created
//  * Delayed objects do not contain any data until the frame is completely rendered (postProcess/endFrame), where all data is fetched from the GPU
// Delayed objects are much more efficient and thus preferred. All GPUMemory is cleared after the current frame and will not survive past endFrame!
struct GPUMemory {
	virtual ~GPUMemory() {}
	// Pointer to the data (on the host/CPU), null if the memory is not accessible
	virtual const void * data() const = 0;
	virtual size_t size() const = 0;
	virtual std::shared_ptr<GPUMemory> sub(size_t o, size_t n) = 0;
	operator bool() const { return data(); }
	template<typename T> const T * data() const { return static_cast<const T*>(data()); }
};


/********************************************************/
/** Draw info                                          **/
/********************************************************/

struct BufferInfo {
	Buffer vertex = 0;
	Buffer index = 0;
	std::vector<Buffer> pixel_constant, vertex_constant;
};
struct ShaderInfo {
	ShaderHash vertex, pixel;
	Texture2D ps_texture_id;
};
struct RenderTargetInfo {
	std::vector<RenderTargetView> outputs;
	DepthStencilView depth;
	int width, height;
};

struct DrawInfo {
	BufferInfo buffer;
	ShaderInfo shader;
	RenderTargetInfo target;

	enum Type {
		VERTEX,
		INDEX,
	};
	uint8_t type;
	uint16_t instances;
	uint32_t n; // Number of vertices or indices
	uint32_t start_index, start_vertex, start_instance;
};

/********************************************************/
/** Game controller                                    **/
/********************************************************/

// Interface implementation of the game controller
class BaseGameController {
protected:
	BaseGameController(const BaseGameController&) = delete;
	BaseGameController& operator=(const BaseGameController&) = delete;
	BaseGameController() = default;

private: // Callback functions (they can be overwritten, but shouldn't be called directly)
	friend class MainGameController;
	
	// Initialization function to called soon after creation (once all other controllers are initialized)
	// To keeps things save, do most of the initialization and function calling in this function, not the contructor
	// Will be called before any other function
	virtual void onInitialize() {}
	virtual void onClose() {}

	/******* IO *******/
	// IO Callback functions (return true swallows the callback, no other functions are called)
	virtual bool onKeyDown(unsigned char key, unsigned char special_status) { return false; }
	virtual bool onKeyUp(unsigned char key) { return false; }
	virtual bool onKillFocus() { return false; }

	/******* Rendering *******/
	// Frame callback functions (called in order of definition below, onEndFrame is the last chance to fetch images, onPresent might have lost or overwritten the buffers already)
	// You can use onPresent to determine if you want to record the next frame
	virtual void onBeginFrame(uint32_t frame_id) {}
	virtual void onPostProcess(uint32_t frame_id) {}
	virtual void onEndFrame(uint32_t frame_id) {}
	virtual void onPresent(uint32_t frame_id) {}

	// Draw callback functions (called in order of definition)
	// If you want to cancel the drawing call use TODO:...
	virtual void onBeginDraw(const DrawInfo & i) {}
	virtual void onEndDraw(const DrawInfo & i) {}

	// Shader creation and binding callbacks
	virtual void onCreateShader(std::shared_ptr<Shader> shader) {}
	virtual void onBindShader(std::shared_ptr<Shader> shader) {}

	// Receive game commands
	virtual void onCommand(const std::string & json) {}

private: // State functions
	// Game state function
	// If multiple plugins produce a state, the JSON struct is merged. If multiple pugins produce the same value, duplicate entries appear in JSON.
	virtual std::string provideGameState() const { return ""; }

public:
	virtual ~BaseGameController() = default;

	/******* Shader *******/
	// Build a shader (create a DirectX object from the shader bytecode)
	virtual void buildShader(const std::shared_ptr<Shader> shader) = 0;
	// Bind a shader (the shader is built in this function if needed), if called within onBindShader the original game shader is not bound
	virtual void bindShader(const std::shared_ptr<Shader> shader) = 0;

	/******* IO *******/
	// IO Control functions (not not override unless you know what you're doing!)
	virtual void keyDown(unsigned char key) = 0;
	virtual void keyUp(unsigned char key) = 0;
	virtual const std::vector<uint8_t> & keyState() const = 0;
	// Button:   0: WM_MOUSEMOVE (down only), 1:WM_LBUTTON, 2:WM_RBUTTON, 3:WM_MBUTTON
	virtual void mouseDown(float x, float y, uint8_t button) = 0;
	virtual void mouseUp(float x, float y, uint8_t button) = 0;

	/******* Rendering *******/
	// Get (current frame) and set (next frame) the recording type, recordNextFrame will overwrite the recording type of the next frame.
	virtual void recordNextFrame(RecordingType type) = 0;
	virtual RecordingType currentRecordingType() const = 0;
	// Hide the current draw call (should only be used in onBeginDraw). It has no effect outside onBeginDraw.
	virtual void hideDraw(bool hide = true) = 0;

	// Create render targets. Only call these function in onInitialize (or if there is not other option in onPresent). Other behavior might be ill defined.
	virtual void addTarget(const std::string & name, TargetType type_hint, bool hidden = false) = 0;
	virtual void addCustomTarget(const std::string & name, TargetType type = TargetType::UNKNOWN, bool hidden = false) = 0;

	// Output functions
	virtual void copyTarget(const std::string & to, const std::string & from) = 0;
	virtual void copyTarget(const std::string & name, const RenderTargetView & rt) = 0;
	virtual std::vector<std::string> listTargets() const = 0;
	virtual TargetType targetType(const std::string & name) const = 0;
	virtual bool hasTarget(const std::string & name) const = 0;

	// Is the target ready to be read? Was it written in the last frame?
	virtual bool targetAvailable(const std::string & name) const = 0;
	
	virtual int defaultWidth() const = 0;
	virtual int defaultHeight() const = 0;

	// If you want to fetch a specific output request it in the beginFrame function (or any time before the target is written, not after!)
	// The request needs to be renewed for every new frame
	virtual void requestOutput(const std::string & name) = 0;
	virtual void requestOutput(const std::string & name, int W, int H) = 0;

	// What type does a certain output channel have?
	virtual DataType outputType(const std::string & name) const = 0;
	virtual int outputChannels(const std::string & name) const = 0;

	// You can only read targets in the endFrame function, calling it from anywhere else leads to undefined behavior
	// try to match the datatype it's more efficient
	virtual bool readTarget(const std::string & name, int W, int H, int C, DataType t, void * data) = 0;
	virtual bool readTarget(const std::string & name, int W, int H, int C, half * data) = 0;
	virtual bool readTarget(const std::string & name, int W, int H, int C, float * data) = 0;
	virtual bool readTarget(const std::string & name, int W, int H, int C, uint8_t * data) = 0;
	virtual bool readTarget(const std::string & name, int W, int H, int C, uint16_t * data) = 0;
	virtual bool readTarget(const std::string & name, int W, int H, int C, uint32_t * data) = 0;

	// Buffer handling and reading
	virtual std::shared_ptr<GPUMemory> readBuffer(Buffer b, size_t n, bool immediate = false) { return readBuffer(b, 0, n, immediate); }
	virtual std::shared_ptr<GPUMemory> readBuffer(Buffer b, size_t offset, size_t n, bool immediate = false) { return readBuffer(b, std::vector<size_t>{ offset }, std::vector<size_t>{ n }, immediate); }
	virtual std::shared_ptr<GPUMemory> readBuffer(Buffer b, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate = false) = 0;
	virtual size_t bufferSize(Buffer b) = 0;
	
	// Game state query functions
	virtual std::string gameState() const = 0;

	// Game command functions
	virtual void command(const std::string & json) = 0;

	// CBuffer handling
	virtual std::shared_ptr<CBuffer> createCBuffer(const std::string & name, size_t max_size) = 0;
	// Bind the cbuffer to the next draw call only. This function call only has an effect within beginDraw, and has no effect anywhere else
	virtual void bindCBuffer(std::shared_ptr<CBuffer> b) = 0;

	// Postprocessing functions
	virtual void callPostFx(std::shared_ptr<Shader> shader) = 0;
};

// This is the main wrapper class. To use the game wrapper inherit from this class
// and register your own class using REGISTER_WRAPPER . More than one wrapper may
// be registered at any given time.
class GameController: public BaseGameController {
protected:
	BaseGameController * main_;
	// Contructor
	template<typename T> friend struct TGameControllerFactory;
public:
	virtual void buildShader(const std::shared_ptr<Shader> shader) { main_->buildShader(shader); }
	virtual void bindShader(const std::shared_ptr<Shader> shader) { main_->bindShader(shader); }

	virtual void keyDown(unsigned char key) final { main_->keyDown(key); }
	virtual void keyUp(unsigned char key) final { main_->keyUp(key); }
	virtual const std::vector<uint8_t> & keyState() const final { return main_->keyState(); }
	virtual void mouseDown(float x, float y, uint8_t button) final { main_->mouseDown(x, y, button); }
	virtual void mouseUp(float x, float y, uint8_t button) final { main_->mouseUp(x, y, button); }

	virtual void recordNextFrame(RecordingType type) final { main_->recordNextFrame(type);  }
	virtual RecordingType currentRecordingType() const final { return main_->currentRecordingType(); }
	virtual void hideDraw(bool hide = true) final { main_->hideDraw(hide);  }

	virtual void addTarget(const std::string & name, bool hidden = false) { main_->addTarget(name, TargetType::UNKNOWN, hidden); }
	virtual void addTarget(const std::string & name, TargetType type_hint, bool hidden = false) { main_->addTarget(name, type_hint, hidden); }
	virtual void addCustomTarget(const std::string & name, TargetType type, bool hidden = false) { main_->addCustomTarget(name, type, hidden); }

	virtual void copyTarget(const std::string & to, const std::string & from) final { main_->copyTarget(to, from); }
	virtual void copyTarget(const std::string & name, const RenderTargetView & rt) final { main_->copyTarget(name, rt); }
	virtual std::vector<std::string> listTargets() const final { return main_->listTargets(); }
	virtual TargetType targetType(const std::string & name) const final { return main_->targetType(name); }
	virtual bool hasTarget(const std::string & name) const final { return main_->hasTarget(name); }
	virtual bool targetAvailable(const std::string & name) const final { return main_->targetAvailable(name); }

	virtual int defaultWidth() const final { return main_->defaultWidth(); }
	virtual int defaultHeight() const final { return main_->defaultHeight(); }

	virtual void requestOutput(const std::string & name) final { main_->requestOutput(name); }
	virtual void requestOutput(const std::string & name, int W, int H) final { main_->requestOutput(name, W, H); }

	virtual DataType outputType(const std::string & name) const final { return main_->outputType(name); }
	virtual int outputChannels(const std::string & name) const final { return main_->outputChannels(name); }

	virtual bool readTarget(const std::string & name, int W, int H, int C, DataType t, void * data) final { return main_->readTarget(name, W, H, C, t, data); }
	virtual bool readTarget(const std::string & name, int W, int H, int C, half * data) final { return main_->readTarget(name, W, H, C, data); }
	virtual bool readTarget(const std::string & name, int W, int H, int C, float * data) final { return main_->readTarget(name, W, H, C, data); }
	virtual bool readTarget(const std::string & name, int W, int H, int C, uint8_t * data) final { return main_->readTarget(name, W, H, C, data); }
	virtual bool readTarget(const std::string & name, int W, int H, int C, uint16_t * data) final { return main_->readTarget(name, W, H, C, data); }
	virtual bool readTarget(const std::string & name, int W, int H, int C, uint32_t * data) final { return main_->readTarget(name, W, H, C, data); }

	using BaseGameController::readBuffer;
	virtual std::shared_ptr<GPUMemory> readBuffer(Buffer b, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate = false) final { return main_->readBuffer(b, offset, n, immediate); }
	virtual size_t bufferSize(Buffer b) final { return main_->bufferSize(b); }

	virtual std::string gameState() const final { return main_->gameState(); }
	
	virtual void command(const std::string & json) final { return main_->command(json); };

	virtual std::shared_ptr<CBuffer> createCBuffer(const std::string & name, size_t max_size) final { return main_->createCBuffer(name, max_size); }
	virtual void bindCBuffer(std::shared_ptr<CBuffer> b) final { main_->bindCBuffer(b); }

	virtual void callPostFx(std::shared_ptr<Shader> shader) final { main_->callPostFx(shader); }
};


// Feel free to ignore anything below this line
struct GameControllerFactory {
	GameControllerFactory() = default;
	GameControllerFactory(const GameControllerFactory&) = delete;
	GameControllerFactory& operator=(const GameControllerFactory&) = delete;
	virtual ~GameControllerFactory() = default;
	virtual GameController* make(BaseGameController *c) = 0;
};
template<typename T> struct TGameControllerFactory: public GameControllerFactory {
	virtual GameController* make(BaseGameController *main) override {
		GameController * w = new T;
		w->main_ = main;
		return w;
	}
};

IMPORT size_t registerFactory(GameControllerFactory * h, int priority=0);
IMPORT size_t unregisterFactory(GameControllerFactory * h);
template<typename T>
struct FactoryRegistry {
	FactoryRegistry& operator=(const FactoryRegistry &) = delete;
	FactoryRegistry(const FactoryRegistry &) = delete;
	GameControllerFactory * f;
	FactoryRegistry(int P) :f(new TGameControllerFactory<T>()) { registerFactory(f, P); }
	~FactoryRegistry() { unregisterFactory(f); }
};
template<typename T> static size_t REG(int P=0) {
	static FactoryRegistry<T> reg(P);
	return !!reg.f;
}
#define REGISTER_CONTROLLER(C) static size_t reg_##C = REG<C>();
#define REGISTER_CONTROLLER_PRIORITY(C, P) static size_t reg_##C = REG<C>(P);

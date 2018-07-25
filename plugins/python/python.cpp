#define NOMINMAX
#include <windows.h>
#include <unordered_map>
#include <unordered_set>
#include "log.h"
#include "sdk.h"
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
namespace py = pybind11;

const std::vector<std::string> MODULE_PATHS = { "." };

struct PythonController;
struct BasePythonController {
	PythonController * main_ = nullptr;
	BasePythonController(const BasePythonController &) = delete;
	BasePythonController(PythonController * c):main_(c) {
	}
	virtual ~BasePythonController() {
	}
	/****** Callbacks ******/
	void onInitialize() {}
	void onClose() {}
	bool onKeyDown(unsigned char key, unsigned char special_status) { return false; }
	bool onKeyUp(unsigned char key) { return false; }
	void onBeginFrame(uint32_t frame_id) {}
	void onPostProcess(uint32_t frame_id) {}
	void onEndFrame(uint32_t frame_id) {}
	void onPresent(uint32_t frame_id) {}
	void onBeginDraw(const DrawInfo & i) {}
	void onEndDraw(const DrawInfo & i) {}
	void onCreateShader(std::shared_ptr<Shader> shader) {}
	void onBindShader(std::shared_ptr<Shader> shader) {}
	void onCommand(const std::string & json) {}

	//virtual std::vector<ProvidedTarget> providedTargets() const { return std::vector<ProvidedTarget>(); }
	//virtual std::vector<ProvidedTarget> providedCustomTargets() const { return std::vector<ProvidedTarget>(); }
	virtual std::string provideGameState() const { return ""; }
	/****** End Callbacks ******/

	/****** Commands ******/
	void buildShader(const std::shared_ptr<Shader> shader);
	void bindShader(const std::shared_ptr<Shader> shader);
	void keyDown(unsigned char key, bool syskey = false);
	void keyUp(unsigned char key, bool syskey = false);
	const std::vector<uint8_t> & keyState() const;
	void mouseDown(float x, float y, uint8_t button);
	void mouseUp(float x, float y, uint8_t button);
	void recordNextFrame(RecordingType type);
	RecordingType currentRecordingType() const;
	void hideDraw(bool hide = true);
	void copyTarget(const std::string & to, const std::string & from);
	void copyTarget_1(const std::string & name, const RenderTargetView & rt);
	std::vector<std::string> listTargets() const;
	TargetType targetType(const std::string & name) const;
	bool hasTarget(const std::string & name) const;
	bool targetAvailable(const std::string & name) const;
	int defaultWidth() const;
	int defaultHeight() const;
	void requestOutput(const std::string & name);
	void requestOutput_1(const std::string & name, int W, int H);
	DataType outputType(const std::string & name) const;
	int outputChannels(const std::string & name) const;
	bool readTarget(const std::string & name, int W, int H, int C, DataType t, void * data);
	bool readTarget_1(const std::string & name, int W, int H, int C, half * data);
	bool readTarget_2(const std::string & name, int W, int H, int C, float * data);
	bool readTarget_3(const std::string & name, int W, int H, int C, uint8_t * data);
	bool readTarget_4(const std::string & name, int W, int H, int C, uint16_t * data);
	bool readTarget_5(const std::string & name, int W, int H, int C, uint32_t * data);
	std::shared_ptr<GPUMemory> readBuffer(Buffer b, size_t n, bool immediate = false);
	std::shared_ptr<GPUMemory> readBuffer_1(Buffer b, size_t offset, size_t n, bool immediate = false);
	std::shared_ptr<GPUMemory> readBuffer_2(Buffer b, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate = false);
	size_t bufferSize(Buffer b);
	std::string gameState() const;
	void command(const std::string & json);
	std::shared_ptr<CBuffer> createCBuffer(const std::string & name, size_t max_size);
	void bindCBuffer(std::shared_ptr<CBuffer> b);
	void callPostFx(std::shared_ptr<Shader> shader);
	/****** End Commands ******/
};
struct PythonControllerRef {
	PythonController * ref;
	PythonControllerRef(PythonController* r) :ref(r) {}
	operator PythonController *() { return ref; }
};
std::string arg2str(py::args a) {
	std::string r = "";
	for (int i = 0; i < a.size(); i++)
		r += (i?" ":"") + (std::string)py::str(a[i]);
	return r;
}
template<typename T>
void wrapType(T m) {
	py::enum_<RecordingType>(m, "RecordingType")
		.value("NONE", RecordingType::NONE)
		.value("DRAW", RecordingType::DRAW)
		.value("DRAW_FIRST", RecordingType::DRAW_FIRST);

	py::enum_<TargetType>(m, "TargetType")
		.value("UNKNOWN", TargetType::UNKNOWN)
		.value("R32G32B32A32_TYPELESS", TargetType::R32G32B32A32_TYPELESS)
		.value("R32G32B32A32_FLOAT", TargetType::R32G32B32A32_FLOAT)
		.value("R32G32B32A32_UINT", TargetType::R32G32B32A32_UINT)
		.value("R32G32B32A32_SINT", TargetType::R32G32B32A32_SINT)
		.value("R32G32B32_TYPELESS", TargetType::R32G32B32_TYPELESS)
		.value("R32G32B32_FLOAT", TargetType::R32G32B32_FLOAT)
		.value("R32G32B32_UINT", TargetType::R32G32B32_UINT)
		.value("R32G32B32_SINT", TargetType::R32G32B32_SINT)
		.value("R16G16B16A16_TYPELESS", TargetType::R16G16B16A16_TYPELESS)
		.value("R16G16B16A16_FLOAT", TargetType::R16G16B16A16_FLOAT)
		.value("R16G16B16A16_UNORM", TargetType::R16G16B16A16_UNORM)
		.value("R16G16B16A16_UINT", TargetType::R16G16B16A16_UINT)
		.value("R16G16B16A16_SNORM", TargetType::R16G16B16A16_SNORM)
		.value("R16G16B16A16_SINT", TargetType::R16G16B16A16_SINT)
		.value("R32G32_TYPELESS", TargetType::R32G32_TYPELESS)
		.value("R32G32_FLOAT", TargetType::R32G32_FLOAT)
		.value("R32G32_UINT", TargetType::R32G32_UINT)
		.value("R32G32_SINT", TargetType::R32G32_SINT)
		.value("R32G8X24_TYPELESS", TargetType::R32G8X24_TYPELESS)
		.value("D32_FLOAT_S8X24_UINT", TargetType::D32_FLOAT_S8X24_UINT)
		.value("R32_FLOAT_X8X24_TYPELESS", TargetType::R32_FLOAT_X8X24_TYPELESS)
		.value("X32_TYPELESS_G8X24_UINT", TargetType::X32_TYPELESS_G8X24_UINT)
		.value("R10G10B10A2_TYPELESS", TargetType::R10G10B10A2_TYPELESS)
		.value("R10G10B10A2_UNORM", TargetType::R10G10B10A2_UNORM)
		.value("R10G10B10A2_UINT", TargetType::R10G10B10A2_UINT)
		.value("R11G11B10_FLOAT", TargetType::R11G11B10_FLOAT)
		.value("R8G8B8A8_TYPELESS", TargetType::R8G8B8A8_TYPELESS)
		.value("R8G8B8A8_UNORM", TargetType::R8G8B8A8_UNORM)
		.value("R8G8B8A8_UNORM_SRGB", TargetType::R8G8B8A8_UNORM_SRGB)
		.value("R8G8B8A8_UINT", TargetType::R8G8B8A8_UINT)
		.value("R8G8B8A8_SNORM", TargetType::R8G8B8A8_SNORM)
		.value("R8G8B8A8_SINT", TargetType::R8G8B8A8_SINT)
		.value("R16G16_TYPELESS", TargetType::R16G16_TYPELESS)
		.value("R16G16_FLOAT", TargetType::R16G16_FLOAT)
		.value("R16G16_UNORM", TargetType::R16G16_UNORM)
		.value("R16G16_UINT", TargetType::R16G16_UINT)
		.value("R16G16_SNORM", TargetType::R16G16_SNORM)
		.value("R16G16_SINT", TargetType::R16G16_SINT)
		.value("R32_TYPELESS", TargetType::R32_TYPELESS)
		.value("D32_FLOAT", TargetType::D32_FLOAT)
		.value("R32_FLOAT", TargetType::R32_FLOAT)
		.value("R32_UINT", TargetType::R32_UINT)
		.value("R32_SINT", TargetType::R32_SINT)
		.value("R24G8_TYPELESS", TargetType::R24G8_TYPELESS)
		.value("D24_UNORM_S8_UINT", TargetType::D24_UNORM_S8_UINT)
		.value("R24_UNORM_X8_TYPELESS", TargetType::R24_UNORM_X8_TYPELESS)
		.value("X24_TYPELESS_G8_UINT", TargetType::X24_TYPELESS_G8_UINT)
		.value("R8G8_TYPELESS", TargetType::R8G8_TYPELESS)
		.value("R8G8_UNORM", TargetType::R8G8_UNORM)
		.value("R8G8_UINT", TargetType::R8G8_UINT)
		.value("R8G8_SNORM", TargetType::R8G8_SNORM)
		.value("R8G8_SINT", TargetType::R8G8_SINT)
		.value("R16_TYPELESS", TargetType::R16_TYPELESS)
		.value("R16_FLOAT", TargetType::R16_FLOAT)
		.value("D16_UNORM", TargetType::D16_UNORM)
		.value("R16_UNORM", TargetType::R16_UNORM)
		.value("R16_UINT", TargetType::R16_UINT)
		.value("R16_SNORM", TargetType::R16_SNORM)
		.value("R16_SINT", TargetType::R16_SINT)
		.value("R8_TYPELESS", TargetType::R8_TYPELESS)
		.value("R8_UNORM", TargetType::R8_UNORM)
		.value("R8_UINT", TargetType::R8_UINT)
		.value("R8_SNORM", TargetType::R8_SNORM)
		.value("R8_SINT", TargetType::R8_SINT)
		.value("A8_UNORM", TargetType::A8_UNORM)
		.value("R1_UNORM", TargetType::R1_UNORM)
		.value("R9G9B9E5_SHAREDEXP", TargetType::R9G9B9E5_SHAREDEXP)
		.value("R8G8_B8G8_UNORM", TargetType::R8G8_B8G8_UNORM)
		.value("G8R8_G8B8_UNORM", TargetType::G8R8_G8B8_UNORM)
		.value("BC1_TYPELESS", TargetType::BC1_TYPELESS)
		.value("BC1_UNORM", TargetType::BC1_UNORM)
		.value("BC1_UNORM_SRGB", TargetType::BC1_UNORM_SRGB)
		.value("BC2_TYPELESS", TargetType::BC2_TYPELESS)
		.value("BC2_UNORM", TargetType::BC2_UNORM)
		.value("BC2_UNORM_SRGB", TargetType::BC2_UNORM_SRGB)
		.value("BC3_TYPELESS", TargetType::BC3_TYPELESS)
		.value("BC3_UNORM", TargetType::BC3_UNORM)
		.value("BC3_UNORM_SRGB", TargetType::BC3_UNORM_SRGB)
		.value("BC4_TYPELESS", TargetType::BC4_TYPELESS)
		.value("BC4_UNORM", TargetType::BC4_UNORM)
		.value("BC4_SNORM", TargetType::BC4_SNORM)
		.value("BC5_TYPELESS", TargetType::BC5_TYPELESS)
		.value("BC5_UNORM", TargetType::BC5_UNORM)
		.value("BC5_SNORM", TargetType::BC5_SNORM)
		.value("B5G6R5_UNORM", TargetType::B5G6R5_UNORM)
		.value("B5G5R5A1_UNORM", TargetType::B5G5R5A1_UNORM)
		.value("B8G8R8A8_UNORM", TargetType::B8G8R8A8_UNORM)
		.value("B8G8R8X8_UNORM", TargetType::B8G8R8X8_UNORM)
		.value("R10G10B10_XR_BIAS_A2_UNORM", TargetType::R10G10B10_XR_BIAS_A2_UNORM)
		.value("B8G8R8A8_TYPELESS", TargetType::B8G8R8A8_TYPELESS)
		.value("B8G8R8A8_UNORM_SRGB", TargetType::B8G8R8A8_UNORM_SRGB)
		.value("B8G8R8X8_TYPELESS", TargetType::B8G8R8X8_TYPELESS)
		.value("B8G8R8X8_UNORM_SRGB", TargetType::B8G8R8X8_UNORM_SRGB)
		.value("BC6H_TYPELESS", TargetType::BC6H_TYPELESS)
		.value("BC6H_UF16", TargetType::BC6H_UF16)
		.value("BC6H_SF16", TargetType::BC6H_SF16)
		.value("BC7_TYPELESS", TargetType::BC7_TYPELESS)
		.value("BC7_UNORM", TargetType::BC7_UNORM)
		.value("BC7_UNORM_SRGB", TargetType::BC7_UNORM_SRGB)
		.value("AYUV", TargetType::AYUV)
		.value("Y410", TargetType::Y410)
		.value("Y416", TargetType::Y416)
		.value("NV12", TargetType::NV12)
		.value("P010", TargetType::P010)
		.value("P016", TargetType::P016)
		.value("_420_OPAQUE", TargetType::_420_OPAQUE)
		.value("YUY2", TargetType::YUY2)
		.value("Y210", TargetType::Y210)
		.value("Y216", TargetType::Y216)
		.value("NV11", TargetType::NV11)
		.value("AI44", TargetType::AI44)
		.value("IA44", TargetType::IA44)
		.value("P8", TargetType::P8)
		.value("A8P8", TargetType::A8P8)
		.value("B4G4R4A4_UNORM", TargetType::B4G4R4A4_UNORM)
		.value("P208", TargetType::P208)
		.value("V208", TargetType::V208)
		.value("V408", TargetType::V408)
		.value("FORCE_UINT", TargetType::FORCE_UINT);

	py::enum_<BindTarget>(m, "BindTarget")
		.value("COLOR0", BindTarget::COLOR0)
		.value("COLOR1", BindTarget::COLOR1)
		.value("COLOR2", BindTarget::COLOR2)
		.value("COLOR3", BindTarget::COLOR3)
		.value("COLOR4", BindTarget::COLOR4)
		.value("COLOR5", BindTarget::COLOR5)
		.value("COLOR6", BindTarget::COLOR6)
		.value("COLOR7", BindTarget::COLOR7)
		.value("UNBIND", BindTarget::UNBIND);

	py::enum_<DataType>(m, "DataType")
		.value("UINT8", DataType::DT_UINT8)
		.value("UINT16", DataType::DT_UINT16)
		.value("UINT32", DataType::DT_UINT32)
		.value("FLOAT", DataType::DT_FLOAT)
		.value("HALF", DataType::DT_HALF)
		.value("UNKNOWN", DataType::DT_UNKNOWN);

	m.def("data_size", dataSize);
}

template<typename T>
void wrapShader(T m) {
	py::class_<ShaderHash>(m, "ShaderHash", py::buffer_protocol())
		.def_buffer([](ShaderHash &h) -> py::buffer_info {
			return py::buffer_info(
				h.h,                                    /* Pointer to buffer */
				sizeof(uint32_t),                         /* Size of one scalar */
				py::format_descriptor<uint32_t>::format(),/* Python struct-style format descriptor */
				1,                                      /* Number of dimensions */
				{ 4 },                                  /* Buffer dimensions */
				{ sizeof(uint32_t) }                      /* Strides (in bytes) for each index */
			);
		});


	py::class_<Shader, std::shared_ptr<Shader>> c(m, "Shader");
	py::enum_<Shader::Type>(c, "Type")
		.value("UNKNOWN", Shader::Type::UNKNOWN)
		.value("VERTEX", Shader::Type::VERTEX)
		.value("PIXEL", Shader::Type::PIXEL)
		.value("COMPUTE", Shader::Type::COMPUTE);

	py::class_<Shader::Buffer>(c, "Buffer")
		.def_readonly("name", &Shader::Buffer::name)
		.def_readonly("bind_point", &Shader::Buffer::bind_point);

	py::class_<Shader::Binding>(c, "Binding")
		.def_readonly("name", &Shader::Binding::name)
		.def_readonly("bind_point", &Shader::Binding::bind_point);

	c
		.def_static("create", &Shader::create)
		.def("append", &Shader::append, py::call_guard<py::gil_scoped_release>())
//		.def("subset", Shader::subset, py::call_guard<py::gil_scoped_release>())
		.def("renameCBuffer", &Shader::renameCBuffer, py::call_guard<py::gil_scoped_release>())
		.def("renameOutput", &Shader::renameOutput, py::call_guard<py::gil_scoped_release>())
		.def("disassemble", &Shader::disassemble, py::call_guard<py::gil_scoped_release>())

		.def_property_readonly("type", &Shader::type, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("hash", &Shader::hash, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("inputs", &Shader::inputs, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("outputs", &Shader::outputs, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("cbuffers", &Shader::cbuffers, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("sbuffers", &Shader::sbuffers, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("textures", &Shader::textures, py::call_guard<py::gil_scoped_release>())
		.def("data", [](std::shared_ptr<Shader> ths) {return ByteCode(ths->data(), ths->data()+ths->size());})
		;
}

PYBIND11_EMBEDDED_MODULE(api, m) {
	wrapType(m);
	wrapShader(m);

	py::class_<PythonControllerRef>(m, "__PythonControllerRef");

	py::class_<DrawInfo>(m, "DrawInfo")
		.def_readonly("type", &DrawInfo::type)
		.def_readonly("instances", &DrawInfo::instances)
		.def_readonly("n", &DrawInfo::n);

	py::class_<BasePythonController>(m, "BaseController")
		.def(py::init<PythonControllerRef>())
		.def("build_shader", &BasePythonController::buildShader, py::call_guard<py::gil_scoped_release>())
		.def("bind_shader", &BasePythonController::bindShader, py::call_guard<py::gil_scoped_release>())
		.def("key_down", &BasePythonController::keyDown, py::call_guard<py::gil_scoped_release>())
		.def("key_up", &BasePythonController::keyUp, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("key_state", &BasePythonController::keyState, py::call_guard<py::gil_scoped_release>())
		.def("mouse_down", &BasePythonController::mouseDown, py::call_guard<py::gil_scoped_release>())
		.def("mouse_up", &BasePythonController::mouseUp, py::call_guard<py::gil_scoped_release>())
		.def("record_next_frame", &BasePythonController::recordNextFrame, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("current_recording_type", &BasePythonController::currentRecordingType, py::call_guard<py::gil_scoped_release>())
		.def("hide_draw", &BasePythonController::hideDraw, py::call_guard<py::gil_scoped_release>())
		.def("copy_target", &BasePythonController::copyTarget, py::call_guard<py::gil_scoped_release>())
		.def("copy_target", &BasePythonController::copyTarget_1, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("list_targets", &BasePythonController::listTargets, py::call_guard<py::gil_scoped_release>())
		.def("target_type", &BasePythonController::targetType, py::call_guard<py::gil_scoped_release>())
		.def("has_target", &BasePythonController::hasTarget, py::call_guard<py::gil_scoped_release>())
		.def("target_available", &BasePythonController::targetAvailable, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("default_width", &BasePythonController::defaultWidth, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("default_height", &BasePythonController::defaultHeight, py::call_guard<py::gil_scoped_release>())
		.def("request_output", &BasePythonController::requestOutput, py::call_guard<py::gil_scoped_release>())
		.def("request_output", &BasePythonController::requestOutput_1, py::call_guard<py::gil_scoped_release>())
		.def("output_type", &BasePythonController::outputType, py::call_guard<py::gil_scoped_release>())
		.def("output_channels", &BasePythonController::outputChannels, py::call_guard<py::gil_scoped_release>())
		.def("read_target", &BasePythonController::readTarget, py::call_guard<py::gil_scoped_release>())
		.def("read_target", &BasePythonController::readTarget_1, py::call_guard<py::gil_scoped_release>())
		.def("read_target", &BasePythonController::readTarget_2, py::call_guard<py::gil_scoped_release>())
		.def("read_target", &BasePythonController::readTarget_3, py::call_guard<py::gil_scoped_release>())
		.def("read_target", &BasePythonController::readTarget_4, py::call_guard<py::gil_scoped_release>())
		.def("read_target", &BasePythonController::readTarget_5, py::call_guard<py::gil_scoped_release>())
		.def("read_buffer", &BasePythonController::readBuffer, py::call_guard<py::gil_scoped_release>())
		.def("read_buffer", &BasePythonController::readBuffer_1, py::call_guard<py::gil_scoped_release>())
		.def("read_buffer", &BasePythonController::readBuffer_2, py::call_guard<py::gil_scoped_release>())
		.def("buffer_size", &BasePythonController::bufferSize, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("game_state", &BasePythonController::gameState, py::call_guard<py::gil_scoped_release>())
		.def("command", &BasePythonController::command, py::call_guard<py::gil_scoped_release>())
		.def("create_c_buffer", &BasePythonController::createCBuffer, py::call_guard<py::gil_scoped_release>())
		.def("bind_c_buffer", &BasePythonController::bindCBuffer, py::call_guard<py::gil_scoped_release>())
		.def("call_post_fx", &BasePythonController::callPostFx, py::call_guard<py::gil_scoped_release>());

	m.def("info", [](py::args s) {LOG(INFO) << arg2str(s); });
	m.def("warn", [](py::args s) {LOG(WARN) << arg2str(s); });
	m.def("err", [](py::args s) {LOG(ERR) << arg2str(s); });
}

// Make sure the python interpreter is running (this needs to come after any module definition!)
py::scoped_interpreter interpreter;
py::gil_scoped_release release__; // Let's not hold the interpreter by default (instead acquire it whenever needed)

struct PythonController : public GameController {
	std::vector<py::module> modules;
	bool controllers_loaded = false;
	std::vector<py::object> controllers;
	void loadModules(std::string dir = ".") {
		py::gil_scoped_acquire acquire;
		modules.clear();
		py::module sys = py::module::import("sys");
		py::list path = sys.attr("path");
		if (!path.contains(dir)) path.append(dir);

		// Load all the modules
		WIN32_FIND_DATA data;
		HANDLE hFind = FindFirstFile((dir+"/*.py").c_str(), &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				LOG(INFO) << "Loading python module " << data.cFileName;
				try {
					std::string module_name(data.cFileName);
					module_name = module_name.substr(0, module_name.size() - 3); // Strip the .py

					py::module m = py::module::import(module_name.c_str());
					m.reload();
					modules.push_back(m);
				} catch (py::error_already_set e) {
					LOG(WARN) << "Failed to load python module '" << data.cFileName << "'! " << e.what();
				}
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}
		// Make sure the original modules gets collected.
		auto gc = py::module::import("gc");
		gc.attr("collect")();
	}
	void loadAllModules() {
		for (auto s : MODULE_PATHS)
			loadModules(s);
	}
	void loadAllControllers() {
		if (!controllers_loaded) {
			py::gil_scoped_acquire acquire;
			controllers_loaded = true;
			controllers.clear();
			auto inspect = py::module::import("inspect");
			auto base_controller = py::module::import("api").attr("BaseController");
			for (auto m : modules) {
				py::list mb = inspect.attr("getmembers")(m);
				for (auto c : mb) {
					py::object cls = py::cast<py::tuple>(c)[1];
					if (py::cast<bool>(inspect.attr("isclass")(cls)) && PyObject_IsSubclass(cls.ptr(), base_controller.ptr()))
						controllers.push_back(cls(PythonControllerRef(this)));
				}
			}
		}
	}
	void unloadAllControllers() {
		callAll("unload");
		py::gil_scoped_acquire acquire;
		controllers.clear();
		controllers_loaded = false;
		auto gc = py::module::import("gc");
		gc.attr("collect")();
	}
	void unloadAllModules() {
		py::gil_scoped_acquire acquire;
		modules.clear();
		auto gc = py::module::import("gc");
		gc.attr("collect")();
	}
	void initialize() {
		loadAllModules();
		loadAllControllers();
	}
	~PythonController() {
		unloadAllModules();
	}
	void reloadAllModules() {
		unloadAllControllers();
		unloadAllModules();
		loadAllModules();
		loadAllControllers();
	}
	template<typename T, typename ...ARGS> T callAll(const char * fname, ARGS... args) {
		py::gil_scoped_acquire acquire;
		T r = (T)0;
		for (auto c : controllers)
			if (py::hasattr(c, fname)) {
				try {
					py::object fn = c.attr(fname);
					if (!fn.is_none()) {
						try {
							T rr = py::cast<T>(fn(args...));
							if (rr) r = rr;
						} catch (py::error_already_set e) {
							LOG(WARN) << "Failed to call function " << e.what();
						} catch (py::reference_cast_error e) {
							LOG(WARN) << "Failed to cast return value " << e.what();
						}
					}
				} catch (py::error_already_set e) {
					LOG(WARN) << e.what();
				}
			}
		return r;
	}
	template<typename ...ARGS> void callAll(const char * fname, ARGS... args) {
		py::gil_scoped_acquire acquire;
		for (auto c : controllers)
			if (py::hasattr(c, fname)) {
				py::object fn = c.attr(fname);
				if (!fn.is_none()) {
					try {
						fn(args...);
					} catch (py::error_already_set e) {
						LOG(WARN) << "Failed to call function " << e.what();
					}
				}
			}
	}
	virtual void onInitialize() final {
		callAll("on_initialize");
	}
	virtual void onClose() final {
		callAll("on_close");
	}
	virtual bool onKeyDown(unsigned char key, unsigned char special_status) final {
		return callAll<bool>("on_key_down", key, special_status);
	}
	virtual bool onKeyUp(unsigned char key) final {
		return callAll<bool>("on_key_up", key);
	}
	virtual void onBeginFrame(uint32_t frame_id) final {
		callAll("on_begin_frame", frame_id);
	}
	virtual void onPostProcess(uint32_t frame_id) final {
		callAll("on_post_process", frame_id);
	}
	virtual void onEndFrame(uint32_t frame_id) final {
		callAll("on_end_frame", frame_id);
	}
	virtual void onPresent(uint32_t frame_id) final {
		callAll("on_present", frame_id);
	}
	virtual void onBeginDraw(const DrawInfo & i) final {
		callAll("on_begin_draw", i);
	}
	virtual void onEndDraw(const DrawInfo & i) final {
		callAll("on_end_draw", i);
	}
	virtual void onCreateShader(std::shared_ptr<Shader> shader) final {
		callAll("on_create_shader", shader);
	}
	virtual void onBindShader(std::shared_ptr<Shader> shader) final {
		callAll("on_bind_shader", shader);
	}
	virtual void onCommand(const std::string & json) final {
		callAll("on_command", json);
	}
	virtual std::string provideGameState() const final {
		py::gil_scoped_acquire acquire;
		std::string r = "{";
		for (auto c : controllers)
			if (py::hasattr(c, "provide_game_state")) {
				py::object gs = c.attr("provide_game_state");
				if (!gs.is_none()) {
					if (r.size() > 1) r += ",";
					std::string rr;
					{
						rr = (std::string)py::str(gs());
					}
					if (rr.size() && rr[0] == '{' && rr[rr.size() - 1] == '}')
						rr = rr.substr(1, rr.size() - 2);
					if (rr.size())
						r += rr;
				}
			}
		return r + "}";
	}
};

REGISTER_CONTROLLER(PythonController);

void BasePythonController::buildShader(const std::shared_ptr<Shader> shader) { main_->buildShader(shader); }
void BasePythonController::bindShader(const std::shared_ptr<Shader> shader) { main_->bindShader(shader); }
void BasePythonController::keyDown(unsigned char key, bool syskey) { main_->keyDown(key, syskey); }
void BasePythonController::keyUp(unsigned char key, bool syskey) { main_->keyUp(key, syskey); }
const std::vector<uint8_t> & BasePythonController::keyState() const { return main_->keyState(); }
void BasePythonController::mouseDown(float x, float y, uint8_t button) { main_->mouseDown(x, y, button); }
void BasePythonController::mouseUp(float x, float y, uint8_t button) { main_->mouseUp(x, y, button); }
void BasePythonController::recordNextFrame(RecordingType type) { main_->recordNextFrame(type); }
RecordingType BasePythonController::currentRecordingType() const { return main_->currentRecordingType(); }
void BasePythonController::hideDraw(bool hide) { main_->hideDraw(hide); }
void BasePythonController::copyTarget(const std::string & to, const std::string & from) { main_->copyTarget(to, from); }
void BasePythonController::copyTarget_1(const std::string & name, const RenderTargetView & rt) { main_->copyTarget(name, rt); }
std::vector<std::string> BasePythonController::listTargets() const { return main_->listTargets(); }
TargetType BasePythonController::targetType(const std::string & name) const { return main_->targetType(name); }
bool BasePythonController::hasTarget(const std::string & name) const { return main_->hasTarget(name); }
bool BasePythonController::targetAvailable(const std::string & name) const { return main_->targetAvailable(name); }
int BasePythonController::defaultWidth() const { return main_->defaultWidth(); }
int BasePythonController::defaultHeight() const { return main_->defaultHeight(); }
void BasePythonController::requestOutput(const std::string & name) { main_->requestOutput(name); }
void BasePythonController::requestOutput_1(const std::string & name, int W, int H) { main_->requestOutput(name, W, H); }
DataType BasePythonController::outputType(const std::string & name) const { return main_->outputType(name); }
int BasePythonController::outputChannels(const std::string & name) const { return main_->outputChannels(name); }
bool BasePythonController::readTarget(const std::string & name, int W, int H, int C, DataType t, void * data) { return main_->readTarget(name, W, H, C, t, data); }
bool BasePythonController::readTarget_1(const std::string & name, int W, int H, int C, half * data) { return main_->readTarget(name, W, H, C, data); }
bool BasePythonController::readTarget_2(const std::string & name, int W, int H, int C, float * data) { return main_->readTarget(name, W, H, C, data); }
bool BasePythonController::readTarget_3(const std::string & name, int W, int H, int C, uint8_t * data) { return main_->readTarget(name, W, H, C, data); }
bool BasePythonController::readTarget_4(const std::string & name, int W, int H, int C, uint16_t * data) { return main_->readTarget(name, W, H, C, data); }
bool BasePythonController::readTarget_5(const std::string & name, int W, int H, int C, uint32_t * data) { return main_->readTarget(name, W, H, C, data); }
std::shared_ptr<GPUMemory> BasePythonController::readBuffer(Buffer b, size_t n, bool immediate) { return main_->readBuffer(b, n, immediate); }
std::shared_ptr<GPUMemory> BasePythonController::readBuffer_1(Buffer b, size_t offset, size_t n, bool immediate) { return main_->readBuffer(b, offset, n, immediate); }
std::shared_ptr<GPUMemory> BasePythonController::readBuffer_2(Buffer b, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate) { return main_->readBuffer(b, offset, n, immediate); }
size_t BasePythonController::bufferSize(Buffer b) { return main_->bufferSize(b); }
std::string BasePythonController::gameState() const { return main_->gameState(); }
void BasePythonController::command(const std::string & json) { main_->command(json); }
std::shared_ptr<CBuffer> BasePythonController::createCBuffer(const std::string & name, size_t max_size) { return main_->createCBuffer(name, max_size); }
void BasePythonController::bindCBuffer(std::shared_ptr<CBuffer> b) { main_->bindCBuffer(b); }
void BasePythonController::callPostFx(std::shared_ptr<Shader> shader) { main_->callPostFx(shader); }


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		LOG(INFO) << "Python plugin turned on";
	}

	if (reason == DLL_PROCESS_DETACH) {
		LOG(INFO) << "Python plugin turned off";
	}
	return TRUE;
}


//
//std::shared_ptr<Shader> Shader::compile(const std::string & src, Type type, std::string * err) {
//	if (type == Shader::UNKNOWN) return std::shared_ptr<Shader>();
//	const char * targets[] = { "ps_5_1", "vs_5_1", nullptr };
//	ID3DBlob * code, *error_msgs;
//	HRESULT hr = D3DCompile(src.c_str(), src.size(), nullptr, nullptr, nullptr, "main", targets[(int)type], 0, 0, &code, &error_msgs);
//	if (FAILED(hr)) {
//		if (err)
//			*err = std::string((const char*)error_msgs->GetBufferPointer(), error_msgs->GetBufferSize());
//		code->Release();
//		error_msgs->Release();
//		return std::shared_ptr<Shader>();
//	}
//	ByteCode bc((const char*)code->GetBufferPointer(), ((const char*)code->GetBufferPointer()) + code->GetBufferSize());
//	code->Release();
//	error_msgs->Release();
//	if (type == PIXEL)
//		return std::make_shared<PixelShader>(bc);
//	if (type == VERTEX)
//		return std::make_shared<VertexShader>(bc);
//	return nullptr;
//}
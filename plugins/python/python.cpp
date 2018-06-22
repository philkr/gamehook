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
	virtual bool keyDown(unsigned char key, unsigned char special_status) { return false; }
	virtual bool keyUp(unsigned char key) { return false; }

	virtual RecordingType recordFrame(uint32_t frame_id) { return NONE; }
	virtual void startFrame(uint32_t frame_id) {}
	virtual void postProcess(uint32_t frame_id) {}
	virtual void endFrame(uint32_t frame_id) {}
	virtual DrawType startDraw(const DrawInfo & i) { return DEFAULT; }
	virtual void endDraw(const DrawInfo & i) {}

	//virtual std::shared_ptr<Shader> injectShader(std::shared_ptr<Shader> shader) { return std::shared_ptr<Shader>(); }
	//virtual std::vector<ProvidedTarget> providedTargets() const { return std::vector<ProvidedTarget>(); }
	//virtual std::vector<ProvidedTarget> providedCustomTargets() const { return std::vector<ProvidedTarget>(); }

	virtual void command(const std::string & json) {}
	virtual std::string gameState() const { return ""; }

	// Going to remove the controller soon
	virtual void unload() {}
	/****** End Callbacks ******/


	RecordingType currentRecordingType() const;
	virtual void copyTarget(const std::string & to, const std::string & from) final;
	virtual std::vector<std::string> listTargets() const final;

	virtual int defaultWidth() const final;
	virtual int defaultHeight() const final;

	// Game state query functions
	virtual std::string getGameState() const final;

	// Game command functions
	virtual void sendCommand(const std::string & json) final;

	/****** Currently UNSUPPORTED ******/
//	virtual void copyTarget(const std::string & name, const RenderTargetView & rt) final { parent_->copyTarget(name, rt); }
//	virtual TargetType targetType(const std::string & name) const final { return main_->targetType(name); }
//	virtual bool hasTarget(const std::string & name) const final { return main_->hasTarget(name); }
//	virtual bool targetAvailable(const std::string & name) const final { return main_->targetAvailable(name); }
	// If you want to fetch a specific output request it in the startDraw function (or any time before the target is written, not after!)
	// The request needs to be renewed for every new frame
//	virtual void requestOutput(const std::string & name) final { parent_->requestOutput(name); }
//	virtual void requestOutput(const std::string & name, int W, int H) final { parent_->requestOutput(name, W, H); }

	// What type does a certain output channel have?
//	virtual DataType outputType(const std::string & name) const final { return main_->outputType(name); }
//	virtual int outputChannels(const std::string & name) const final { return main_->outputChannels(name); }

	// You can only read targets in the endFrame function, calling it from anywhere else leads to undefined behavior
	// You need to match the datatype, or else an error will be thrown
	//virtual bool readTarget(const std::string & name, int W, int H, int C, DataType t, void * data) final { return main_->readTarget(name, W, H, C, t, data); }
	//virtual bool readTarget(const std::string & name, int W, int H, int C, half * data) final { return main_->readTarget(name, W, H, C, data); }
	//virtual bool readTarget(const std::string & name, int W, int H, int C, float * data) final { return main_->readTarget(name, W, H, C, data); }
	//virtual bool readTarget(const std::string & name, int W, int H, int C, uint8_t * data) final { return main_->readTarget(name, W, H, C, data); }
	//virtual bool readTarget(const std::string & name, int W, int H, int C, uint16_t * data) final { return main_->readTarget(name, W, H, C, data); }
	//virtual bool readTarget(const std::string & name, int W, int H, int C, uint32_t * data) final { return main_->readTarget(name, W, H, C, data); }

	// Buffer handling and reading
	//using BaseGameController::readBuffer;
	//virtual std::shared_ptr<GPUMemory> readBuffer(Buffer b, const std::vector<size_t> & offset, const std::vector<size_t> & n, bool immediate = false) final { return main_->readBuffer(b, offset, n, immediate); }
	//virtual size_t bufferSize(Buffer b) final { return main_->bufferSize(b); }

	// CBuffer handling
	//virtual std::shared_ptr<CBuffer> createCBuffer(const std::string & name, size_t max_size) final { return main_->createCBuffer(name, max_size); }
	//virtual void bindCBuffer(std::shared_ptr<CBuffer> b) final { main_->bindCBuffer(b); }

	//virtual void callPostFx(std::shared_ptr<Shader> shader) final { main_->callPostFx(shader); }
	/****** End Currently UNSUPPORTED ******/
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
PYBIND11_EMBEDDED_MODULE(api, m) {
	py::enum_<RecordingType>(m, "RecordingType")
		.value("NONE", RecordingType::NONE)
		.value("DRAW", RecordingType::DRAW)
		.value("DRAW_FIRST", RecordingType::DRAW_FIRST);

	py::enum_<DrawType>(m, "DrawType")
		.value("DEFAULT", DrawType::DEFAULT)
		.value("STATIC", DrawType::STATIC)
		.value("RIGID", DrawType::RIGID)
		.value("DYNAMIC", DrawType::DYNAMIC)
		.value("HIDE", DrawType::HIDE);

	py::class_<PythonControllerRef>(m, "__PythonControllerRef");

	py::class_<DrawInfo>(m, "DrawInfo")
		.def_readonly("type", &DrawInfo::type)
		.def_readonly("instances", &DrawInfo::instances)
		.def_readonly("n", &DrawInfo::n);

	py::class_<BasePythonController>(m, "BaseController")
		.def(py::init<PythonControllerRef>())
		.def("current_recording_type", &BasePythonController::currentRecordingType, py::call_guard<py::gil_scoped_release>())
		.def("copy_target", &BasePythonController::copyTarget, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("targets", &BasePythonController::listTargets, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("default_width", &BasePythonController::defaultWidth, py::call_guard<py::gil_scoped_release>())
		.def_property_readonly("default_height", &BasePythonController::defaultHeight, py::call_guard<py::gil_scoped_release>())
		.def("fetch_game_state", &BasePythonController::getGameState, py::call_guard<py::gil_scoped_release>())
		.def("send_command", &BasePythonController::sendCommand, py::call_guard<py::gil_scoped_release>())
		.def("key_down", &BasePythonController::keyDown, py::call_guard<py::gil_scoped_release>())
		.def("key_up", &BasePythonController::keyUp, py::call_guard<py::gil_scoped_release>())
		//.def("record_frame", &BasePythonController::recordFrame, py::call_guard<py::gil_scoped_release>())
		//.def("start_frame", &BasePythonController::startFrame, py::call_guard<py::gil_scoped_release>())
		//.def("post_process", &BasePythonController::postProcess, py::call_guard<py::gil_scoped_release>())
		//.def("end_frame", &BasePythonController::endFrame, py::call_guard<py::gil_scoped_release>())
		//.def("start_draw", &BasePythonController::startDraw, py::call_guard<py::gil_scoped_release>())
		//.def("end_draw", &BasePythonController::endDraw, py::call_guard<py::gil_scoped_release>())
		//.def("command", &BasePythonController::command, py::call_guard<py::gil_scoped_release>())
		//.def("game_state", &BasePythonController::gameState, py::call_guard<py::gil_scoped_release>())
		//.def("unload", &BasePythonController::unload, py::call_guard<py::gil_scoped_release>())
		;

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
	virtual bool keyDown(unsigned char key, unsigned char special_status) final {
		if (key == VK_F11) reloadAllModules();
		return callAll<bool>("key_down", key, special_status);
	}
	virtual bool keyUp(unsigned char key) final {
		return callAll<bool>("key_up", key);
	}
	virtual RecordingType recordFrame(uint32_t frame_id) final {
		return callAll<RecordingType>("record_frame", frame_id);
	}
	virtual void startFrame(uint32_t frame_id) final {
		callAll("start_frame", frame_id);
	}
	virtual void postProcess(uint32_t frame_id) final {
		callAll("post_processing", frame_id);
	}
	virtual void endFrame(uint32_t frame_id) final {
		callAll("end_frame", frame_id);
	}
	virtual DrawType startDraw(const DrawInfo & i) final {
		return callAll<DrawType>("start_draw", i);
	}
	virtual void endDraw(const DrawInfo & i) final {
		callAll("end_draw", i);
	}
	virtual void command(const std::string & json) {
		callAll("command", json);
	}
	virtual std::string gameState() const {
		py::gil_scoped_acquire acquire;
		std::string r = "{";
		for (auto c : controllers)
			if (py::hasattr(c, "game_state")) {
				py::object gs = c.attr("game_state");
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

RecordingType BasePythonController::currentRecordingType() const { return main_->currentRecordingType(); }
void BasePythonController::copyTarget(const std::string & to, const std::string & from) { main_->copyTarget(to, from); }
std::vector<std::string> BasePythonController::listTargets() const { return main_->listTargets(); }
int BasePythonController::defaultWidth() const { return main_->defaultWidth(); }
int BasePythonController::defaultHeight() const { return main_->defaultHeight(); }
std::string BasePythonController::getGameState() const { return main_->getGameState(); }
void BasePythonController::sendCommand(const std::string & json) { return main_->sendCommand(json); };

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		LOG(INFO) << "Python plugin turned on";
	}

	if (reason == DLL_PROCESS_DETACH) {
		LOG(INFO) << "Python plugin turned off";
	}
	return TRUE;
}

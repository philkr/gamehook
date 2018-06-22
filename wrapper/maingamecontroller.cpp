#include "maingamecontroller.h"
#include "util.h"
#include <map>
#include <memory>
#include <unordered_map>
#include <algorithm>

const std::multimap<int, std::shared_ptr<GameControllerFactory> > & factories();
MainGameController::MainGameController() {
}

void MainGameController::clearControllers() {
	controllers_.clear();
}

void MainGameController::startControllers() {
	controllers_.clear();
	// Create all the child controllers
	for (auto f : factories()) {
		std::shared_ptr<GameController>c(f.second->make(this));
		if (c)
			controllers_.push_back(c);
	}
	for (auto c : controllers_) {
		c->initialize();
		initController(c);
	}
}

template<typename T, typename ...Args> T callController(const std::vector<std::shared_ptr<GameController>> &controllers, T(GameController::*f)(Args...) const, const Args&... args) {
	T r = T(0);
	for (auto c : controllers) {
		T rr = (c.get()->*f)(args...);
		if (rr && r && rr != r)
			LOG(WARN) << "Multiple callbacks answered the call differently, chosing the highest priority and ignoring the rest!";
		else if (rr)
			r = rr;
	}
	return r;
}
template<typename T, typename ...Args> T callController(const std::vector<std::shared_ptr<GameController>> &controllers, T (GameController::*f)(Args...), const Args&... args) {
	T r = T(0);
	for (auto c : controllers) {
		T rr = (c.get()->*f)(args...);
		if (rr && r && rr != r)
			LOG(WARN) << "Multiple callbacks answered the call differently, chosing the highest priority and ignoring the rest!";
		else if (rr)
			r = rr;
	}
	return r;
}
template<typename ...Args> bool callControllerAll(const std::vector<std::shared_ptr<GameController>> &controllers, bool (GameController::*f)(Args...), const Args&... args) {
	bool r = true;
	for (auto c : controllers)
		if (!(c.get()->*f)(args...))
			r = false;
	return r;
}
template<typename ...Args> bool callController(const std::vector<std::shared_ptr<GameController>> &controllers, bool (GameController::*f)(Args...), const Args&... args) {
	bool r = false;
	for (auto c : controllers)
		if ((c.get()->*f)(args...))
			r = true;
	return r;
}
template<typename ...Args> bool callControllerReturn(const std::vector<std::shared_ptr<GameController>> &controllers, bool (GameController::*f)(Args...), const Args&... args) {
	for (auto c : controllers)
		if ((c.get()->*f)(args...))
			return true;
	return false;
}
template<typename ...Args> void callController(const std::vector<std::shared_ptr<GameController>> &controllers, void (GameController::*f)(Args...), const Args&... args) {
	for (auto c : controllers)
		(c.get()->*f)(args...);
}
template<typename ...Args> RecordingType callController(const std::vector<std::shared_ptr<GameController>> &controllers, RecordingType (GameController::*f)(Args...), const Args&... args) {
	uint32_t r = 0;
	for (auto c : controllers)
		r |= (uint32_t)(c.get()->*f)(args...);
	return (RecordingType)r;
}
bool MainGameController::keyDown(unsigned char key, unsigned char special_status) {
	return callControllerReturn(controllers_, &GameController::keyDown, key, special_status);
}
bool MainGameController::keyUp(unsigned char key) {
	return callControllerReturn(controllers_, &GameController::keyUp, key);
}
std::string TN(const std::string & s) { return s; }
std::string TN(const ProvidedTarget & s) { return s.name; }
template<typename T, typename HT> std::vector<T> setMerge(const std::vector<std::shared_ptr<GameController>> &controllers, std::vector<T>(GameController::*f)(void) const) {
	std::unordered_map<HT, std::pair<T,std::string> > set;
	for (auto c : controllers) {
		std::vector<T> t = (c.get()->*f)();
		std::string cn = typeid(c.get()).name();
		for (const T & s : t) {
			HT h = TN(s);
			if (set.count(h)) {
				LOG(WARN) << "Plugins '" << set[h].second << "' and '" << cn << "' both provide target '" << h << "'! This will not end well...";
			} else {
				set[h] = std::make_pair(s, cn);
			}
		}
	}
	std::vector<T> r;
	for (const auto & t : set)
		r.push_back(t.second.first);
	return r;
}
std::vector<ProvidedTarget> MainGameController::providedTargets() const {
	return setMerge<ProvidedTarget, std::string>(controllers_, &GameController::providedTargets);
}
std::vector<ProvidedTarget> MainGameController::providedCustomTargets() const {
	return setMerge<ProvidedTarget, std::string>(controllers_, &GameController::providedCustomTargets);
}
std::vector<ProvidedTarget> MainGameController::providedTargets(std::shared_ptr<GameController> c) const {
	return c->providedTargets();
}
std::vector<ProvidedTarget> MainGameController::providedCustomTargets(std::shared_ptr<GameController> c) const {
	return c->providedCustomTargets();
}

std::string MainGameController::gameState() const {
	std::string r = "{";
	for (auto c : controllers_) {
		std::string s = strip(c->gameState());
		if (s.size() > 2) {
			if (r.size() > 1) r += ",";
			r += s.substr(1, s.size() - 2);
		}
	}
	return r + "}";
}
void MainGameController::sendCommand(const std::string &s) {
	for (auto c : controllers_)
		c->command(s);
}
RecordingType MainGameController::recordFrame(uint32_t frame_id) {
	return callController(controllers_, &GameController::recordFrame, frame_id);
}
void MainGameController::startFrame(uint32_t frame_id) {
	return callController(controllers_, &GameController::startFrame, frame_id);
}
void MainGameController::postProcess(uint32_t frame_id) {
	callController(controllers_, &GameController::postProcess, frame_id);
}
void MainGameController::endFrame(uint32_t frame_id) {
	callController(controllers_, &GameController::endFrame, frame_id);
}
DrawType MainGameController::startDraw(const DrawInfo & i) {
	return callController<DrawType, const DrawInfo &>(controllers_, &GameController::startDraw, i);
}

void MainGameController::endDraw(const DrawInfo & i) {
	callController<const DrawInfo &>(controllers_, &GameController::endDraw, i);
}

std::shared_ptr<Shader> MainGameController::injectShader(std::shared_ptr<Shader> shader) {
	return callController<std::shared_ptr<Shader>, std::shared_ptr<Shader>>(controllers_, &GameController::injectShader, shader);
}

bool MainGameController::stop() {
	return callControllerAll(controllers_, &GameController::stop);
}

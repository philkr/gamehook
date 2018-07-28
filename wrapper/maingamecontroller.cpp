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
	for (auto c : controllers_)
		c->onClose();
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
		c->onInitialize();
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
bool MainGameController::onKeyDown(unsigned char key, unsigned char special_status) {
	return callControllerReturn(controllers_, &GameController::onKeyDown, key, special_status);
}
bool MainGameController::onKeyUp(unsigned char key) {
	return callControllerReturn(controllers_, &GameController::onKeyUp, key);
}
std::string MainGameController::gameState() const {
	std::string r = "{";
	for (auto c : controllers_) {
		std::string s = strip(c->provideGameState());
		if (s.size() > 2) {
			if (r.size() > 1) r += ",";
			r += s.substr(1, s.size() - 2);
		}
	}
	return r + "}";
}
void MainGameController::command(const std::string &s) {
	for (auto c : controllers_)
		c->onCommand(s);
}
void MainGameController::onBeginFrame(uint32_t frame_id) {
	return callController(controllers_, &GameController::onBeginFrame, frame_id);
}
void MainGameController::onPostProcess(uint32_t frame_id) {
	callController(controllers_, &GameController::onPostProcess, frame_id);
}
void MainGameController::onEndFrame(uint32_t frame_id) {
	callController(controllers_, &GameController::onEndFrame, frame_id);
}
void MainGameController::onPresent(uint32_t frame_id) {
	callController(controllers_, &GameController::onPresent, frame_id);
}
void MainGameController::onBeginDraw(const DrawInfo & i) {
	callController<const DrawInfo &>(controllers_, &GameController::onBeginDraw, i);
}

void MainGameController::onEndDraw(const DrawInfo & i) {
	callController<const DrawInfo &>(controllers_, &GameController::onEndDraw, i);
}

void MainGameController::onCreateShader(std::shared_ptr<Shader> shader) {
	callController(controllers_, &GameController::onCreateShader, shader);
}

void MainGameController::onBindShader(std::shared_ptr<Shader> shader) {
	callController(controllers_, &GameController::onBindShader, shader);
}

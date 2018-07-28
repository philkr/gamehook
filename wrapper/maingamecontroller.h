#pragma once
#include "api.h"
#include <memory>
#include <vector>

class MainGameController : public BaseGameController {
public:
	std::vector<std::shared_ptr<GameController>> controllers_;
	MainGameController();

	// Initialize the GameController (e.g. pass it shaders already loaded for in the fly loading)
	virtual void clearControllers();
	virtual void startControllers();

	// Callbacks
	virtual bool onKeyDown(unsigned char key, unsigned char special_status);
	virtual bool onKeyUp(unsigned char key);

	virtual void onBeginFrame(uint32_t frame_id) final;
	virtual void onPostProcess(uint32_t frame_id) final;
	virtual void onEndFrame(uint32_t frame_id) final;
	virtual void onPresent(uint32_t frame_id) final;

	virtual void onBeginDraw(const DrawInfo & i) final;
	virtual void onEndDraw(const DrawInfo & i) final;

	virtual void onCreateShader(std::shared_ptr<Shader> shader) final;
	virtual void onBindShader(std::shared_ptr<Shader> shader) final;

	// State functions
	virtual std::string gameState() const final;

	// Send command function
	virtual void command(const std::string &s) final;
};

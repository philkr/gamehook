#pragma once
#include "api.h"
#include <memory>
#include <vector>

class MainGameController : public BaseGameController {
public:
	std::vector<std::shared_ptr<GameController>> controllers_;
	MainGameController();

	// Initialize the GameController (e.g. pass it shaders already loaded for in the fly loading)
	virtual void initController(std::shared_ptr<GameController> c) = 0;
	virtual void clearControllers();
	virtual void startControllers();


	// Implements the main game controller (a dispatcher of all custom controllers)
	virtual void sendKeyDown(unsigned char key, bool syskey = false) = 0;
	virtual void sendKeyUp(unsigned char key, bool syskey = false) = 0;
	virtual const std::vector<uint8_t> & keyState() const = 0;

	// Callbacks
	virtual bool keyDown(unsigned char key, unsigned char special_status);
	virtual bool keyUp(unsigned char key);
	virtual RecordingType recordFrame(uint32_t frame_id) override;
	virtual void startFrame(uint32_t frame_id) override;
	virtual void endFrame(uint32_t frame_id) override;
	virtual void postProcess(uint32_t frame_id) override;
	virtual DrawType startDraw(const DrawInfo & i) override;
	virtual void endDraw(const DrawInfo & i) override;
	virtual std::shared_ptr<Shader> injectShader(std::shared_ptr<Shader> shader) override;
	virtual bool stop() override;

	// Other functions
	virtual std::vector<ProvidedTarget> providedTargets() const override;
	virtual std::vector<ProvidedTarget> providedCustomTargets() const override;
	virtual std::vector<ProvidedTarget> providedTargets(std::shared_ptr<GameController> c) const;
	virtual std::vector<ProvidedTarget> providedCustomTargets(std::shared_ptr<GameController> c) const;

	// State functions
	virtual std::string gameState() const final;

	// Send command function
	virtual void sendCommand(const std::string &s) final;
};


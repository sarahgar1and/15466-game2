#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;


	Scene::Transform *body = nullptr;
	glm::vec3 body_position;
	glm::vec3 body_scale;

	Scene::Transform *currFly = nullptr;
	glm::vec3 fly_position;
	glm::vec3 offScreen = glm::vec3(0.0f, 0.0f, -1000.0f); //add to pos vector to move off screen

	float wobble = 0.0f;

	enum Stage {
		BABY,
		TWEEN,
		ADULT
	};

	float get_scale_by_stage(Stage s);
	Stage stage = BABY;
	int score = 0;
	
	//camera:
	Scene::Camera *camera = nullptr;

};

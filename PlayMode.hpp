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

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;


	Scene::Transform *body = nullptr;
	glm::vec3 body_scale;

	Scene::Transform *currFly = nullptr;
	glm::vec3 offScreen = glm::vec3(0.0f, 0.0f, -1000.0f); //add to pos vector to move off screen
	glm::vec3 target_position;
	bool update_fly(bool update_curr_pos);
	glm::vec3 get_new_fly_position();
	float FlySpeed = 3.0f;
	glm::vec3 fly_direction;

	float wobble = 0.0f;

	int score = 0;
	
	//camera:
	Scene::Camera *camera = nullptr;

};

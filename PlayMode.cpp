#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint frog_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > frog_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("frog1.pnct"));
	frog_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > frog_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("frog1.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = frog_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = frog_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*frog_scene) {
	for (auto &transform : scene.transforms) {
		if (transform.name == "Body") body = &transform;
		if (transform.name == "Fly") currFly = &transform;
	}
	if (body == nullptr) throw std::runtime_error("Body not found.");
	if (currFly == nullptr) throw std::runtime_error("Fly not found.");

	body_position = body->position;
	body_scale = body->scale;

	// currFly->position = currFly->position + offScreen;
	fly_position = currFly->position;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

float PlayMode::get_scale_by_stage(Stage s){
	switch(s){
		case BABY:
			return 0.5f;
		case TWEEN:
			return 0.75f;
		case ADULT:
			return 1.0f;
		default:
			assert(false && "Should never reach this");
	}
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		float mouseX = evt.button.x;
		float mouseY = evt.button.y;

		std::cout << "mouseX: " << mouseX << std::endl;
		std::cout << "mouseY: " << mouseY << std::endl;

		// Convert fly position to clip space (See Scene.cpp draw(camera))
		glm::mat4 clip_from_world = camera->make_projection() * glm::mat4(camera->transform->make_local_from_world());
		glm::vec4 clip_space = clip_from_world * glm::vec4(fly_position, 1.0f);
		// Normalized Device Coordinates (See https://www.youtube.com/watch?v=pThw0S8MR7w&t=474s)
		glm::vec3 ndc;
		if (clip_space.w != 0.0f) {
			ndc.x = clip_space.x / clip_space.w;
			ndc.y = clip_space.y / clip_space.w;
			ndc.z = clip_space.z / clip_space.w;
		}
		float flyX = ((ndc.x + 1.0f) * 0.5f) * window_size.x;
		float flyY = ((1.0f - ndc.y) * 0.5f) * window_size.y;

		std::cout << "flyX: " << flyX << std::endl;
		std::cout << "flyY: " << flyY << std::endl;

		// Check "bbox" - not very precise but whatever
		if (flyX - 50.0f <= mouseX && mouseX <= flyX + 50.0f && 
			flyY - 50.0f <= mouseY && mouseY <= flyY + 50.0f ){
			
			score++;
			std::cout << "You caught a fly!" << std::endl;
		}
		
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	float squish = ((0.5f * std::sin(wobble * 2.0f * 2.0f * float(M_PI)) + 0.5f) + 1.5f) * 0.5f;
	if (squish < 1.0f) wobble = 0;
	// std::cout << squish << std::endl;

	body->scale = glm::vec3(body_scale.x, body_scale.y, body_scale.z * squish) * get_scale_by_stage(stage);

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

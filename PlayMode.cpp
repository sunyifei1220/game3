#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
GLuint hexapod_meshes_for_lit_color_texture_program = 0;
GLuint poop_meshes_for_lit_color_texture_program = 0;
Scene::Transform* default_transform;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});
Load< MeshBuffer > poop_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("poop.pnct"));
	poop_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
	});
Load< Scene > poop_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("poop.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
		default_transform = transform;

	});
});
Load< Sound::Sample > poop_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("poop.wav"));
});

void add_poop(Scene & scene, Scene::Transform * transform) {
	transform->scale  *= 0.1;
	Mesh const& mesh = poop_meshes->lookup("poop");
	scene.drawables.emplace_back(transform);
	Scene::Drawable& drawable = scene.drawables.back();
	
	drawable.pipeline = lit_color_texture_program_pipeline;

	drawable.pipeline.vao = poop_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = mesh.type;
	drawable.pipeline.start = mesh.start;
	drawable.pipeline.count = mesh.count;
}
PlayMode::PlayMode() : scene(*poop_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	Sound::loop_3D(*poop_sample, 2.0f, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f);

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	static bool create_poop = false;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}else if (evt.key.keysym.sym == SDLK_SPACE) {
			if (!changed) {
				total_score -= 30;
				return false;
			}
			else if (!create_poop) {
				Scene::Transform* transform = new Scene::Transform();
				transform->scale = default_transform->scale;
				transform->position = camera->transform->position;
				transform->position.y += 7.0f;
				transform->position.x -= 7.0f;
				transform->position.z -= 5.0f;
				add_poop(scene,transform);
				create_poop = true;
			}
			space.downs += 1;
			space.pressed = true;
			space.released = false;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) { 
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			create_poop = false; 
			space.pressed = false;
			space.released = true;
			return true;
		}
	}
	return false;
}

void PlayMode::update(float elapsed) {
	static long start = static_cast<long>(time(NULL));
	long curr = static_cast<long>(time(NULL));
	if (curr - start <= 1) {
		space.downs = 0;
		return;
	}
	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		if(space.pressed) {
			Scene::Transform * transform = new Scene::Transform();
			float scale = (space.downs) * 0.1f;
			transform->scale = glm::vec3(scale, scale, 0.5 * scale);
			if (!scene.drawables.empty()) {
				auto drawable = scene.drawables.back();
				drawable.transform->scale += glm::vec3(scale, scale, 0.5 * scale);
				
				if (changed) total_score += 10;
			}
			space.downs = 0;
		}
		
		set_duration(); 
		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

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
		lines.draw_text("Hold Space when volumes up, release when volumes down. WASD to Move. Totol score: " + std::to_string(total_score),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Hold Space when volumes up, release when volumes down. WASD to Move. Totol score: " + std::to_string(total_score),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

void PlayMode::set_duration() {
	long curr = static_cast<long>(time(NULL));
	static long start = 0, end = curr;
	
	if (!changed) {
		if (curr - end < 3) return; // leave at least 3s as a buffer
		int to_change = rand() % 3; 
		if (!to_change) { // 1/3 chance to change
			duration = (uint16_t)rand() % 5 + 1;
			start = curr;
			change_volume();
			changed = true;
		}
	}
	else {
		if (curr - start >= duration) {
			Sound::set_volume(1.0f);
			duration = 0;
			end = curr;
			changed = false;
		}
	}
}

void PlayMode::change_volume() {
	int index = rand() % 4;
	float volume = volumes[index];
	Sound::set_volume(volume);
}
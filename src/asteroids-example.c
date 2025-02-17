/* ####################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : asteroids-example
# @created     : 30/01/2025
  #################################################################### */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#include "raylib.h"
#include "raymath.h"

const int screen_width = 800;
const int screen_height = 800;
const Vector2 screen_center = {(float) screen_width / 2, (float) screen_height / 2};
#define DRAW_FPS false
#define DRAW_FPS_AVG_SAMPLES 20
#define DRAW_FRAMETIME true
#define DRAW_FRAMETIME_AVG_SAMPLES 5000

#define ECS_VERSION "whisker_ecs_v1.h"
#include ECS_VERSION

typedef enum GAME_STATE
{
	GAME_STATE_PLAYING,
	GAME_STATE_END,
} GAME_STATE;

GAME_STATE game_state;

float game_time_started;
float game_time_ended;

void game_init();
void game_end();
void game_update();
void game_draw_frame();


/*********************
*  game components  *
*********************/
typedef enum ASTEROID_SIZE  
{
	ASTEROID_SIZE_SMALL = 1,
	ASTEROID_SIZE_MEDIUM = 2,
	ASTEROID_SIZE_LARGE = 4,
} ASTEROID_SIZE;
ASTEROID_SIZE asteroid_sizes[] = {ASTEROID_SIZE_SMALL, ASTEROID_SIZE_MEDIUM, ASTEROID_SIZE_LARGE};

typedef enum PLAYER_STATE  
{
	PLAYER_STATE_DEFAULT,
	PLAYER_STATE_HIT,
	PLAYER_STATE_COOLDOWN,
	PLAYER_STATE_DEAD,
} PLAYER_STATE;

#define ASTEROID_ROTATION_VELOCITY_MIN 5 
#define ASTEROID_ROTATION_VELOCITY_MAX 240
#define ASTEROID_VELOCITY_MIN 150 
#define ASTEROID_VELOCITY_MAX 300
#define ASTEROID_RANDOM_ANGLE 30 * DEG2RAD
#define ASTEROID_OFF_SCREEN_PAD 128 
#define ASTEROID_SPAWN_RATE 0.6f 
#define ASTEROID_SPAWN_START 2 
#define ASTEROID_RADIUS 16 
#define ASTEROID_SCORE 10 
#define ASTEROID_DAMAGE 15 
#define ASTEROID_HIT_VELOCITY_REDUCTION 0.4f

#define PLAYER_ROTATION_VELOCITY 360 
#define PLAYER_VELOCITY 250
#define PLAYER_ACCELERATION 350 
#define PLAYER_DECELLERATION 0.8 
#define PLAYER_RADIUS 32 
#define PLAYER_START_LIFE 100
#define PLAYER_HIT_COOLDOWN 1.15f
#define PLAYER_HIT_NUDGE_FORCE 150.0f

#define PROJECTILE_VELOCITY 500 
#define PROJECTILE_WIDTH 10.0f 
#define PROJECTILE_LENGTH 30.0f 
#define PROJECTILE_FIRE_RATE 0.4f 

#define COMPONENT_POSITION 1 
#define COMPONENT_VELOCITY 2
#define COMPONENT_ROTATION 3
#define COMPONENT_ROTATION_VELOCITY 4
#define COMPONENT_ASTEROID_SIZE 5
#define COMPONENT_CREATION_TIME 6
#define COMPONENT_RADIUS_SIZE 7
#define COMPONENT_COLLISION 8
typedef struct component_collision
{
	size_t entity_a;
	size_t entity_b;
} component_collision;

#define COMPONENT_SCORE 9 
#define COMPONENT_LIFE 10 
#define COMPONENT_PLAYER_STATE 11 
#define COMPONENT_HIT_TIME 12 
#define COMPONENT_HIT_COLLISION 13 
#define COMPONENT_FPS 14 
typedef struct component_fps
{
	int samples[DRAW_FPS_AVG_SAMPLES];
	double time;
	int index;
	int max_index;
	int max;
	int min;
} component_fps;
#define COMPONENT_FRAMETIME 15 
typedef struct component_frametime
{
	float samples[DRAW_FRAMETIME_AVG_SAMPLES];
	double time;
	int index;
	int max_index;
	float max;
	float min;
} component_frametime;

#define COMPONENT_TAG_PLAYER COMPONENT_MAX-1 
#define COMPONENT_TAG_ASTEROID COMPONENT_MAX-2 
#define COMPONENT_TAG_PROJECTILE COMPONENT_MAX-3 
#define COMPONENT_TAG_DESTROY_OFFSCREEN COMPONENT_MAX-4 
#define COMPONENT_TAG_SCREEN_WRAP COMPONENT_MAX-5 
#define COMPONENT_TAG_MOVE_DIRECTION COMPONENT_MAX-6 
#define COMPONENT_TAG_DESTROY COMPONENT_MAX-7 
#define COMPONENT_TAG_ASTEROID_HIT COMPONENT_MAX-8 

#define NEARBLACK CLITERAL(Color){15, 15, 15, 266} 

Texture2D player_ship;

/********************
*  game functions  *
********************/
void spawn_asteroid();
void add_asteroid(Vector2 position, Vector2 velocity, float rotation, float rotation_velocity, ASTEROID_SIZE size);
void create_player_entity();

// systems
void system_movement_direction(float delta_time);
void system_movement_velocity(float delta_time);
void system_rotation_velocity(float delta_time);
void system_collision(float delta_time);
void system_collision_cull(float delta_time);
void system_destroy_offscreen(float delta_time);
void system_screen_wrap(float delta_time);
void system_entity_deferred_destroy(float delta_time);

void system_asteroid_spawn(float delta_time);
void system_player_controller(float delta_time);
void system_projectile_collide_destroy(float delta_time);
void system_asteroid_respawn_on_hit(float delta_time);
void system_asteroid_score(float delta_time);
void system_asteroid_hit_asteroid(float delta_time);
void system_player_hit_asteroid(float delta_time);
void system_player_hit_nudge(float delta_time);
void system_player_hit_cooldown(float delta_time);
void system_player_hit_to_recover(float delta_time);
void system_player_death_on_life_depleted(float delta_time);

void system_draw_asteroid(float delta_time);
void system_draw_projectile(float delta_time);
void system_draw_player(float delta_time);
void system_draw_hud(float delta_time);
void system_draw_game_over(float delta_time);
void system_draw_fps(float delta_time);
void system_draw_frame_time(float delta_time);


int main(int argc, char** argv)
{
	srand(time(0));
	
	InitWindow(screen_width, screen_height, "Asteroids");

	// load textures
	player_ship = LoadTexture("assets/ship_2.png");

	game_init();

	while (!WindowShouldClose())
	{
		game_update();
		game_draw_frame();
	}

	deinit_ecs();

	CloseWindow();

    return 0;
}

void game_init()
{
	game_state = GAME_STATE_PLAYING;
	game_time_started = GetTime();

	init_ecs();

	// create player entity
	create_player_entity();

	// spawn asteroids
	for (int i = 0; i < ASTEROID_SPAWN_START; ++i)
	{
		spawn_asteroid();
	}

	// update
	register_system(system_asteroid_spawn);
	register_system(system_movement_velocity);
	register_system(system_rotation_velocity);
	register_system(system_player_controller);
	register_system(system_movement_direction);
	register_system(system_collision);
	register_system(system_destroy_offscreen);
	register_system(system_screen_wrap);
	register_system(system_collision_cull);
	register_system(system_projectile_collide_destroy);
	register_system(system_asteroid_respawn_on_hit);
	register_system(system_asteroid_score);
	register_system(system_asteroid_hit_asteroid);
	register_system(system_player_hit_asteroid);
	register_system(system_player_hit_nudge);
	register_system(system_player_hit_cooldown);
	register_system(system_player_death_on_life_depleted);

	// draw
	register_system(system_draw_asteroid);
	register_system(system_draw_projectile);
	register_system(system_draw_player);
	register_system(system_draw_hud);
	register_system(system_draw_game_over);

	register_system(system_player_hit_to_recover);
	register_system(system_entity_deferred_destroy);

	if (DRAW_FPS)
	{
		register_system(system_draw_fps);
	}
	if (DRAW_FRAMETIME)
	{
		register_system(system_draw_frame_time);
	}
}
void game_end()
{
	game_state = GAME_STATE_END;
	game_time_ended = GetTime();
}

void game_update()
{
	if (game_state == GAME_STATE_END && IsKeyPressed(KEY_R)) 
	{
		game_init();
	}

	return;
}

void game_draw_frame()
{
	BeginDrawing();

	ClearBackground(NEARBLACK);

	// draw game
	/* draw_asteroid(GetFrameTime()); */
	update_systems(GetFrameTime());
	
	if (game_state == GAME_STATE_END) 
	{
		// game end
	}
	else
	{
		// game running
	}

	EndDrawing();
}

void create_player_entity()
{
	size_t e = create_entity();

	Vector2 position = screen_center;

	SET_COMPONENT(COMPONENT_POSITION, Vector2, e, &(position));
	SET_COMPONENT(COMPONENT_VELOCITY, Vector2, e, &((Vector2) { }));
	SET_COMPONENT(COMPONENT_RADIUS_SIZE, float, e, (void*)&((float){PLAYER_RADIUS * 0.66f}));
	SET_COMPONENT(COMPONENT_ROTATION, float, e, (void*)&(float){180});
	SET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, e, (void*)&(float){0});
	SET_COMPONENT(COMPONENT_CREATION_TIME, double, e, (void*)&(double){GetTime()});
	SET_COMPONENT(COMPONENT_SCORE, int, e, (void*)&(int){0});
	SET_COMPONENT(COMPONENT_LIFE, int, e, (void*)&(int){PLAYER_START_LIFE});
	SET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e, (void*)&(PLAYER_STATE){PLAYER_STATE_DEFAULT});
	SET_COMPONENT(COMPONENT_HIT_TIME, double, e, (void*)&(double){0});
	SET_COMPONENT(COMPONENT_HIT_COLLISION, component_collision, e, (void*)&(component_collision){});

	// TODO: store this on a central game/world entity instead of the player
	SET_COMPONENT(COMPONENT_FPS, component_fps, e, (void*)&(component_fps){0});
	SET_COMPONENT(COMPONENT_FRAMETIME, component_frametime, e, (void*)&(component_frametime){0});
	add_component_entity(COMPONENT_TAG_PLAYER, e);    
	add_component_entity(COMPONENT_TAG_SCREEN_WRAP, e);    
}

void spawn_asteroid()
{
	// choose spawn position off screen
	Vector2 position = {
		.x = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, screen_width + ASTEROID_OFF_SCREEN_PAD)),
		.y = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, screen_height + ASTEROID_OFF_SCREEN_PAD)),
	};

	// set the position randomly to the left/right/top/bottom of the screen
	if (GetRandomValue(0, 1))
	{
		position.x = (position.x > screen_center.x) ? screen_width + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD;
	}
	else
	{
		position.y = (position.y > screen_center.y) ? screen_height + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD;
	}

	// set random velocity angle
	Vector2 velocity = Vector2Subtract(screen_center, position);
	velocity = Vector2Scale(Vector2Normalize(velocity), GetRandomValue(ASTEROID_VELOCITY_MIN, ASTEROID_VELOCITY_MAX));
	velocity = Vector2Rotate(velocity, (float) GetRandomValue(-ASTEROID_RANDOM_ANGLE, ASTEROID_RANDOM_ANGLE));

	// set random size
	int size_i = (ASTEROID_SIZE)rand() % 3;
	ASTEROID_SIZE size = asteroid_sizes[size_i];

	add_asteroid(position, velocity, (float)(rand() % 360), (float)GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), size);
}

void add_asteroid(Vector2 position, Vector2 velocity, float rotation, float rotation_velocity, ASTEROID_SIZE size)
{
	// create an entity id
	size_t e = create_entity();

	// set the entity component data
	SET_COMPONENT(COMPONENT_POSITION, Vector2, e, &(position));
	SET_COMPONENT(COMPONENT_VELOCITY, Vector2, e, &(velocity));
	SET_COMPONENT(COMPONENT_ASTEROID_SIZE, ASTEROID_SIZE, e, (void*)&size);
	SET_COMPONENT(COMPONENT_RADIUS_SIZE, float, e, (void*)&((float){(ASTEROID_RADIUS * 0.6f) * size}));
	SET_COMPONENT(COMPONENT_ROTATION, float, e, (void*)&rotation);
	SET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, e, (void*)&(rotation_velocity));
	SET_COMPONENT(COMPONENT_CREATION_TIME, double, e, (void*)&(double){GetTime()});
	add_component_entity(COMPONENT_TAG_ASTEROID, e);    
	add_component_entity(COMPONENT_TAG_DESTROY_OFFSCREEN, e);    

	printf("add_asteroid:size %d at %fx%f\n", size, position.x, position.y);
}

void add_projectile(Vector2 position, float rotation)
{
	// create an entity id
	size_t e = create_entity();

	// set the entity component data
	SET_COMPONENT(COMPONENT_POSITION, Vector2, e, &(position));
	SET_COMPONENT(COMPONENT_ROTATION, float, e, (void*)&rotation);
	SET_COMPONENT(COMPONENT_RADIUS_SIZE, float, e, (void*)&((float){((PROJECTILE_WIDTH + PROJECTILE_LENGTH) * 2) / 4}));

	// TODO: this is only because the recycled entity's data still exists and is
	// updated by the movement system because it looks for a POSITION component
	SET_COMPONENT(COMPONENT_VELOCITY, Vector2, e, &((Vector2){}));

	SET_COMPONENT(COMPONENT_CREATION_TIME, double, e, (void*)&(double){GetTime()});

	add_component_entity(COMPONENT_TAG_PROJECTILE, e);    
	add_component_entity(COMPONENT_TAG_MOVE_DIRECTION, e);    
	add_component_entity(COMPONENT_TAG_DESTROY_OFFSCREEN, e);    

	printf("add_projectile:rot %f at %fx%f\n", rotation, position.x, position.y);
}

/********************
*  update systems  *
********************/
void system_movement_velocity(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_POSITION, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2* position = GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		Vector2* velocity = GET_COMPONENT(COMPONENT_VELOCITY, Vector2, e);		

		position->x += velocity->x * delta_time;
		position->y += velocity->y * delta_time;
	}
}

void system_rotation_velocity(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_ROTATION_VELOCITY, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		float* rotation = GET_COMPONENT(COMPONENT_ROTATION, float, e);		
		float* rotation_velocity = GET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, e);		

		*rotation += *rotation_velocity * delta_time;
	}
}

void system_destroy_offscreen(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_DESTROY_OFFSCREEN, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2 position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		if (
				position.x < -(ASTEROID_OFF_SCREEN_PAD) ||
				position.y < -(ASTEROID_OFF_SCREEN_PAD) ||
				position.x > (screen_width + ASTEROID_OFF_SCREEN_PAD) ||
				position.y > (screen_height + ASTEROID_OFF_SCREEN_PAD)
				)
		{
			printf("system:destroy_offscreen:e = %d @ %d\n", e, i);
			add_component_entity(COMPONENT_TAG_DESTROY, e);    
		}
	}
}

void system_movement_direction(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_MOVE_DIRECTION, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2* position = GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		float rotation = *GET_COMPONENT(COMPONENT_ROTATION, float, e);		

		float radians = DEG2RAD * (rotation + 90.0f);
		position->x += PROJECTILE_VELOCITY * cos(radians) * delta_time;
		position->y += PROJECTILE_VELOCITY * sin(radians) * delta_time;
	}
}

void system_asteroid_spawn(float delta_time)
{
	// get the youngest asteroid creation time
	size_t asteroid_component_entities[ENTITY_MAX] = {};
	size_t asteroid_entity_length = set_component_entities(COMPONENT_TAG_ASTEROID, asteroid_component_entities);
	double last_asteroid_spawned = -ASTEROID_SPAWN_RATE;

	for (size_t ii = 0; ii < asteroid_entity_length; ++ii)
	{
		size_t pe = asteroid_component_entities[ii];

		double creation_time = *GET_COMPONENT(COMPONENT_CREATION_TIME, double, pe);		

		if (creation_time >= last_asteroid_spawned)
		{
			last_asteroid_spawned = creation_time;
		}
	}

	bool can_spawn = (GetTime() > last_asteroid_spawned + ASTEROID_SPAWN_RATE);

	if (can_spawn)
	{
		spawn_asteroid();
	}
}

void system_player_controller(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_PLAYER, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);
		if (*player_state == PLAYER_STATE_DEAD && IsKeyPressed(KEY_R)) {
			game_init();
			break;
		}
		if (*player_state != PLAYER_STATE_DEFAULT) {
			continue;
		}

		// player rotation
		float* rotation = GET_COMPONENT(COMPONENT_ROTATION, float, e);		
		float* rotation_velocity = GET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, e);		

		int rotation_input = (int)IsKeyDown(KEY_D) - (int)IsKeyDown(KEY_A);
		/* *rotation += (rotation_input * PLAYER_ROTATION_VELOCITY * delta_time); */
		*rotation_velocity = PLAYER_ROTATION_VELOCITY * rotation_input;

		// player thrust
		Vector2* velocity = GET_COMPONENT(COMPONENT_VELOCITY, Vector2, e);		

		Vector2 facing_direction = Vector2Rotate((Vector2){0, 1}, *rotation * DEG2RAD);

		int thrust_input = (int)IsKeyDown(KEY_W) - (int)IsKeyDown(KEY_S);
		if (thrust_input > 0) {
			*velocity = Vector2Add(*velocity, Vector2Scale(facing_direction, PLAYER_ACCELERATION * delta_time));

			float mag = Vector2Length(*velocity);
			if (mag > PLAYER_VELOCITY) {
				*velocity = Vector2Scale(*velocity, PLAYER_VELOCITY / mag);
			}
		}
		else
		{
			*velocity = Vector2Add(*velocity, Vector2Scale(*velocity, -PLAYER_DECELLERATION * delta_time));
		}

		// projectiles
		Vector2 position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		

		// get the youngest projectile creation time
		size_t projectile_component_entities[ENTITY_MAX] = {};
		size_t projectile_entity_length = set_component_entities(COMPONENT_TAG_PROJECTILE, projectile_component_entities);
		double last_projectile_fired = -PROJECTILE_FIRE_RATE;

		for (size_t ii = 0; ii < projectile_entity_length; ++ii)
		{
			size_t pe = projectile_component_entities[ii];

			double creation_time = *GET_COMPONENT(COMPONENT_CREATION_TIME, double, pe);		

			if (creation_time >= last_projectile_fired)
			{
				last_projectile_fired = creation_time;
			}
		}

		bool can_fire = (GetTime() > last_projectile_fired + PROJECTILE_FIRE_RATE);

		if (IsKeyDown(KEY_SPACE) && can_fire) {
			add_projectile(Vector2Add(position, Vector2Scale(facing_direction, 24)), *rotation);
		}
	}
}

void system_screen_wrap(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_SCREEN_WRAP, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2* position = GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		if (
				position->x < 0 ||
				position->y < 0 ||
				position->x > screen_width ||
				position->y > screen_height
				)
		{
			printf("system:screen_wrap:e = %d @ %d\n", e, i);

			if (position->x < 0)
			{
				position->x += screen_width;
			}
			if (position->x > screen_width)
			{
				position->x -= screen_width;
			}
			if (position->y < 0)
			{
				position->y += screen_height;
			}
			if (position->y > screen_height)
			{
				position->y -= screen_height;
			}
		}
	}
}

void system_collision(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};

	size_t entity_length = set_component_entities(COMPONENT_RADIUS_SIZE, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2 position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		float radius_size = *GET_COMPONENT(COMPONENT_RADIUS_SIZE, float, e);		

		size_t colliding_component_entities[ENTITY_MAX] = {};
		size_t colliding_entity_length = set_component_entities(COMPONENT_POSITION, colliding_component_entities);

		for (size_t ci = 0; ci < colliding_entity_length; ++ci)
		{
			size_t ce = colliding_component_entities[ci];

			if (e == ce) {
				continue;
			}

			Vector2 colliding_position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, ce);		
			float colliding_radius_size = *GET_COMPONENT(COMPONENT_RADIUS_SIZE, float, ce);		

    		float distance = Vector2Distance(position, colliding_position);
    		if (distance <= (radius_size + colliding_radius_size))
    		{
				/* printf("system:collision:%zu->%zu (on)\n", e, ce); */

				/* DrawCircle(position.x, position.y, radius_size, Fade(GREEN, 0.6f)); */
				/* DrawCircle(colliding_position.x, colliding_position.y, colliding_radius_size, Fade(BLUE, 0.6f)); */

				size_t collision_entity = create_entity();

				component_collision col = {e, ce};
				SET_COMPONENT(COMPONENT_COLLISION, component_collision, collision_entity, &col);
    		}
		}
	}
}

void system_collision_cull(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_COLLISION, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		add_component_entity(COMPONENT_TAG_DESTROY, e);    
	}
}

void system_asteroid_respawn_on_hit(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_ASTEROID_HIT, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		ASTEROID_SIZE* size = GET_COMPONENT(COMPONENT_ASTEROID_SIZE, ASTEROID_SIZE, e);
		Vector2* position = GET_COMPONENT(COMPONENT_POSITION, Vector2, e);

		int spawn_count = 0;

		ASTEROID_SIZE new_size = ASTEROID_SIZE_LARGE;

		switch (*size) {
			case ASTEROID_SIZE_LARGE:
				new_size = ASTEROID_SIZE_MEDIUM;
				spawn_count = 3;
				break;
			case ASTEROID_SIZE_MEDIUM:
				new_size = ASTEROID_SIZE_SMALL;
				spawn_count = 2;
				break;
		}

		printf("system:asteroid_respawn_on_destroy:spawn count %d from size %d\n", spawn_count, *size);

		for (int ii = 0; ii < spawn_count; ++ii)
		{
			printf("system:asteroid_respawn_on_destroy:spawning (from %zu) from size %d\n", e, spawn_count, *size);

			add_asteroid(*position, (Vector2) {GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2)), GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2))}, GetRandomValue(-360, 360) * DEG2RAD, GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), new_size);
		}
	}
}

void system_asteroid_score(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_ASTEROID_HIT, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		ASTEROID_SIZE* size = GET_COMPONENT(COMPONENT_ASTEROID_SIZE, ASTEROID_SIZE, e);

		// get score entity
		size_t score_component_entities[ENTITY_MAX] = {};
		size_t score_entity_length = set_component_entities(COMPONENT_SCORE, score_component_entities);
		for (size_t si = 0; si < score_entity_length; ++si)
		{
			size_t se = score_component_entities[si];

			int add_score = (int)*size * ASTEROID_SCORE;

			int *score = GET_COMPONENT(COMPONENT_SCORE, int, se);
			*score += add_score;

			printf("system:asteroid_score:+%d points for %zu (%d total)\n", add_score, se, *score);
		}
	}
}

void system_projectile_collide_destroy(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_COLLISION, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		component_collision collision = *GET_COMPONENT(COMPONENT_COLLISION, component_collision, e);		


		// find projectiles colliding with asteroids
		if (
				// projectile -> asteroid
				(has_component_entity(COMPONENT_TAG_PROJECTILE, collision.entity_a) && has_component_entity(COMPONENT_TAG_ASTEROID, collision.entity_b))
				)
		{
			printf("system:projectile_collide_destroy:%zu hit asteroid %zu\n", collision.entity_a, collision.entity_b);

			// destroy existing asteroid and projectile
			add_component_entity(COMPONENT_TAG_DESTROY, collision.entity_a);    
			add_component_entity(COMPONENT_TAG_DESTROY, collision.entity_b);    

			add_component_entity(COMPONENT_TAG_ASTEROID_HIT, collision.entity_b);    
		}
	}
}

void system_player_hit_asteroid(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_COLLISION, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		component_collision* collision = GET_COMPONENT(COMPONENT_COLLISION, component_collision, e);		


		// find asteroids that collided with the player
			if (
					// asteroid -> player
					(has_component_entity(COMPONENT_TAG_ASTEROID, collision->entity_a) && has_component_entity(COMPONENT_TAG_PLAYER, collision->entity_b))
					)
			{
				PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, collision->entity_b);

				// deal damage to the player, only in the default state
				// TODO: maybe figure out a more ECS-like way to handle this,
				// e.g. a component COMPONENT_TAG_PLAYER_STATE_DEFAULT?
				if (*player_state != PLAYER_STATE_DEFAULT) {
					continue;
				}

				*player_state = PLAYER_STATE_HIT;
				SET_COMPONENT(COMPONENT_HIT_TIME, double, collision->entity_b, (void*)&(double){GetTime()});
				component_collision col = {collision->entity_a, collision->entity_b};
				SET_COMPONENT(COMPONENT_HIT_COLLISION, component_collision, collision->entity_b, &col);

				// TODO: refactor this into a separate system
				int* player_life = GET_COMPONENT(COMPONENT_LIFE, int, collision->entity_b);
				ASTEROID_SIZE* asteroid_size = GET_COMPONENT(COMPONENT_ASTEROID_SIZE, ASTEROID_SIZE, collision->entity_a);

				int damage = *asteroid_size * ASTEROID_DAMAGE;

				Vector2* asteroid_velocity = GET_COMPONENT(COMPONENT_VELOCITY, Vector2, collision->entity_a);
				*asteroid_velocity = Vector2Scale(*asteroid_velocity, ASTEROID_HIT_VELOCITY_REDUCTION);
				float* rotation_velocity = GET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, collision->entity_a);		
				*rotation_velocity += (GetRandomValue(-270, 270));

				printf("system:player_damage:%zu hit player %zu (%d damage)\n", collision->entity_a, collision->entity_b, damage);

				*player_life -= damage;
				if (*player_life <= 0) {
					*player_life = 0;
				}
			}
	}
}

void system_asteroid_hit_asteroid(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_COLLISION, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		component_collision* collision = GET_COMPONENT(COMPONENT_COLLISION, component_collision, e);		


		// find asteroids that collided with the player
			if (
					// asteroid -> asteroid
					(has_component_entity(COMPONENT_TAG_ASTEROID, collision->entity_a) && has_component_entity(COMPONENT_TAG_ASTEROID, collision->entity_b))
					)
			{

				// asteroid a
				Vector2* asteroida_velocity = GET_COMPONENT(COMPONENT_VELOCITY, Vector2, collision->entity_a);
				float* asteroida_rotation_velocity = GET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, collision->entity_a);		

				*asteroida_rotation_velocity = (GetRandomValue(-180, 180));

				Vector2 asteroida_position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, collision->entity_a);		
				Vector2 asteroida_hit_by_position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, collision->entity_b);		

				Vector2 asteroida_nudge_direction = Vector2Normalize(Vector2Subtract(asteroida_position, asteroida_hit_by_position));
				*asteroida_velocity = Vector2Scale(asteroida_nudge_direction, PLAYER_HIT_NUDGE_FORCE);


				// asteroid b
				Vector2* asteroidb_velocity = GET_COMPONENT(COMPONENT_VELOCITY, Vector2, collision->entity_b);
				float* asteroidb_rotation_velocity = GET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, collision->entity_b);		

				*asteroidb_rotation_velocity = (GetRandomValue(-180, 180));

				/* printf("system:asteroid_hit_asteroid:%zu hit asteroid %zu\n", collision->entity_a, collision->entity_b); */
			}
	}
}

void system_player_hit_cooldown(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_PLAYER_STATE, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);
		double* hit_time = GET_COMPONENT(COMPONENT_HIT_TIME, double, e);

		if (*player_state == PLAYER_STATE_COOLDOWN && *hit_time + PLAYER_HIT_COOLDOWN < GetTime()) {
			printf("system:player_hit_cooldown:returning to default state\n");
			*player_state = PLAYER_STATE_DEFAULT;
		}
	}
}

void system_player_death_on_life_depleted(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_PLAYER_STATE, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);
		int* player_life = GET_COMPONENT(COMPONENT_LIFE, int, e);

		if (*player_state == PLAYER_STATE_DEFAULT && *player_life <= 0) {
			printf("system:player_death_on_life_depleted\n");
			REMOVE_COMPONENT(COMPONENT_VELOCITY, Vector2, e);
			*player_state = PLAYER_STATE_DEAD;
		}
	}
}

void system_player_hit_nudge(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_PLAYER_STATE, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);

		if (*player_state == PLAYER_STATE_HIT) {
			printf("system:player_hit_nudge:nudging player from the hit\n");

			component_collision* collision = GET_COMPONENT(COMPONENT_HIT_COLLISION, component_collision, e);		
			Vector2 position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
			Vector2 hit_by_position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, collision->entity_a);		

			Vector2 nudge_direction = Vector2Normalize(Vector2Subtract(position, hit_by_position));

			Vector2* velocity = GET_COMPONENT(COMPONENT_VELOCITY, Vector2, e);		
			float* rotation_velocity = GET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, e);		
			*rotation_velocity += (GetRandomValue(-270, 270));

			printf("system:player_hit_nudge:velocity before %fx%f\n", velocity->x, velocity->y);
			*velocity = Vector2Scale(nudge_direction, PLAYER_HIT_NUDGE_FORCE);

			printf("system:player_hit_nudge:velocity after %fx%f\n", velocity->x, velocity->y);
		}
	}
}

void system_player_hit_to_recover(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_PLAYER_STATE, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);
		if (*player_state == PLAYER_STATE_HIT) {
			printf("system:player_hit_to_recover\n");
			*player_state = PLAYER_STATE_COOLDOWN;
		}
	}
}

void system_entity_deferred_destroy(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_DESTROY, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		remove_entity(e);
	}
}

/******************
*  draw systems  *
******************/
void system_draw_asteroid(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_ASTEROID, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2* position = GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		float* rotation = GET_COMPONENT(COMPONENT_ROTATION, float, e);		
		ASTEROID_SIZE* size = GET_COMPONENT(COMPONENT_ASTEROID_SIZE, ASTEROID_SIZE, e);		

		DrawPolyLines(*position, 3, 16 * (int)*size, *rotation, WHITE);
		/* DrawText(TextFormat("%d", e), position->x, position->y, 32, WHITE); */
	}
}
void system_draw_projectile(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_PROJECTILE, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		Vector2 position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		float rotation = *GET_COMPONENT(COMPONENT_ROTATION, float, e);		

		Rectangle rect = { position.x, position.y, PROJECTILE_WIDTH, PROJECTILE_LENGTH };
		Vector2 origin = { rect.width / 2, rect.height / 2 };
		DrawRectanglePro(rect, origin, rotation, RED);
	}
}
void system_draw_hud(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_SCORE, component_entities);

	const int font_size = 64;
	const Color black = Fade(BLACK, 0.8f);
	const Color white = Fade(WHITE, 0.8f);
	const int x_pad = 16;
	const int y_pad = 8;
	const int shadow = 8;

	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);
		if (*player_state == PLAYER_STATE_DEAD)
		{
			continue;
		}

		// score
		int score = *GET_COMPONENT(COMPONENT_SCORE, int, e);		

		char* score_text = TextFormat("%d POINTS", score);
		float score_measure = MeasureText(score_text, font_size);

		int score_x = screen_width - score_measure;
		int score_y = font_size + y_pad;

		DrawText(score_text, score_x, score_y, 48, black);
		DrawText(score_text, score_x + shadow, score_y + shadow, 48, white);

		// life
		int life = *GET_COMPONENT(COMPONENT_LIFE, int, e);		

		char* life_text = TextFormat("%d%% LIFE", life);
		float life_measure = MeasureText(life_text, font_size);

		int life_x = x_pad;
		int life_y = font_size + y_pad;

		DrawText(life_text, life_x, life_y, 48, black);
		DrawText(life_text, life_x + shadow, life_y + shadow, 48, white);
	}
}
void system_draw_player(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_PLAYER, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);

		Vector2 position = *GET_COMPONENT(COMPONENT_POSITION, Vector2, e);		
		if (*player_state == PLAYER_STATE_DEAD)
		{
			double hit_time = *GET_COMPONENT(COMPONENT_HIT_TIME, double, e);		
			float explosion_radius = (GetTime() - (hit_time + PLAYER_HIT_COOLDOWN)) * 1750;
			const Color explosion_color = Fade(RED, 0.25f);

			if (explosion_radius > (screen_width + screen_height))
			{
				DrawRectangle(0, 0, screen_width, screen_height, explosion_color);
			}
			else
			{
				DrawCircle(position.x, position.y, explosion_radius, explosion_color);
			}
			continue;
		}

		float rotation = *GET_COMPONENT(COMPONENT_ROTATION, float, e);		

		const Rectangle source = {0, 0, 32, 32};
		Rectangle dest = {position.x, position.y, 48, 48};
		Vector2 origin = {dest.width / 2, dest.height / 2};

		const Color color = Fade((*player_state == PLAYER_STATE_COOLDOWN) ? RED : WHITE, ((*player_state == PLAYER_STATE_COOLDOWN && (int)(GetTime() * 100) % 2)) ? 0.4f : 1.0f);
		DrawTexturePro(player_ship, source, dest, origin, rotation + 180, color);
	}
}

void system_draw_game_over(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_PLAYER, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		PLAYER_STATE* player_state = GET_COMPONENT(COMPONENT_PLAYER_STATE, PLAYER_STATE, e);

		if (*player_state == PLAYER_STATE_DEAD)
		{
			// game end
			int score = *GET_COMPONENT(COMPONENT_SCORE, int, e);		
			double hit_time = *GET_COMPONENT(COMPONENT_HIT_TIME, double, e);
			double creation_time = *GET_COMPONENT(COMPONENT_CREATION_TIME, double, e);

			const char* game_over = "Game Over!";
			const char* press_r = "Press R to restart";
			const char* score_text = TextFormat("POINTS: %d", score);

			/* DrawRectangle(0, 0, screen_width, screen_height, Fade(RAYWHITE, 0.8f)); */
			DrawText(score_text, screen_width / 2 - MeasureText(score_text, 60) / 2, screen_height * 0.15f, 60, WHITE);
			DrawText(game_over, screen_width / 2 - MeasureText(game_over, 40) / 2, screen_height / 2 - 10, 40, WHITE);
			DrawText(press_r, screen_width / 2 - MeasureText(press_r, 20) / 2, screen_height * 0.75f, 20, WHITE);

			int mins = (int)(hit_time - creation_time) / 60;
			int secs = (int)(hit_time - creation_time) % 60;

			DrawText(TextFormat("Time played: %d minutes, %d seconds", mins, secs), 20, screen_height - 40, 20, WHITE);
		}
	}
}

void system_draw_fps(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_PLAYER, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		component_fps* fps = GET_COMPONENT(COMPONENT_FPS, component_fps, e);
		int current_fps = GetFPS();

		// update fps samples
		if (GetTime() - 0.1 > fps->time || fps->time == 0)
		{
			fps->time = GetTime();
    		fps->samples[fps->index] = current_fps;
    		fps->index = (fps->index + 1) % DRAW_FPS_AVG_SAMPLES;
		}

    	if (fps->index > fps->max_index)
    	{
    		fps->max_index = fps->index;
    	}

		// calculate average fps
    	float sum = 0.0;
    	for (int i = 0; i < fps->max_index + 1; i++) {
        	sum += fps->samples[i];
    	}
    	int average_fps = (int) (sum / fps->max_index);

    	if (current_fps > fps->max)
    	{
    		fps->max = current_fps;
    	}
    	if ((current_fps < fps->min || fps->min <= 0) && fps->max_index >= 10)
    	{
    		fps->min = current_fps;
    	}

		const int fps_string_count = 4;
		char* fps_strings[fps_string_count] = {};
		fps_strings[0] = TextFormat("FPS cur: %d", current_fps);
		fps_strings[1] = TextFormat("FPS avg: %d", average_fps);
		fps_strings[2] = TextFormat("FPS min: %d", fps->min);
		fps_strings[3] = TextFormat("FPS max: %d", fps->max);

		const int font_size = 48;
		const int line_padding = 8;
		const int top_padding = (font_size + line_padding) * 2;
		for (int i = 0; i < fps_string_count; ++i)
		{
			DrawText(fps_strings[i], 16, (i + 1) * (line_padding + font_size) + top_padding, font_size, RED);
		}
	}
}

void system_draw_frame_time(float delta_time)
{
	size_t component_entities[ENTITY_MAX] = {};
	size_t entity_length = set_component_entities(COMPONENT_TAG_PLAYER, component_entities);
	for (size_t i = 0; i < entity_length; ++i)
	{
		size_t e = component_entities[i];

		component_frametime* frametime = GET_COMPONENT(COMPONENT_FRAMETIME, component_frametime, e);
		float current_frametime = delta_time * 1000;

		// update frametime samples
		/* if (GetTime() - 0.1 > frametime->time || frametime->time == 0) */
		/* { */
			frametime->time = GetTime();
    		frametime->samples[frametime->index] = current_frametime;
    		frametime->index = (frametime->index + 1) % DRAW_FRAMETIME_AVG_SAMPLES;
		/* } */

    	if (frametime->index > frametime->max_index)
    	{
    		frametime->max_index = frametime->index;
    	}

		// calculate average frametime
    	float sum = 0.0;
    	for (int i = 0; i < frametime->max_index + 1; i++) {
        	sum += frametime->samples[i];
    	}
    	float average_frametime = (sum / frametime->max_index);

    	if ((current_frametime > frametime->max || frametime->max <= 0) && frametime->max_index >= 10)
    	{
    		frametime->max = current_frametime;
    	}
    	if ((current_frametime < frametime->min || frametime->min <= 0) && frametime->max_index >= 10)
    	{
    		frametime->min = current_frametime;
    	}

		const int frametime_string_count = 4;
		char* frametime_strings[frametime_string_count] = {};
		frametime_strings[0] = TextFormat("ms/f cur: %.4f", current_frametime);
		frametime_strings[1] = TextFormat("ms/f avg: %.4f", average_frametime);
		frametime_strings[2] = TextFormat("ms/f min: %.4f", frametime->min);
		frametime_strings[3] = TextFormat("ms/f max: %.4f", frametime->max);

		const int font_size = 48;
		const int line_padding = 8;
		const int top_padding = (font_size + line_padding) * 2;
		for (int i = 0; i < frametime_string_count; ++i)
		{
			DrawText(frametime_strings[i], 16, (i + 1) * (line_padding + font_size) + top_padding, font_size, RED);
		}
	}
}

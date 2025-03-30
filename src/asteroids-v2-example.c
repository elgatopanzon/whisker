/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : asteroids-v2-example
 * @created     : Monday Feb 17, 2025 21:20:16 CST
 */

#include "whisker_std.h"
#include "raylib.h"
#include "raymath.h"

#include "whisker.h"
#include "whisker_debug.h"
#include "whisker_ecs.h"
#include "whisker_ecs_pool.h"

const int asteroids_screen_width = 800;
const int asteroids_screen_height = 800;
const Vector2 asteroids_screen_center = {(float) asteroids_screen_width / 2, (float) asteroids_screen_height / 2};
#define DRAW_FPS false
#define DRAW_FPS_AVG_SAMPLES 20
#define DRAW_FRAMETIME true
#define DRAW_FRAMETIME_AVG_SAMPLES 300

whisker_ecs *asteroids_ecs;
whisker_ecs_pool *asteroids_asteroids_pool;
whisker_ecs_pool *asteroids_collisions_pool;
whisker_ecs_pool *asteroids_bullets_pool;

typedef enum ASTEROIDS_GAME_STATE
{
	ASTEROIDS_GAME_STATE_PLAYING,
	ASTEROIDS_GAME_STATE_END,
} ASTEROIDS_GAME_STATE;

ASTEROIDS_GAME_STATE asteroids_game_state;

typedef enum ASTEROIDS_ASTEROID_SIZE  
{
	ASTEROIDS_ASTEROID_SIZE_SMALL = 1,
	ASTEROIDS_ASTEROID_SIZE_MEDIUM = 2,
	ASTEROIDS_ASTEROID_SIZE_LARGE = 4,
} ASTEROIDS_ASTEROID_SIZE;
ASTEROIDS_ASTEROID_SIZE asteroid_sizes[] = {ASTEROIDS_ASTEROID_SIZE_SMALL, ASTEROIDS_ASTEROID_SIZE_MEDIUM, ASTEROIDS_ASTEROID_SIZE_LARGE};

typedef enum ASTEROIDS_PLAYER_STATE  
{
	ASTEROIDS_PLAYER_STATE_DEFAULT,
	ASTEROIDS_PLAYER_STATE_HIT,
	ASTEROIDS_PLAYER_STATE_COOLDOWN,
	ASTEROIDS_PLAYER_STATE_DEAD,
} ASTEROIDS_PLAYER_STATE;

#define ASTEROID_ROTATION_VELOCITY_MIN 5 
#define ASTEROID_ROTATION_VELOCITY_MAX 240
#define ASTEROID_VELOCITY_MIN 150 
#define ASTEROID_VELOCITY_MAX 300
#define ASTEROID_RANDOM_ANGLE 30 * DEG2RAD
#define ASTEROID_OFF_SCREEN_PAD 128 
#define ASTEROID_SPAWN_RATE 0.6f 
#define ASTEROID_SPAWN_START 4
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

Texture2D asteroids_player_ship;

float asteroids_game_time_started;
float asteroids_game_time_ended;

void asteroids_game_init();
void asteroids_game_end();
void asteroids_game_update();
void asteroids_game_draw_frame();

void asteroids_init_ecs();
void asteroids_deinit_ecs();

void asteroids_spawn_asteroid();
void asteroids_add_asteroid(Vector2 position, Vector2 velocity, float rotation, float rotation_velocity, ASTEROIDS_ASTEROID_SIZE size);
void asteroids_add_projectile(Vector2 position, float rotation);
void asteroids_create_player_entity();

void asteroids_pool_init_asteroids();
void asteroids_pool_init_collisions();
void asteroids_pool_init_bullets();

typedef struct asteroids_component_collision
{
	whisker_ecs_entity_id entity_a;
	whisker_ecs_entity_id entity_b;
} asteroids_component_collision;

typedef struct asteroids_component_fps
{
	int samples[DRAW_FPS_AVG_SAMPLES];
	double time;
	int index;
	int max_index;
	int max;
	int min;
} asteroids_component_fps;
typedef struct asteroids_component_frametime
{
	float samples[DRAW_FRAMETIME_AVG_SAMPLES];
	double time;
	int index;
	int max_index;
	float max;
	float min;
} asteroids_component_frametime;

int main(int argc, char** argv)
{
	srand(time(0));
	
	/* SetConfigFlags(FLAG_VSYNC_HINT); */
	InitWindow(asteroids_screen_width, asteroids_screen_height, "Asteroids");

	// load textures
	asteroids_player_ship = LoadTexture("assets/ship_2.png");

	asteroids_game_init();

	while (!WindowShouldClose())
	{
		asteroids_game_update();
		asteroids_game_draw_frame();
	}

	asteroids_deinit_ecs();

	CloseWindow();

    return 0;
}

void asteroids_init_ecs()
{
	asteroids_ecs = whisker_ecs_create();

	// create pool instances
	asteroids_asteroids_pool = whisker_ecs_p_create_and_init(asteroids_ecs->components, asteroids_ecs->entities, 32, 16);
	asteroids_collisions_pool = whisker_ecs_p_create_and_init(asteroids_ecs->components, asteroids_ecs->entities, 32, 16);
	asteroids_bullets_pool = whisker_ecs_p_create_and_init(asteroids_ecs->components, asteroids_ecs->entities, 8, 4);
}
void asteroids_deinit_ecs()
{
	whisker_ecs_free(asteroids_ecs);

	whisker_ecs_p_free_all(asteroids_asteroids_pool);
	whisker_ecs_p_free_all(asteroids_collisions_pool);
	whisker_ecs_p_free_all(asteroids_bullets_pool);
}


void asteroids_system_velocity_2d(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "vel_2d", "pos_2d", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		Vector2 *vel_2d = whisker_ecs_itor_get(itor, 0);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 1);

		/* debug_log(DEBUG, velocity_2d, "velocity_2d %zu pos %fx%f vel %fx%f", itor->entity_id, pos_2d->x, pos_2d->y, vel_2d->x, vel_2d->y); */

		pos_2d->x += vel_2d->x * context->delta_time;
		pos_2d->y += vel_2d->y * context->delta_time;
	}
}

void asteroids_system_asteroid_spawn(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "system_asteroid_spawn", "system_asteroid_spawn_time", "pos_2d,vel_2d,ast_size,radius,rot,rot_v,ctime,t_ast,t_screen_cull");

	while (whisker_ecs_s_iterate(itor)) 
	{
		double *system_asteroid_spawn_time = whisker_ecs_itor_get(itor, 1);

		double time = GetTime();

		double time_diff = (*system_asteroid_spawn_time + ASTEROID_SPAWN_RATE) - time;
		int spawn_count = (-time_diff / ASTEROID_SPAWN_RATE + 1);
		if (spawn_count > 50)
		{
			spawn_count = 50;
		}

		for (int i = 0; i < spawn_count; ++i)
		{
			*system_asteroid_spawn_time = time;

			// choose spawn position off screen
			Vector2 position = {
				.x = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD)),
				.y = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD)),
			};

			// set the position randomly to the left/right/top/bottom of the screen
			if (GetRandomValue(0, 1))
			{
				position.x = (position.x > asteroids_screen_center.x) ? asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD;
			}
			else
			{
				position.y = (position.y > asteroids_screen_center.y) ? asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD;
			}

			// set random velocity angle
			Vector2 velocity = Vector2Subtract(asteroids_screen_center, position);
			velocity = Vector2Scale(Vector2Normalize(velocity), GetRandomValue(ASTEROID_VELOCITY_MIN, ASTEROID_VELOCITY_MAX));
			velocity = Vector2Rotate(velocity, (float) GetRandomValue(-ASTEROID_RANDOM_ANGLE, ASTEROID_RANDOM_ANGLE));

			// set random size
			int size_i = (ASTEROIDS_ASTEROID_SIZE)rand() % 3;
			ASTEROIDS_ASTEROID_SIZE size = asteroid_sizes[size_i];

			// create an entity id
			whisker_ecs_entity_id e = whisker_ecs_p_request_entity(asteroids_asteroids_pool);

			// set the entity component data
			float rotation = (float)(rand() % 360);
			float rotation_velocity = GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX);
    		whisker_ecs_set(context->components, itor->component_ids_opt[0], Vector2, e, &position);
    		whisker_ecs_set(context->components, itor->component_ids_opt[1], Vector2, e, &velocity);
    		whisker_ecs_set(context->components, itor->component_ids_opt[2], ASTEROIDS_ASTEROID_SIZE, e, (void*)&size);
    		whisker_ecs_set(context->components, itor->component_ids_opt[3], float, e, (void*)&((float){(ASTEROID_RADIUS * 0.6f) * size}));
    		whisker_ecs_set(context->components, itor->component_ids_opt[4], float, e, (void*)&rotation);
    		whisker_ecs_set(context->components, itor->component_ids_opt[5], float, e, (void*)&rotation_velocity);
    		whisker_ecs_set(context->components, itor->component_ids_opt[6], double, e, (void*)&(double){GetTime()});
    		whisker_ecs_set_tag(context->components, itor->component_ids_opt[7], e);
    		whisker_ecs_set_tag(context->components, itor->component_ids_opt[8], e);

			debug_log(DEBUG, spawn_asteroid, "entity %d size %d at %fx%f (pool size %zu)", e.index, size, position.x, position.y, asteroids_asteroids_pool->entity_pool_length);
		}
	}
}

void asteroids_system_rotation_velocity(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "rot_v", "rot", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		float *rot_v = whisker_ecs_itor_get(itor, 0);
		float *rot = whisker_ecs_itor_get(itor, 1);

		*rot += *rot_v * context->delta_time;
	}
}

void asteroids_system_movement_direction(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_move_dir,rot", "pos_2d", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		float *rot = whisker_ecs_itor_get(itor, 1);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 2);

		float radians = DEG2RAD * (*rot + 90.0f);
		pos_2d->x += PROJECTILE_VELOCITY * cos(radians) * context->delta_time;
		pos_2d->y += PROJECTILE_VELOCITY * sin(radians) * context->delta_time;
	}
}

void asteroids_system_player_controller(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,p_state,pos_2d", "rot,rot_v,vel_2d,fire_time", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 1);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 2);
		float *rot = whisker_ecs_itor_get(itor, 3);
		float *rot_v = whisker_ecs_itor_get(itor, 4);
		Vector2 *vel_2d = whisker_ecs_itor_get(itor, 5);
		double *fire_time = whisker_ecs_itor_get(itor, 6);

		if (IsKeyPressed(KEY_R)) {
			asteroids_game_state = ASTEROIDS_GAME_STATE_END;
			return;
		}
		if (*p_state != ASTEROIDS_PLAYER_STATE_DEFAULT) {
			return;
		}

		// player rotation
		int rotation_input = (int)IsKeyDown(KEY_D) - (int)IsKeyDown(KEY_A);
		/* *rotation += (rotation_input * PLAYER_ROTATION_VELOCITY * delta_time); */
		*rot_v = PLAYER_ROTATION_VELOCITY * rotation_input;

		// player thrust
		Vector2 facing_direction = Vector2Rotate((Vector2){0, 1}, *rot * DEG2RAD);

		int thrust_input = (int)IsKeyDown(KEY_W) - (int)IsKeyDown(KEY_S);
		if (thrust_input > 0) {
			*vel_2d = Vector2Add(*vel_2d, Vector2Scale(facing_direction, PLAYER_ACCELERATION * context->delta_time));

			float mag = Vector2Length(*vel_2d);
			if (mag > PLAYER_VELOCITY) {
				*vel_2d = Vector2Scale(*vel_2d, PLAYER_VELOCITY / mag);
			}
		}
		else
		{
			*vel_2d = Vector2Add(*vel_2d, Vector2Scale(*vel_2d, -PLAYER_DECELLERATION * context->delta_time));
		}

		// projectiles
		double now = GetTime();
		bool can_fire = (now > *fire_time + PROJECTILE_FIRE_RATE);

		if (IsKeyDown(KEY_SPACE) && can_fire) {
			asteroids_add_projectile(Vector2Add(*pos_2d, Vector2Scale(facing_direction, 50)), *rot);
			*fire_time = now;
		}
	}
}

void asteroids_system_screen_wrap(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_screen_wrap", "pos_2d", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 1);

		if (
				pos_2d->x < 0 ||
				pos_2d->y < 0 ||
				pos_2d->x > asteroids_screen_width ||
				pos_2d->y > asteroids_screen_height
				)
		{
			debug_log(DEBUG, system:screen_wrap, "e = %d", itor->entity_id.index);

			if (pos_2d->x < 0)
			{
				pos_2d->x += asteroids_screen_width;
			}
			if (pos_2d->x > asteroids_screen_width)
			{
				pos_2d->x -= asteroids_screen_width;
			}
			if (pos_2d->y < 0)
			{
				pos_2d->y += asteroids_screen_height;
			}
			if (pos_2d->y > asteroids_screen_height)
			{
				pos_2d->y -= asteroids_screen_height;
			}
		}
	}
}

void asteroids_system_player_death_on_life_depleted(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,life", "p_state", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		int *life = whisker_ecs_itor_get(itor, 1);
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 2);

		if (*p_state == ASTEROIDS_PLAYER_STATE_DEFAULT && *life <= 0) {
			debug_log(DEBUG, system:player_death_on_life_depleted, "");
			/* whisker_ecs_remove_component(asteroids_ecs, "vel_2d", sizeof(Vector2), system.entity_id); */
			*p_state = ASTEROIDS_PLAYER_STATE_DEAD;
		}
	}
}

void asteroids_system_player_hit_cooldown(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,hit_time,p_state", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		double *hit_time = whisker_ecs_itor_get(itor, 1);
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 2);

		if (*p_state == ASTEROIDS_PLAYER_STATE_COOLDOWN && *hit_time + PLAYER_HIT_COOLDOWN < GetTime()) {
			debug_log(DEBUG, system:player_hit_cooldown, "returning to default state");
			*p_state = ASTEROIDS_PLAYER_STATE_DEFAULT;
		}
	}
}
struct asteroids_system_collision_entity 
{
	whisker_ecs_entity_id id;
	Vector2 *pos_2d;
	float *radius;
};
void asteroids_system_collision(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor_outer = whisker_ecs_s_get_iterator(context, 0, "pos_2d,radius", "", "collision");

	size_t entity_count = itor_outer->count;
	size_t count = 0;
	struct asteroids_system_collision_entity entities[entity_count];

	while (whisker_ecs_s_iterate(itor_outer)) 
	{
		Vector2 *pos_2d = whisker_ecs_itor_get(itor_outer, 0);
		float *radius = whisker_ecs_itor_get(itor_outer, 1);
		entities[count] = (struct asteroids_system_collision_entity) {
			.id = itor_outer->entity_id,
			.pos_2d = pos_2d,
			.radius = radius,
		};
		count++;
	}

	for (int i = 0; i < count; ++i)
	{
		Vector2 *pos_2d = entities[i].pos_2d;
		float *radius = entities[i].radius;

		for (int j = 0; j < count; ++j)
		{
			if (entities[i].id.index == entities[j].id.index) {
				continue;
			}


			Vector2 *colliding_position = entities[j].pos_2d;
			float *colliding_radius_size = entities[j].radius;

			/* debug_printf("checking entity %zu (%fx%f) for collision with %zu (%fx%f)\n", itor_outer->entity_id, itor_inner->entity_id, pos_2d->x, pos_2d->y, colliding_position->x, colliding_position->y); */

			float distance = Vector2Distance(*pos_2d, *colliding_position);
			if (distance <= (*radius + *colliding_radius_size))
			{
    			float overlap = (*radius + *colliding_radius_size) - distance;
    			Vector2 direction = Vector2Normalize(Vector2Subtract(*pos_2d, *colliding_position));
    			Vector2 correction = Vector2Scale(direction, overlap / 2);

    			*pos_2d = Vector2Add(*pos_2d, correction);
    			*colliding_position = Vector2Subtract(*colliding_position, correction);

    			/* whisker_ecs_entity_id collision_e = whisker_ecs_create_entity_deferred(context->entities); */
    			whisker_ecs_entity_id collision_e = whisker_ecs_p_request_entity(asteroids_collisions_pool);

    			asteroids_component_collision col = {};
    			col.entity_a = entities[i].id;
    			col.entity_b = entities[j].id;
    			whisker_ecs_set(context->components, itor_outer->component_ids_opt[0], asteroids_component_collision, collision_e, &col);
			}
		}
	}
}

void asteroids_system_collision_cull(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "collision", "", "t_cull");

	while (whisker_ecs_s_iterate(itor)) 
	{
		whisker_ecs_set_tag(context->components, itor->component_ids_opt[0], itor->entity_id);
	}
}

void asteroids_system_destroy_offscreen(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_screen_cull,pos_2d", "", "t_cull");

	while (whisker_ecs_s_iterate(itor)) 
	{
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 1);

		if (pos_2d->x < -(ASTEROID_OFF_SCREEN_PAD) ||
			pos_2d->y < -(ASTEROID_OFF_SCREEN_PAD) ||
			pos_2d->x > (asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD) ||
			pos_2d->y > (asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD)
			)
		{
			debug_log(DEBUG, system:destroy_offscreen, "e = %zu %fx%f", itor->entity_id.id, pos_2d->x, pos_2d->y);
			/* whisker_ecs_c_set_component(system->components, itor->component_ids_opt[0], sizeof(bool), itor->entity_id, &(bool){false}); */
			whisker_ecs_set_tag(context->components, itor->component_ids_opt[0], itor->entity_id);
		}
	}
}

void asteroids_system_projectile_collide_destroy(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "collision", "", "t_cull,t_ast_hit,t_bullet,t_ast");

	while (whisker_ecs_s_iterate(itor)) 
	{
		asteroids_component_collision *collision = whisker_ecs_itor_get(itor, 0);

		if (!whisker_ecs_has(context->components, itor->component_ids_opt[2], collision->entity_a) || !whisker_ecs_has(context->components, itor->component_ids_opt[3], collision->entity_b))
		{
			continue;
		}

		debug_log(DEBUG, system:projectile_collide_destroy, "%zu hit asteroid %zu", collision->entity_a, collision->entity_b);

		// destroy existing asteroid and projectile
		whisker_ecs_set_tag(context->components, itor->component_ids_opt[0], collision->entity_a);    
		whisker_ecs_set_tag(context->components, itor->component_ids_opt[0], collision->entity_b);    
		whisker_ecs_set_tag(context->components, itor->component_ids_opt[1], collision->entity_b);    
	}
}

void asteroids_system_asteroid_respawn_on_hit(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_ast_hit,ast_size,pos_2d", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_ASTEROID_SIZE *ast_size = whisker_ecs_itor_get(itor, 1);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 2);

		int spawn_count = 0;

		ASTEROIDS_ASTEROID_SIZE new_size = ASTEROIDS_ASTEROID_SIZE_LARGE;

		switch (*ast_size) {
			case ASTEROIDS_ASTEROID_SIZE_LARGE:
				new_size = ASTEROIDS_ASTEROID_SIZE_MEDIUM;
				spawn_count = 3;
				break;
			case ASTEROIDS_ASTEROID_SIZE_MEDIUM:
				new_size = ASTEROIDS_ASTEROID_SIZE_SMALL;
				spawn_count = 2;
				break;
		}

		debug_log(DEBUG, system:asteroid_respawn_on_destroy, "spawn count %d from size %d", spawn_count, *ast_size);

		// note: the add asteroids triggers a component realloc leaving *pos_2d
		// invalid after the first loop
		Vector2 spawn_pos = *pos_2d;
		ASTEROIDS_ASTEROID_SIZE asteroid_size = *ast_size;
		for (int ii = 0; ii < spawn_count; ++ii)
		{
			debug_log(DEBUG, system:asteroid_respawn_on_destroy, "spawning (from %d) size %d", itor->entity_id.index, asteroid_size);

			asteroids_add_asteroid(spawn_pos, (Vector2) {GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2)), GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2))}, GetRandomValue(-360, 360) * DEG2RAD, GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), new_size);
		}
	}
}

void asteroids_system_asteroid_hit_asteroid(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "collision", "", "t_ast,vel_2d,rot_v,pos_2d");

	while (whisker_ecs_s_iterate(itor)) 
	{
		asteroids_component_collision *collision = whisker_ecs_itor_get(itor, 0);

		if (!whisker_ecs_has(context->components, itor->component_ids_opt[0], collision->entity_a) || !whisker_ecs_has(context->components, itor->component_ids_opt[0], collision->entity_b))
		{
			continue;
		}

		/* debug_printf("collision %d: %d hit asteroid %d\n", system.entity->id.index, collision->entity_a.index, collision->entity_b.index); */

		// asteroid a
		Vector2 *asteroida_velocity = whisker_ecs_get(context->components, itor->component_ids_opt[1], collision->entity_a);
		float *asteroida_rotation_velocity = whisker_ecs_get(context->components, itor->component_ids_opt[2], collision->entity_a);		


		Vector2 *asteroida_position = whisker_ecs_get(context->components, itor->component_ids_opt[3], collision->entity_a);		
		Vector2 *asteroida_hit_by_position = whisker_ecs_get(context->components, itor->component_ids_opt[3], collision->entity_b);		

		Vector2 asteroida_nudge_direction = Vector2Normalize(Vector2Subtract(*asteroida_position, *asteroida_hit_by_position));


		// asteroid b
		Vector2 *asteroidb_velocity = whisker_ecs_get(context->components, itor->component_ids_opt[1], collision->entity_b);
		float *asteroidb_rotation_velocity = whisker_ecs_get(context->components, itor->component_ids_opt[2], collision->entity_b);		

		/* debug_printf("system:asteroid_hit_asteroid 1:%d (%f) hit asteroid %d (%f)\n", collision->entity_a.index, *asteroida_rotation_velocity, collision->entity_b.index, *asteroidb_rotation_velocity); */

		*asteroida_rotation_velocity = (GetRandomValue(-180, 180));
		*asteroida_velocity = Vector2Scale(asteroida_nudge_direction, PLAYER_HIT_NUDGE_FORCE);
		*asteroidb_rotation_velocity = (GetRandomValue(-180, 180));

		/* debug_printf("system:asteroid_hit_asteroid 1:%d (%f) hit asteroid %d (%f)\n", collision->entity_a.index, *asteroida_rotation_velocity, collision->entity_b.index, *asteroidb_rotation_velocity); */
	}
}

void asteroids_system_asteroid_score(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_ast_hit,ast_size", "", "score");

	int add_score = 0;
	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_ASTEROID_SIZE *ast_size = whisker_ecs_itor_get(itor, 1);

		add_score += (int)*ast_size * ASTEROID_SCORE;
	}

	whisker_ecs_system_iterator *itor_score = whisker_ecs_s_get_iterator(context, 1, "score", "", "");

	while (whisker_ecs_s_iterate(itor_score)) 
	{
		int *score = whisker_ecs_itor_get(itor_score, 0);
		*score += add_score;
	}
}

void asteroids_system_player_hit_asteroid(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "collision", "", "p_state,hit_time,hit_collision,life,ast_size,vel_2d,rot_v,t_ast,t_player");

	while (whisker_ecs_s_iterate(itor)) 
	{
		asteroids_component_collision *collision = whisker_ecs_itor_get(itor, 0);

		if (!whisker_ecs_has(context->components, itor->component_ids_opt[7], collision->entity_a) || !whisker_ecs_has(context->components, itor->component_ids_opt[8], collision->entity_b))
		{
			continue;
		}

		ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_get(context->components, itor->component_ids_opt[0], collision->entity_b);

		// deal damage to the player, only in the default state
		// TODO: maybe figure out a better way to handle this state
		if (*player_state != ASTEROIDS_PLAYER_STATE_DEFAULT) {
			return;
		}

		*player_state = ASTEROIDS_PLAYER_STATE_HIT;

		double *hit_time = whisker_ecs_get(context->components, itor->component_ids_opt[1], collision->entity_b);
		*hit_time = GetTime();

		asteroids_component_collision *col = whisker_ecs_get(context->components, itor->component_ids_opt[2], collision->entity_b);
		col->entity_a = collision->entity_a;
		col->entity_b = collision->entity_b;

		// TODO: refactor this to use system access macros
		int* player_life = whisker_ecs_get(context->components, itor->component_ids_opt[3], collision->entity_b);
		ASTEROIDS_ASTEROID_SIZE* asteroid_size = whisker_ecs_get(context->components, itor->component_ids_opt[4], collision->entity_a);

		int damage = *asteroid_size * ASTEROID_DAMAGE;

		Vector2* asteroid_velocity = whisker_ecs_get(context->components, itor->component_ids_opt[5], collision->entity_a);
		*asteroid_velocity = Vector2Scale(*asteroid_velocity, ASTEROID_HIT_VELOCITY_REDUCTION);
		float* rotation_velocity = whisker_ecs_get(context->components, itor->component_ids_opt[6], collision->entity_a);		
		*rotation_velocity += (GetRandomValue(-270, 270));

		debug_log(DEBUG, system:player_damage, "%zu hit player %zu (%d damage)", collision->entity_a, collision->entity_b, damage);

		*player_life -= damage;
		if (*player_life <= 0) {
			*player_life = 0;
		}
	}
}

void asteroids_system_player_hit_nudge(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,p_state,hit_collision,pos_2d", "rot_v,vel_2d", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 1);
		asteroids_component_collision *hit_collision = whisker_ecs_itor_get(itor, 2);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 3);
		float *rot_v = whisker_ecs_itor_get(itor, 4);
		Vector2 *vel_2d = whisker_ecs_itor_get(itor, 5);

		if (*p_state == ASTEROIDS_PLAYER_STATE_HIT) {
			debug_log(DEBUG, system:player_hit_nudge, "nudging player from the hit");

			Vector2 *hit_by_position = whisker_ecs_get(context->components, itor->component_ids_rw[3], hit_collision->entity_a);		

			Vector2 nudge_direction = Vector2Normalize(Vector2Subtract(*pos_2d, *hit_by_position));

			*rot_v += (GetRandomValue(-270, 270));

			debug_log(DEBUG, system:player_hit_nudge, "velocity before %fx%f", vel_2d->x, vel_2d->y);
			*vel_2d = Vector2Scale(nudge_direction, PLAYER_HIT_NUDGE_FORCE);

			debug_log(DEBUG, system:player_hit_nudge, "velocity after %fx%f", vel_2d->x, vel_2d->y);
		}
	}
}

void asteroids_system_player_hit_to_recover(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,p_state", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 1);

		if (*p_state == ASTEROIDS_PLAYER_STATE_HIT) {
			debug_log(DEBUG, system:player_hit_to_recover, "");
			*p_state = ASTEROIDS_PLAYER_STATE_COOLDOWN;
		}
	}
}

void asteroids_system_entity_deferred_destroy(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_cull", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		/* debug_printf("destroying entity %zu %d\n", itor->entity_id); */

		whisker_ecs_destroy_entity_deferred(asteroids_ecs->entities, itor->entity_id);
	}
}

void asteroids_system_draw_asteroid(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_ast,rot,pos_2d,ast_size", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		float *rot = whisker_ecs_itor_get(itor, 1);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 2);
		ASTEROIDS_ASTEROID_SIZE *ast_size = whisker_ecs_itor_get(itor, 3);

		// skip offscreen asteroids from drawing
		if (pos_2d->x < -(ASTEROID_OFF_SCREEN_PAD) ||
			pos_2d->y < -(ASTEROID_OFF_SCREEN_PAD) ||
			pos_2d->x > (asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD) ||
			pos_2d->y > (asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD)
			)
		{
			continue;
		}

		/* debug_printf("drawing asteroid %zu size %d pos %fx%f\n", itor->entity_id, *ast_size, pos_2d->x, pos_2d->y); */

		DrawPolyLines(*pos_2d, 3, 16 * (int)*ast_size, *rot, WHITE);
	}
}

void asteroids_system_draw_projectile(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_bullet,rot,pos_2d", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		float *rot = whisker_ecs_itor_get(itor, 1);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 2);

		Rectangle rect = {};
		rect.x = pos_2d->x;
		rect.y = pos_2d->y;
		rect.width = PROJECTILE_WIDTH;
		rect.height = PROJECTILE_LENGTH;
		Vector2 origin = {};
		origin.x = rect.width / 2;
		origin.y = rect.height / 2;
		DrawRectanglePro(rect, origin, *rot, RED);
	}
}

void asteroids_system_draw_player(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,rot,pos_2d,hit_time,p_state", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		float *rot = whisker_ecs_itor_get(itor, 1);
		Vector2 *pos_2d = whisker_ecs_itor_get(itor, 2);
		double *hit_time = whisker_ecs_itor_get(itor, 3);
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 4);

		if (*p_state == ASTEROIDS_PLAYER_STATE_DEAD)
		{
			float explosion_radius = (GetTime() - (*hit_time + PLAYER_HIT_COOLDOWN)) * 1750;
			const Color explosion_color = Fade(RED, 0.25f);

			if (explosion_radius > (asteroids_screen_width + asteroids_screen_height))
			{
				DrawRectangle(0, 0, asteroids_screen_width, asteroids_screen_height, explosion_color);
			}
			else
			{
				DrawCircle(pos_2d->x, pos_2d->y, explosion_radius, explosion_color);
			}
			return;
		}

		Rectangle source = {};
		source.x = 0;
		source.y = 0;
		source.width = 32;
		source.height = 32;

		Rectangle dest = {};
		dest.x = pos_2d->x;
		dest.y = pos_2d->y;
		dest.width = 48;
		dest.height = 48;

		Vector2 origin = {};
		origin.x = dest.width / 2;
		origin.y = dest.height / 2;

		const Color color = Fade((*p_state == ASTEROIDS_PLAYER_STATE_COOLDOWN) ? RED : WHITE, ((*p_state == ASTEROIDS_PLAYER_STATE_COOLDOWN && (int)(GetTime() * 100) % 2)) ? 0.4f : 1.0f);
		DrawTexturePro(asteroids_player_ship, source, dest, origin, *rot + 180, color);
	}
}

void asteroids_system_draw_hud(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,p_state,score,life", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 1);
		int *score = whisker_ecs_itor_get(itor, 2);
		int *life = whisker_ecs_itor_get(itor, 3);

		const int font_size = 64;
		const Color black = Fade(BLACK, 0.8f);
		const Color white = Fade(WHITE, 0.8f);
		const int x_pad = 16;
		const int y_pad = 8;
		const int shadow = 8;

		if (*p_state == ASTEROIDS_PLAYER_STATE_DEAD)
		{
			return;
		}

		// score
		char* score_text = TextFormat("%d POINTS", *score);
		float score_measure = MeasureText(score_text, font_size);

		int score_x = asteroids_screen_width - score_measure;
		int score_y = font_size + y_pad;

		DrawText(score_text, score_x, score_y, 48, black);
		DrawText(score_text, score_x + shadow, score_y + shadow, 48, white);

		// life
		char* life_text = TextFormat("%d%% LIFE", *life);
		float life_measure = MeasureText(life_text, font_size);

		int life_x = x_pad;
		int life_y = font_size + y_pad;

		DrawText(life_text, life_x, life_y, 48, black);
		DrawText(life_text, life_x + shadow, life_y + shadow, 48, white);
	}
}

void asteroids_system_draw_game_over(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_player,p_state,score,life,hit_time,ctime", "", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		ASTEROIDS_PLAYER_STATE *p_state = whisker_ecs_itor_get(itor, 1);
		int *score = whisker_ecs_itor_get(itor, 2);
		int *life = whisker_ecs_itor_get(itor, 3);
		double *hit_time = whisker_ecs_itor_get(itor, 4);
		double *ctime = whisker_ecs_itor_get(itor, 5);

		if (*p_state == ASTEROIDS_PLAYER_STATE_DEAD)
		{
			// game end
			const char* game_over = "Game Over!";
			const char* press_r = "Press R to restart";
			const char* score_text = TextFormat("POINTS: %d", *score);
			int mins = (int)(*hit_time - *ctime) / 60;
			int secs = (int)(*hit_time - *ctime) % 60;

			const char* time_text = TextFormat("Time played: %d minutes, %d seconds", mins, secs);

			/* DrawRectangle(0, 0, screen_width, screen_height, Fade(RAYWHITE, 0.8f)); */
			DrawText(score_text, asteroids_screen_width / 2 - MeasureText(score_text, 60) / 2, asteroids_screen_height * 0.15f, 60, WHITE);
			DrawText(game_over, asteroids_screen_width / 2 - MeasureText(game_over, 40) / 2, asteroids_screen_height / 2 - 10, 40, WHITE);
			DrawText(press_r, asteroids_screen_width / 2 - MeasureText(press_r, 20) / 2, asteroids_screen_height * 0.75f, 20, WHITE);
			DrawText(time_text, asteroids_screen_width / 2 - MeasureText(time_text, 20) / 2, asteroids_screen_height / 2 + 40, 20, WHITE);
		}
	}
}

void asteroids_system_draw_frame_time(whisker_ecs_system_context *context)
{
	size_t asteroid_count = whisker_ecs_s_get_iterator(context, 0, "t_ast", "", "")->count;

	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 1, "system_draw_frame_time", "frametime", "");

	while (whisker_ecs_s_iterate(itor)) 
	{
		float current_frametime = context->delta_time * 1000;
		asteroids_component_frametime *frametime = whisker_ecs_itor_get(itor, 1);

		// update frametime samples
		frametime->samples[frametime->index] = current_frametime;
		frametime->index = (frametime->index + 1) % DRAW_FRAMETIME_AVG_SAMPLES;

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

		if (GetTime() - 2.5 > frametime->time || frametime->time == 0)
		{
			frametime->time = GetTime();
		    frametime->max = current_frametime;
		    frametime->min = current_frametime;
		}

		if ((current_frametime > frametime->max || frametime->max <= 0) && frametime->max_index >= 10)
		{
		    frametime->max = current_frametime;
		}
		if ((current_frametime < frametime->min || frametime->min <= 0) && frametime->max_index >= 10)
		{
		    frametime->min = current_frametime;
		}

		const int frametime_string_count = 4;
		const char* frametime_string = TextFormat("%2.2f %2.2f %2.2f ms/f", average_frametime, frametime->min, frametime->max);
		const char* process_frametime = TextFormat("phase %2.2f ms/f", context->process_phase_time_step->delta_time_variable * 1000);

		int font_size = 32;
		DrawText(frametime_string, 20, asteroids_screen_height - (font_size + 8) + 4, font_size, BLACK);
		DrawText(frametime_string, 16, asteroids_screen_height - (font_size + 8), font_size, RED);

		DrawText(process_frametime, 400, asteroids_screen_height - (font_size + 8) + 4, font_size, BLACK);
		DrawText(process_frametime, 400, asteroids_screen_height - (font_size + 8), font_size, RED);

		size_t entity_count = whisker_ecs_e_count(context->entities);
		size_t entity_count_alive = whisker_ecs_e_alive_count(context->entities);

		const char* s1 = TextFormat("AC %d", asteroid_count);
		const char* s2 = TextFormat("EA %d", entity_count_alive);
		const char* s3 = TextFormat("ET %d", entity_count);
		DrawText(s1, 20, asteroids_screen_height - ((font_size + 8) * 4) + 4, font_size, BLACK);
		DrawText(s1, 16, asteroids_screen_height - ((font_size + 8) * 4), font_size, RED);
		DrawText(s2, 20, asteroids_screen_height - ((font_size + 8) * 3) + 4, font_size, BLACK);
		DrawText(s2, 16, asteroids_screen_height - ((font_size + 8) * 3), font_size, RED);
		DrawText(s3, 20, asteroids_screen_height - ((font_size + 8) * 2) + 4, font_size, BLACK);
		DrawText(s3, 16, asteroids_screen_height - ((font_size + 8) * 2), font_size, RED);
	}
}

#define NEARBLACK CLITERAL(Color){15, 15, 15, 10} 
void asteroids_system_raylib_start_drawing(whisker_ecs_system_context *context)
{
	BeginDrawing();

	ClearBackground(NEARBLACK);
}

void asteroids_system_raylib_end_drawing(whisker_ecs_system_context *context)
{
	EndDrawing();
}

void asteroids_system_test_pos_2d_events(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *added_itor = whisker_ecs_s_get_iterator(context, 0, "pos_2d_ev_added", "", "");

	while (whisker_ecs_s_iterate(added_itor)) 
	{
		debug_log(DEBUG, pos_2d_added, "event entity %zu event target %zu", added_itor->entity_id, added_itor->entity_id);
	}

	whisker_ecs_system_iterator *changed_itor = whisker_ecs_s_get_iterator(context, 1, "pos_2d_ev_changed", "", "");

	while (whisker_ecs_s_iterate(changed_itor)) 
	{
		debug_log(DEBUG, pos_2d_changed, "event entity %zu event target %zu", changed_itor->entity_id, changed_itor->entity_id);
	}

	whisker_ecs_system_iterator *removed_itor = whisker_ecs_s_get_iterator(context, 2, "pos_2d_ev_removed_from", "", "");

	while (whisker_ecs_s_iterate(removed_itor)) 
	{
		whisker_ecs_entity_id *entity = whisker_ecs_itor_get(removed_itor, 0);
		debug_log(DEBUG, pos_2d_removed_from, "event entity %zu event target %zu", removed_itor->entity_id, *entity);
	}

}

void asteroids_game_init()
{
	asteroids_game_state = ASTEROIDS_GAME_STATE_PLAYING;
	asteroids_game_time_started = GetTime();

	if (asteroids_ecs != NULL)
	{
		asteroids_deinit_ecs();
	}
	asteroids_init_ecs();

	// create player entity
	asteroids_create_player_entity();

	// init pool prototype entities
	asteroids_pool_init_asteroids();
	asteroids_pool_init_collisions();
	asteroids_pool_init_bullets();

	// spawn asteroids
	for (int i = 0; i < ASTEROID_SPAWN_START; ++i)
	{
		asteroids_spawn_asteroid();
	}

	/*****************
	*  phases: pre  *
	*****************/
	// pre_load phase
	{
		whisker_ecs_system *spawn_sys = 
    		whisker_ecs_register_system(
        		asteroids_ecs, 
        		asteroids_system_asteroid_spawn, 
        		"system_asteroid_spawn", 
        		WHISKER_ECS_PROCESS_PHASE_PRE_LOAD, 
        		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
    		);
			whisker_ecs_set_named(
    			asteroids_ecs->entities,
    			asteroids_ecs->components,
    			system_asteroid_spawn_time,
    			double,
    			spawn_sys->entity_id,
    			&(double){GetTime()}
			);
	}


	/********************
	*  phases: update  *
	********************/
	// pre_update phase
	{
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_player_controller,
    		"system_player_controller",
    		WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_velocity_2d,
    		"system_velocity_2d",
    		WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_AUTO
		);
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_rotation_velocity,
    		"system_rotation_velocity",
    		WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_AUTO
		);
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_movement_direction,
    		"system_movement_direction",
    		WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_screen_wrap,
    		"system_screen_wrap",
    		WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_collision,
    		"system_collision",
    		WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
	}

	// on_update phase
	{
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_projectile_collide_destroy, 
    		"system_projectile_collide_destroy", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_asteroid_score, 
    		"system_asteroid_score", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_asteroid_hit_asteroid, 
    		"system_asteroid_hit_asteroid", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_player_hit_asteroid, 
    		"system_player_hit_asteroid", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_player_hit_nudge, 
    		"system_player_hit_nudge", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_player_hit_cooldown, 
    		"system_player_hit_cooldown", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_player_death_on_life_depleted, 
    		"system_player_death_on_life_depleted", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_player_hit_to_recover, 
    		"system_player_hit_to_recover", 
    		WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
	}

	// post_update phase
	{
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_asteroid_respawn_on_hit, 
    		"system_asteroid_respawn_on_hit", 
    		WHISKER_ECS_PROCESS_PHASE_POST_UPDATE, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);

		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_destroy_offscreen,
    		"system_destroy_offscreen",
    		WHISKER_ECS_PROCESS_PHASE_POST_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_AUTO
		);

		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_collision_cull,
    		"system_collision_cull",
    		WHISKER_ECS_PROCESS_PHASE_POST_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_AUTO
		);

		// test system: pos_2d events
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_test_pos_2d_events,
    		"system_test_pos_2d_events",
    		WHISKER_ECS_PROCESS_PHASE_POST_UPDATE,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
	}


	/********************
	*  phases: render  *
	********************/
	// pre_render phase
	{
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_raylib_start_drawing,
    		"system_raylib_start_drawing",
    		WHISKER_ECS_PROCESS_PHASE_PRE_RENDER,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
	}

	// on_render
	{
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_draw_asteroid, 
    		"system_draw_asteroid", 
    		WHISKER_ECS_PROCESS_PHASE_ON_RENDER, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_draw_player, 
    		"system_draw_player", 
    		WHISKER_ECS_PROCESS_PHASE_ON_RENDER, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_draw_projectile, 
    		"system_draw_projectile", 
    		WHISKER_ECS_PROCESS_PHASE_ON_RENDER, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_draw_hud, 
    		"system_draw_hud", 
    		WHISKER_ECS_PROCESS_PHASE_POST_RENDER, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
		whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_draw_game_over, 
    		"system_draw_game_over", 
    		WHISKER_ECS_PROCESS_PHASE_POST_RENDER, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);

#ifdef DRAW_FRAMETIME
		whisker_ecs_system *frametime_sys = whisker_ecs_register_system(
    		asteroids_ecs, 
    		asteroids_system_draw_frame_time, 
    		"system_draw_frame_time", 
    		WHISKER_ECS_PROCESS_PHASE_POST_RENDER, 
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);

		whisker_ecs_set_named(
    		asteroids_ecs->entities, 
    		asteroids_ecs->components, 
    		frametime, 
    		asteroids_component_frametime, 
    		frametime_sys->entity_id, 
    		&(asteroids_component_frametime){}
		);
#endif
	}

	// final_render phase
	{
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_raylib_end_drawing,
    		"system_raylib_end_drawing",
    		WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER,
    		WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD
		);
	}


	/******************
	*  phases: final  *
	******************/
	// final phase
	{
		whisker_ecs_register_system(
    		asteroids_ecs,
    		asteroids_system_entity_deferred_destroy,
    		"system_entity_deferred_destroy",
    		WHISKER_ECS_PROCESS_PHASE_FINAL,
    		WHISKER_ECS_PROCESS_THREADED_AUTO
		);
	}
}

void asteroids_game_end()
{
	asteroids_game_state = ASTEROIDS_GAME_STATE_END;
	asteroids_game_time_ended = GetTime();
}

void asteroids_game_update()
{
	if (asteroids_game_state == ASTEROIDS_GAME_STATE_END) 
	{
		asteroids_game_init();
	}

	return;
}

void asteroids_game_draw_frame()
{
	whisker_ecs_update(asteroids_ecs, GetFrameTime());
}

void asteroids_create_player_entity()
{
	whisker_ecs_entity_id e = whisker_ecs_create_named_entity_deferred(asteroids_ecs->entities, "player");

	Vector2 position = asteroids_screen_center;

	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, pos_2d, Vector2, e, &(position));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, vel_2d, Vector2, e, &((Vector2) { }));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, radius, float, e, (void*)&((float){PLAYER_RADIUS * 0.66f}));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, rot, float, e, (void*)&(float){180});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, rot_v, float, e, (void*)&(float){0});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, ctime, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, score, int, e, (void*)&(int){0});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, life, int, e, (void*)&(int){PLAYER_START_LIFE});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, p_state, ASTEROIDS_PLAYER_STATE, e, (void*)&(ASTEROIDS_PLAYER_STATE){ASTEROIDS_PLAYER_STATE_DEFAULT});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, fire_time, double, e, (void*)&(double){0});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, hit_time, double, e, (void*)&(double){0});
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, hit_collision, asteroids_component_collision, e, (void*)&(asteroids_component_collision){});

	/* whisker_ecs_set(asteroids_ecs, fps, asteroids_component_fps, e, (void*)&(asteroids_component_fps){0}); */
	/* whisker_ecs_set(asteroids_ecs, frametime, asteroids_component_frametime, e, (void*)&(asteroids_component_frametime){0}); */
	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_player, e);    
	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_screen_wrap, e);    
}

void asteroids_spawn_asteroid()
{
	// choose spawn position off screen
	Vector2 position = {
		.x = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD)),
		.y = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD)),
	};

	// set the position randomly to the left/right/top/bottom of the screen
	if (GetRandomValue(0, 1))
	{
		position.x = (position.x > asteroids_screen_center.x) ? asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD;
	}
	else
	{
		position.y = (position.y > asteroids_screen_center.y) ? asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD;
	}

	// set random velocity angle
	Vector2 velocity = Vector2Subtract(asteroids_screen_center, position);
	velocity = Vector2Scale(Vector2Normalize(velocity), GetRandomValue(ASTEROID_VELOCITY_MIN, ASTEROID_VELOCITY_MAX));
	velocity = Vector2Rotate(velocity, (float) GetRandomValue(-ASTEROID_RANDOM_ANGLE, ASTEROID_RANDOM_ANGLE));

	// set random size
	int size_i = (ASTEROIDS_ASTEROID_SIZE)rand() % 3;
	ASTEROIDS_ASTEROID_SIZE size = asteroid_sizes[size_i];

	asteroids_add_asteroid(position, velocity, (float)(rand() % 360), (float)GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), size);
}

void asteroids_add_asteroid(Vector2 position, Vector2 velocity, float rotation, float rotation_velocity, ASTEROIDS_ASTEROID_SIZE size)
{
	// create an entity id
	whisker_ecs_entity_id e = whisker_ecs_p_request_entity(asteroids_asteroids_pool);

	// set the entity component data
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, pos_2d, Vector2, e, &(position));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, vel_2d, Vector2, e, &(velocity));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, ast_size, ASTEROIDS_ASTEROID_SIZE, e, (void*)&size);
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, radius, float, e, (void*)&((float){(ASTEROID_RADIUS * 0.6f) * size}));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, rot, float, e, (void*)&rotation);
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, rot_v, float, e, (void*)&(rotation_velocity));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, ctime, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_ast, e);    
	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_screen_cull, e);    

	debug_log(DEBUG, add_asteroid, "entity %d size %d at %fx%f", e.index, size, position.x, position.y);
}

void asteroids_add_projectile(Vector2 position, float rotation)
{
	// create an entity id
	whisker_ecs_entity_id e = whisker_ecs_p_request_entity(asteroids_bullets_pool);

	// set the entity component data
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, pos_2d, Vector2, e, &(position));
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, rot, float, e, (void*)&rotation);
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, radius, float, e, (void*)&((float){((PROJECTILE_WIDTH + PROJECTILE_LENGTH) * 2) / 4}));

	// TODO: this is only because the recycled entity's data still exists and is
	// updated by the movement system because it looks for a POSITION component
	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, vel_2d, Vector2, e, &((Vector2){}));

	whisker_ecs_set_named(asteroids_ecs->entities, asteroids_ecs->components, ctime, double, e, (void*)&(double){GetTime()});

	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_bullet, e);    
	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_move_dir, e);    
	whisker_ecs_set_named_tag(asteroids_ecs->entities, asteroids_ecs->components, t_screen_cull, e);    

	debug_log(DEBUG, add_projectile, "rot %f at %fx%f", rotation, position.x, position.y);
}


void asteroids_pool_init_asteroids()
{
	// create an entity id
	Vector2 pos_2d = {0,0};
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, pos_2d, Vector2, &pos_2d);
	Vector2 vel_2d = {0,0};
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, vel_2d, Vector2, &vel_2d);
	ASTEROIDS_ASTEROID_SIZE ast_size = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, ast_size, ASTEROIDS_ASTEROID_SIZE, &ast_size);
	float radius = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, radius, float, &radius);
	float rot = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, rot, float, &rot);
	float rot_v = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, rot_v, float, &rot_v);
	double ctime = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_asteroids_pool, ctime, double, &ctime);

	whisker_ecs_p_set_prototype_named_tag(asteroids_asteroids_pool, t_ast);
	whisker_ecs_p_set_prototype_named_tag(asteroids_asteroids_pool, t_screen_cull);
}

void asteroids_pool_init_collisions()
{
	asteroids_component_collision collision = {0};
	whisker_ecs_p_set_prototype_named_component(asteroids_collisions_pool, collision, asteroids_component_collision, &collision);
}

void asteroids_pool_init_bullets()
{
	Vector2 pos_2d = {0,0};
	whisker_ecs_p_set_prototype_named_component(asteroids_bullets_pool, pos_2d, Vector2, &pos_2d);
	float rot = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_bullets_pool, rot, float, &rot);
	float radius = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_bullets_pool, radius, float, &radius);
	Vector2 vel_2d = {0,0};
	whisker_ecs_p_set_prototype_named_component(asteroids_bullets_pool, vel_2d, Vector2, &vel_2d);
	double ctime = 0;
	whisker_ecs_p_set_prototype_named_component(asteroids_bullets_pool, ctime, double, &ctime);

	whisker_ecs_p_set_prototype_named_tag(asteroids_bullets_pool, t_bullet);
	whisker_ecs_p_set_prototype_named_tag(asteroids_bullets_pool, t_move_dir);
	whisker_ecs_p_set_prototype_named_tag(asteroids_bullets_pool, t_screen_cull);
}

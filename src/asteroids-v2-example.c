/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : asteroids-v2-example
 * @created     : Monday Feb 17, 2025 21:20:16 CST
 */

#include "whisker_std.h"
#include "raylib.h"
#include "raymath.h"

#include "whisker.h"
#include "whisker_ecs.h"

const int asteroids_screen_width = 800;
const int asteroids_screen_height = 800;
const Vector2 asteroids_screen_center = {(float) asteroids_screen_width / 2, (float) asteroids_screen_height / 2};
#define DRAW_FPS false
#define DRAW_FPS_AVG_SAMPLES 20
#define DRAW_FRAMETIME true
#define DRAW_FRAMETIME_AVG_SAMPLES 5000

whisker_ecs *asteroids_ecs;

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
#define ASTEROID_SPAWN_START 400
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

// systems
void asteroids_system_movement_direction(whisker_ecs_system_update system);
void asteroids_system_movement_velocity(whisker_ecs_system_update system);
void asteroids_system_rotation_velocity(whisker_ecs_system_update system);
void asteroids_system_collision(whisker_ecs_system_update system);
void asteroids_system_collision_cull(whisker_ecs_system_update system);
void asteroids_system_destroy_offscreen(whisker_ecs_system_update system);
void asteroids_system_screen_wrap(whisker_ecs_system_update system);
void asteroids_system_entity_deferred_destroy(whisker_ecs_system_update system);

void asteroids_system_asteroid_spawn(whisker_ecs_system_update system);
void asteroids_system_player_controller(whisker_ecs_system_update system);
void asteroids_system_projectile_collide_destroy(whisker_ecs_system_update system);
void asteroids_system_asteroid_respawn_on_hit(whisker_ecs_system_update system);
void asteroids_system_asteroid_score(whisker_ecs_system_update system);
void asteroids_system_asteroid_hit_asteroid(whisker_ecs_system_update system);
void asteroids_system_player_hit_asteroid(whisker_ecs_system_update system);
void asteroids_system_player_hit_nudge(whisker_ecs_system_update system);
void asteroids_system_player_hit_cooldown(whisker_ecs_system_update system);
void asteroids_system_player_hit_to_recover(whisker_ecs_system_update system);
void asteroids_system_player_death_on_life_depleted(whisker_ecs_system_update system);

void asteroids_system_draw_asteroid(whisker_ecs_system_update system);
void asteroids_system_draw_projectile(whisker_ecs_system_update system);
void asteroids_system_draw_player(whisker_ecs_system_update system);
void asteroids_system_draw_hud(whisker_ecs_system_update system);
void asteroids_system_draw_game_over(whisker_ecs_system_update system);
void asteroids_system_draw_fps(whisker_ecs_system_update system);
void asteroids_system_draw_frame_time(whisker_ecs_system_update system);

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
	whisker_ecs_create(&asteroids_ecs);
}
void asteroids_deinit_ecs()
{
	whisker_ecs_free(asteroids_ecs);
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

	// spawn asteroids
	for (int i = 0; i < ASTEROID_SPAWN_START; ++i)
	{
		asteroids_spawn_asteroid();
	}

	// update
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_asteroid_spawn,
    "asteroid_spawn",
    "",
    "asteroid_spawn"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_movement_velocity,
    "movement_velocity",
    "",
    "pos_2d,vel_2d"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_rotation_velocity,
    "rotation_velocity",
    "",
    "rot,rot_v"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_player_controller,
    "player_controller",
    "t_player,p_state,pos_2d",
    "rot,rot_v,vel_2d,fire_time"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_movement_direction,
    "movement_velocity",
    "t_move_dir",
    "pos_2d,rot"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_collision,
    "collision_2d",
    "radius,pos_2d",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_destroy_offscreen,
    "destroy_offscreen",
    "t_screen_cull,pos_2d",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_screen_wrap,
    "screen_wrap",
    "t_screen_wrap",
    "pos_2d"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_collision_cull,
    "collision_cull",
    "collision",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_projectile_collide_destroy,
    "projectile_collide_destroy",
    "collision",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_asteroid_respawn_on_hit,
    "asteroid_score",
    "t_ast_hit,ast_size,pos_2d",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_asteroid_score,
    "asteroid_score",
    "t_ast_hit,ast_size",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_asteroid_hit_asteroid,
    "asteroid_hit_asteroid",
    "collision",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_player_hit_asteroid,
    "player_hit_asteroid",
    "collision",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_player_hit_nudge,
    "player_hit_nudge",
    "t_player,p_state,hit_collision,pos_2d",
    "vel_2d,rot_v"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_player_hit_cooldown,
    "player_hit_cooldown",
    "t_player,hit_time",
    "p_state"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_player_death_on_life_depleted,
    "player_death_on_life_depleted",
    "t_player,life",
    "p_state"
);
/*  */
/* // draw */
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_draw_asteroid,
    "draw_asteroid",
    "t_ast,pos_2d,rot,ast_size",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_draw_projectile,
    "draw_projectile",
    "t_bullet,pos_2d,rot",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_draw_player,
    "draw_player",
    "p_state,pos_2d,rot,hit_time",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_draw_hud,
    "draw_hud",
    "score,p_state,life",
    ""
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_draw_game_over,
    "draw_game_over",
    "p_state,score,hit_time,ctime",
    ""
);
/*  */
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_player_hit_to_recover,
    "player_hit_to_recover",
    "t_player",
    "p_state"
);
whisker_ecs_register_system(
    asteroids_ecs,
    asteroids_system_entity_deferred_destroy,
    "entity_deferred_destroy",
    "t_cull",
    ""
);
    /*  */
	/* if (DRAW_FPS) */
	/* { */
	/* 	register_system(system_draw_fps); */
	/* } */
	if (DRAW_FRAMETIME)
	{
		whisker_ecs_register_system(asteroids_ecs, asteroids_system_draw_frame_time, "draw_frametime", "frametime", "frametime");
	}
}

void asteroids_game_end()
{
	asteroids_game_state = ASTEROIDS_GAME_STATE_END;
	asteroids_game_time_ended = GetTime();
}

void asteroids_game_update()
{
	if (asteroids_game_state == ASTEROIDS_GAME_STATE_END && IsKeyPressed(KEY_R)) 
	{
		asteroids_game_init();
	}

	return;
}

#define NEARBLACK CLITERAL(Color){15, 15, 15, 266} 
void asteroids_game_draw_frame()
{
	BeginDrawing();

	ClearBackground(NEARBLACK);

	// draw game
	/* draw_asteroid(GetFrameTime()); */
	whisker_ecs_update(asteroids_ecs, GetFrameTime());
	
	if (asteroids_game_state == ASTEROIDS_GAME_STATE_END) 
	{
		// game end
	}
	else
	{
		// game running
	}

	EndDrawing();
}

void asteroids_create_player_entity()
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_(asteroids_ecs->entities, &e);

	Vector2 position = asteroids_screen_center;

	whisker_ecs_set(asteroids_ecs, pos_2d, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs, vel_2d, Vector2, e, &((Vector2) { }));
	whisker_ecs_set(asteroids_ecs, radius, float, e, (void*)&((float){PLAYER_RADIUS * 0.66f}));
	whisker_ecs_set(asteroids_ecs, rot, float, e, (void*)&(float){180});
	whisker_ecs_set(asteroids_ecs, rot_v, float, e, (void*)&(float){0});
	whisker_ecs_set(asteroids_ecs, ctime, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set(asteroids_ecs, score, int, e, (void*)&(int){0});
	whisker_ecs_set(asteroids_ecs, life, int, e, (void*)&(int){PLAYER_START_LIFE});
	whisker_ecs_set(asteroids_ecs, p_state, ASTEROIDS_PLAYER_STATE, e, (void*)&(ASTEROIDS_PLAYER_STATE){ASTEROIDS_PLAYER_STATE_DEFAULT});
	whisker_ecs_set(asteroids_ecs, fire_time, double, e, (void*)&(double){0});
	whisker_ecs_set(asteroids_ecs, hit_time, double, e, (void*)&(double){0});
	whisker_ecs_set(asteroids_ecs, hit_collision, asteroids_component_collision, e, (void*)&(asteroids_component_collision){});

	// TODO: store this on a central game/world entity instead of the player
	whisker_ecs_set(asteroids_ecs, fps, asteroids_component_fps, e, (void*)&(asteroids_component_fps){0});
	whisker_ecs_set(asteroids_ecs, frametime, asteroids_component_frametime, e, (void*)&(asteroids_component_frametime){0});
	whisker_ecs_set_tag(asteroids_ecs, t_player, e);    
	whisker_ecs_set_tag(asteroids_ecs, t_screen_wrap, e);    
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
	whisker_ecs_entity_id e = whisker_ecs_create_entity(asteroids_ecs);

	// set the entity component data
	whisker_ecs_set(asteroids_ecs, pos_2d, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs, vel_2d, Vector2, e, &(velocity));
	whisker_ecs_set(asteroids_ecs, ast_size, ASTEROIDS_ASTEROID_SIZE, e, (void*)&size);
	whisker_ecs_set(asteroids_ecs, radius, float, e, (void*)&((float){(ASTEROID_RADIUS * 0.6f) * size}));
	whisker_ecs_set(asteroids_ecs, rot, float, e, (void*)&rotation);
	whisker_ecs_set(asteroids_ecs, rot_v, float, e, (void*)&(rotation_velocity));
	whisker_ecs_set(asteroids_ecs, ctime, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set_tag(asteroids_ecs, t_ast, e);    
	whisker_ecs_set_tag(asteroids_ecs, t_screen_cull, e);    

	printf("add_asteroid:entity %d size %d at %fx%f\n", e.index, size, position.x, position.y);
}

void asteroids_add_projectile(Vector2 position, float rotation)
{
	// create an entity id
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_(asteroids_ecs->entities, &e);

	// set the entity component data
	whisker_ecs_set(asteroids_ecs, pos_2d, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs, rot, float, e, (void*)&rotation);
	whisker_ecs_set(asteroids_ecs, radius, float, e, (void*)&((float){((PROJECTILE_WIDTH + PROJECTILE_LENGTH) * 2) / 4}));

	// TODO: this is only because the recycled entity's data still exists and is
	// updated by the movement system because it looks for a POSITION component
	whisker_ecs_set(asteroids_ecs, vel_2d, Vector2, e, &((Vector2){}));

	whisker_ecs_set(asteroids_ecs, ctime, double, e, (void*)&(double){GetTime()});

	whisker_ecs_set_tag(asteroids_ecs, t_bullet, e);    
	whisker_ecs_set_tag(asteroids_ecs, t_move_dir, e);    
	whisker_ecs_set_tag(asteroids_ecs, t_screen_cull, e);    

	printf("add_projectile:rot %f at %fx%f\n", rotation, position.x, position.y);
}


/*************
*  systems  *
*************/
void asteroids_system_collision(whisker_ecs_system_update system)
{
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		
	float radius_size = *whisker_ecs_sys_get_read_alias(radius, float);		

	size_t entity_count = whisker_ecs_e_count(system.entities);
	whisker_ecs_entity_id id = system.entity_id;

	for (size_t ci = 0; ci < entity_count; ++ci)
	{
		whisker_ecs_entity_id ce = system.entities->entities[ci].id;

		// skip entities not matching the same archetype as the system, or
		// identitical entities as the one being processed
		if (id.index == ce.index || !whisker_ecs_a_match(system.system->read_archetype, system.entities->entities[ci].archetype)) {
			continue;
		}

		Vector2 colliding_position = *whisker_ecs_sys_get_read_alias_e(pos_2d, Vector2, ce);		
		float colliding_radius_size = *whisker_ecs_sys_get_read_alias_e(radius, float, ce);		

    	float distance = Vector2Distance(position, colliding_position);
    	if (distance <= (radius_size + colliding_radius_size))
    	{
			/* printf("system:collision:%zu->%zu (on)\n", e, ce); */

			/* DrawCircle(position.x, position.y, radius_size, Fade(GREEN, 0.6f)); */
			/* DrawCircle(colliding_position.x, colliding_position.y, colliding_radius_size, Fade(BLUE, 0.6f)); */

			whisker_ecs_entity_id collision = whisker_ecs_create_entity(asteroids_ecs);

			asteroids_component_collision col = {id, ce};
			whisker_ecs_set(asteroids_ecs, collision, asteroids_component_collision, collision, &col);
    	}
	}
}

void asteroids_system_collision_cull(whisker_ecs_system_update system)
{
	whisker_ecs_set_tag(asteroids_ecs, t_cull, system.entity_id);
}

void asteroids_system_asteroid_hit_asteroid(whisker_ecs_system_update system)
{
		asteroids_component_collision *collision = whisker_ecs_sys_get_read_alias(collision, asteroids_component_collision);		

		// find asteroids that collided with other asteroids
		if (
				// asteroid -> asteroid
				(whisker_ecs_has_tag(asteroids_ecs, t_ast, collision->entity_a) && whisker_ecs_has_tag(asteroids_ecs, t_ast, collision->entity_b))
				)
		{

			/* printf("collision %d: %d hit asteroid %d\n", system.entity->id.index, collision->entity_a.index, collision->entity_b.index); */

			// asteroid a
			Vector2* asteroida_velocity = whisker_ecs_get(asteroids_ecs,vel_2d, Vector2, collision->entity_a);
			float* asteroida_rotation_velocity = whisker_ecs_get(asteroids_ecs, rot_v, float, collision->entity_a);		


			Vector2 asteroida_position = *whisker_ecs_get(asteroids_ecs, pos_2d, Vector2, collision->entity_a);		
			Vector2 asteroida_hit_by_position = *whisker_ecs_get(asteroids_ecs, pos_2d, Vector2, collision->entity_b);		

			Vector2 asteroida_nudge_direction = Vector2Normalize(Vector2Subtract(asteroida_position, asteroida_hit_by_position));


			// asteroid b
			Vector2* asteroidb_velocity = whisker_ecs_get(asteroids_ecs, vel_2d, Vector2, collision->entity_b);
			float* asteroidb_rotation_velocity = whisker_ecs_get(asteroids_ecs, rot_v, float, collision->entity_b);		

			/* printf("system:asteroid_hit_asteroid 1:%d (%f) hit asteroid %d (%f)\n", collision->entity_a.index, *asteroida_rotation_velocity, collision->entity_b.index, *asteroidb_rotation_velocity); */

			*asteroida_rotation_velocity = (GetRandomValue(-180, 180));
			*asteroida_velocity = Vector2Scale(asteroida_nudge_direction, PLAYER_HIT_NUDGE_FORCE);
			*asteroidb_rotation_velocity = (GetRandomValue(-180, 180));

			/* printf("system:asteroid_hit_asteroid 1:%d (%f) hit asteroid %d (%f)\n", collision->entity_a.index, *asteroida_rotation_velocity, collision->entity_b.index, *asteroidb_rotation_velocity); */
		}
}



void asteroids_system_player_controller(whisker_ecs_system_update system)
{
	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_read_alias(p_state, ASTEROIDS_PLAYER_STATE);
	if (*player_state == ASTEROIDS_PLAYER_STATE_DEAD && IsKeyPressed(KEY_R)) {
		asteroids_game_state = ASTEROIDS_GAME_STATE_END;
		return;
	}
	if (*player_state != ASTEROIDS_PLAYER_STATE_DEFAULT) {
		return;
	}

	// player rotation
	float *rotation = whisker_ecs_sys_get_write_alias(rot, float);		
	float *rotation_velocity = whisker_ecs_sys_get_write_alias(rot_v, float);		

	int rotation_input = (int)IsKeyDown(KEY_D) - (int)IsKeyDown(KEY_A);
	/* *rotation += (rotation_input * PLAYER_ROTATION_VELOCITY * delta_time); */
	*rotation_velocity = PLAYER_ROTATION_VELOCITY * rotation_input;

	// player thrust
	Vector2 *velocity = whisker_ecs_sys_get_write_alias(vel_2d, Vector2);		

	Vector2 facing_direction = Vector2Rotate((Vector2){0, 1}, *rotation * DEG2RAD);

	int thrust_input = (int)IsKeyDown(KEY_W) - (int)IsKeyDown(KEY_S);
	if (thrust_input > 0) {
		*velocity = Vector2Add(*velocity, Vector2Scale(facing_direction, PLAYER_ACCELERATION * system.system->delta_time));

		float mag = Vector2Length(*velocity);
		if (mag > PLAYER_VELOCITY) {
			*velocity = Vector2Scale(*velocity, PLAYER_VELOCITY / mag);
		}
	}
	else
	{
		*velocity = Vector2Add(*velocity, Vector2Scale(*velocity, -PLAYER_DECELLERATION * system.system->delta_time));
	}

	// projectiles
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		
	double *projectile_time = whisker_ecs_sys_get_write_alias(fire_time, double);		

	double now = GetTime();
	bool can_fire = (now > *projectile_time + PROJECTILE_FIRE_RATE);

	if (IsKeyDown(KEY_SPACE) && can_fire) {
		asteroids_add_projectile(Vector2Add(position, Vector2Scale(facing_direction, 24)), *rotation);
		*projectile_time = now;
	}
}

void asteroids_system_projectile_collide_destroy(whisker_ecs_system_update system)
{
		asteroids_component_collision *collision = whisker_ecs_sys_get_read_alias(collision, asteroids_component_collision);		


		// find projectiles colliding with asteroids
		if (
				// asteroid -> asteroid
				(whisker_ecs_has_tag(asteroids_ecs, t_bullet, collision->entity_a) && whisker_ecs_has_tag(asteroids_ecs, t_ast, collision->entity_b))
				)
		{
			printf("system:projectile_collide_destroy:%zu hit asteroid %zu\n", collision->entity_a, collision->entity_b);

			// destroy existing asteroid and projectile
			whisker_ecs_set_tag(asteroids_ecs, t_cull, collision->entity_a);    
			whisker_ecs_set_tag(asteroids_ecs, t_cull, collision->entity_b);    

			whisker_ecs_set_tag(asteroids_ecs, t_ast_hit, collision->entity_b);    
		}
}

void asteroids_system_screen_wrap(whisker_ecs_system_update system)
{
	Vector2* position = whisker_ecs_sys_get_write_alias(pos_2d, Vector2);		
	if (
			position->x < 0 ||
			position->y < 0 ||
			position->x > asteroids_screen_width ||
			position->y > asteroids_screen_height
			)
	{
		printf("system:screen_wrap:e = %d\n", system.entity_id.index);

		if (position->x < 0)
		{
			position->x += asteroids_screen_width;
		}
		if (position->x > asteroids_screen_width)
		{
			position->x -= asteroids_screen_width;
		}
		if (position->y < 0)
		{
			position->y += asteroids_screen_height;
		}
		if (position->y > asteroids_screen_height)
		{
			position->y -= asteroids_screen_height;
		}
	}
}

void asteroids_system_movement_velocity(whisker_ecs_system_update system)
{
	Vector2 *position = whisker_ecs_sys_get_write_alias(pos_2d, Vector2);		
	Vector2 *velocity = whisker_ecs_sys_get_write_alias(vel_2d, Vector2);		

	position->x += velocity->x * system.system->delta_time;
	position->y += velocity->y * system.system->delta_time;
}

void asteroids_system_rotation_velocity(whisker_ecs_system_update system)
{
	float *rotation = whisker_ecs_sys_get_write_alias(rot, float);		
	float *rotation_velocity = whisker_ecs_sys_get_write_alias(rot_v, float);		

	*rotation += *rotation_velocity * system.system->delta_time;
}

void asteroids_system_movement_direction(whisker_ecs_system_update system)
{
	Vector2 *position = whisker_ecs_sys_get_write_alias(pos_2d, Vector2);		
	float rotation = *whisker_ecs_sys_get_write_alias(rot, float);		

	float radians = DEG2RAD * (rotation + 90.0f);
	position->x += PROJECTILE_VELOCITY * cos(radians) * system.system->delta_time;
	position->y += PROJECTILE_VELOCITY * sin(radians) * system.system->delta_time;
}

void asteroids_system_asteroid_spawn(whisker_ecs_system_update system)
{
	double *asteroid_spawn = whisker_ecs_sys_get_write_alias(asteroid_spawn, double);
	double time = GetTime();

	bool can_spawn = (time > *asteroid_spawn + ASTEROID_SPAWN_RATE);

	if (can_spawn)
	{
		asteroids_spawn_asteroid();
		*asteroid_spawn = time;
	}
}

void asteroids_system_destroy_offscreen(whisker_ecs_system_update system)
{
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		
	if (
			position.x < -(ASTEROID_OFF_SCREEN_PAD) ||
			position.y < -(ASTEROID_OFF_SCREEN_PAD) ||
			position.x > (asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD) ||
			position.y > (asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD)
			)
	{
		printf("system:destroy_offscreen:e = %d\n", system.entity_id.index);
		whisker_ecs_set_tag(asteroids_ecs, t_cull, system.entity_id);
	}
}

void asteroids_system_entity_deferred_destroy(whisker_ecs_system_update system)
{
	whisker_ecs_destroy_entity(asteroids_ecs, system.entity_id);
}

void asteroids_system_asteroid_score(whisker_ecs_system_update system)
{
	ASTEROIDS_ASTEROID_SIZE size = *whisker_ecs_sys_get_read_alias(ast_size, ASTEROIDS_ASTEROID_SIZE);

	size_t entity_count = whisker_ecs_e_count(system.entities);
	whisker_ecs_entity_id id = system.entity_id;

	for (size_t si = 0; si < entity_count; ++si)
	{
		whisker_ecs_entity_id se = system.entities->entities[si].id;

		if (whisker_ecs_has_tag(asteroids_ecs, score, se))
		{
			int add_score = (int)size * ASTEROID_SCORE;

			int *score = whisker_ecs_get(asteroids_ecs, score, int, se);
			*score += add_score;

			printf("system:asteroid_score:+%d points for %zu (%d total)\n", add_score, se, *score);
		}
	}
}

void asteroids_system_player_hit_asteroid(whisker_ecs_system_update system)
{
		asteroids_component_collision *collision = whisker_ecs_sys_get_read_alias(collision, asteroids_component_collision);		

		// find asteroids colliding with the player
		if (
				// asteroid -> asteroid
				(whisker_ecs_has_tag(asteroids_ecs, t_ast, collision->entity_a) && whisker_ecs_has_tag(asteroids_ecs, t_player, collision->entity_b))
				)
		{
			ASTEROIDS_PLAYER_STATE* player_state = whisker_ecs_get(asteroids_ecs, p_state, ASTEROIDS_PLAYER_STATE, collision->entity_b);

			// deal damage to the player, only in the default state
			// TODO: maybe figure out a more ECS-like way to handle this,
			// e.g. a component t_player_STATE_DEFAULT?
			if (*player_state != ASTEROIDS_PLAYER_STATE_DEFAULT) {
				return;
			}

			*player_state = ASTEROIDS_PLAYER_STATE_HIT;
			whisker_ecs_set(asteroids_ecs, hit_time, double, collision->entity_b, (void*)&(double){GetTime()});
			asteroids_component_collision col = {collision->entity_a, collision->entity_b};
			whisker_ecs_set(asteroids_ecs, hit_collision, asteroids_component_collision, collision->entity_b, &col);

			// TODO: refactor this into a separate system
			int* player_life = whisker_ecs_get(asteroids_ecs, life, int, collision->entity_b);
			ASTEROIDS_ASTEROID_SIZE* asteroid_size = whisker_ecs_get(asteroids_ecs, ast_size, ASTEROIDS_ASTEROID_SIZE, collision->entity_a);

			int damage = *asteroid_size * ASTEROID_DAMAGE;

			Vector2* asteroid_velocity = whisker_ecs_get(asteroids_ecs, vel_2d, Vector2, collision->entity_a);
			*asteroid_velocity = Vector2Scale(*asteroid_velocity, ASTEROID_HIT_VELOCITY_REDUCTION);
			float* rotation_velocity = whisker_ecs_get(asteroids_ecs, rot_v, float, collision->entity_a);		
			*rotation_velocity += (GetRandomValue(-270, 270));

			printf("system:player_damage:%zu hit player %zu (%d damage)\n", collision->entity_a, collision->entity_b, damage);

			*player_life -= damage;
			if (*player_life <= 0) {
				*player_life = 0;
			}
		}
}

void asteroids_system_player_hit_cooldown(whisker_ecs_system_update system)
{
	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_write_alias(p_state, ASTEROIDS_PLAYER_STATE);
	double* hit_time = whisker_ecs_sys_get_read_alias(hit_time, double);

	if (*player_state == ASTEROIDS_PLAYER_STATE_COOLDOWN && *hit_time + PLAYER_HIT_COOLDOWN < GetTime()) {
		printf("system:player_hit_cooldown:returning to default state\n");
		*player_state = ASTEROIDS_PLAYER_STATE_DEFAULT;
	}
}

void asteroids_system_player_death_on_life_depleted(whisker_ecs_system_update system)
{
	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_write_alias(p_state, ASTEROIDS_PLAYER_STATE);
	int* player_life = whisker_ecs_sys_get_read_alias(life, int);

	if (*player_state == ASTEROIDS_PLAYER_STATE_DEFAULT && *player_life <= 0) {
		printf("system:player_death_on_life_depleted\n");
		/* whisker_ecs_remove_component(asteroids_ecs, "vel_2d", sizeof(Vector2), system.entity_id); */
		*player_state = ASTEROIDS_PLAYER_STATE_DEAD;
	}
}

void asteroids_system_player_hit_nudge(whisker_ecs_system_update system)
{
	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_read_alias(p_state, ASTEROIDS_PLAYER_STATE);

	if (*player_state == ASTEROIDS_PLAYER_STATE_HIT) {
		printf("system:player_hit_nudge:nudging player from the hit\n");

		asteroids_component_collision* collision = whisker_ecs_sys_get_read_alias(hit_collision, asteroids_component_collision);		
		Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		
		Vector2 hit_by_position = *whisker_ecs_sys_get_read_alias_e(pos_2d, Vector2, collision->entity_a);		

		Vector2 nudge_direction = Vector2Normalize(Vector2Subtract(position, hit_by_position));

		Vector2* velocity = whisker_ecs_sys_get_write_alias(vel_2d, Vector2);		
		float* rotation_velocity = whisker_ecs_sys_get_write_alias(rot_v, float);		
		*rotation_velocity += (GetRandomValue(-270, 270));

		printf("system:player_hit_nudge:velocity before %fx%f\n", velocity->x, velocity->y);
		*velocity = Vector2Scale(nudge_direction, PLAYER_HIT_NUDGE_FORCE);

		printf("system:player_hit_nudge:velocity after %fx%f\n", velocity->x, velocity->y);
	}
}

void asteroids_system_player_hit_to_recover(whisker_ecs_system_update system)
{
	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_write_alias(p_state, ASTEROIDS_PLAYER_STATE);
	if (*player_state == ASTEROIDS_PLAYER_STATE_HIT) {
		printf("system:player_hit_to_recover\n");
		*player_state = ASTEROIDS_PLAYER_STATE_COOLDOWN;
	}
}

void asteroids_system_asteroid_respawn_on_hit(whisker_ecs_system_update system)
{
	ASTEROIDS_ASTEROID_SIZE size = *whisker_ecs_sys_get_read_alias(ast_size, ASTEROIDS_ASTEROID_SIZE);
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		

	int spawn_count = 0;

	ASTEROIDS_ASTEROID_SIZE new_size = ASTEROIDS_ASTEROID_SIZE_LARGE;

	switch (size) {
		case ASTEROIDS_ASTEROID_SIZE_LARGE:
			new_size = ASTEROIDS_ASTEROID_SIZE_MEDIUM;
			spawn_count = 3;
			break;
		case ASTEROIDS_ASTEROID_SIZE_MEDIUM:
			new_size = ASTEROIDS_ASTEROID_SIZE_SMALL;
			spawn_count = 2;
			break;
	}

	printf("system:asteroid_respawn_on_destroy:spawn count %d from size %d\n", spawn_count, size);

	for (int ii = 0; ii < spawn_count; ++ii)
	{
		printf("system:asteroid_respawn_on_destroy:spawning (from %d) size %d\n", system.entity_id.index, size);

		asteroids_add_asteroid(position, (Vector2) {GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2)), GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2))}, GetRandomValue(-360, 360) * DEG2RAD, GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), new_size);
	}
}


// draw systems
void asteroids_system_draw_asteroid(whisker_ecs_system_update system)
{
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		
	float rotation = *whisker_ecs_sys_get_read_alias(rot, float);		
	ASTEROIDS_ASTEROID_SIZE size = *whisker_ecs_sys_get_read_alias(ast_size, ASTEROIDS_ASTEROID_SIZE);

	DrawPolyLines(position, 3, 16 * (int)size, rotation, WHITE);
}

void asteroids_system_draw_projectile(whisker_ecs_system_update system)
{
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		
	float rotation = *whisker_ecs_sys_get_read_alias(rot, float);		

	Rectangle rect = { position.x, position.y, PROJECTILE_WIDTH, PROJECTILE_LENGTH };
	Vector2 origin = { rect.width / 2, rect.height / 2 };
	DrawRectanglePro(rect, origin, rotation, RED);
}

void asteroids_system_draw_player(whisker_ecs_system_update system)
{
	// get a component of type ASTEROIDS_PLAYER_STATE with the same name
	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_read_alias(p_state, ASTEROIDS_PLAYER_STATE);
	// get a component of type Vector2 with the alias name pos_2d
	Vector2 position = *whisker_ecs_sys_get_read_alias(pos_2d, Vector2);		

	if (*player_state == ASTEROIDS_PLAYER_STATE_DEAD)
	{
		double hit_time = *whisker_ecs_sys_get_read_alias(hit_time, double);		
		float explosion_radius = (GetTime() - (hit_time + PLAYER_HIT_COOLDOWN)) * 1750;
		const Color explosion_color = Fade(RED, 0.25f);

		if (explosion_radius > (asteroids_screen_width + asteroids_screen_height))
		{
			DrawRectangle(0, 0, asteroids_screen_width, asteroids_screen_height, explosion_color);
		}
		else
		{
			DrawCircle(position.x, position.y, explosion_radius, explosion_color);
		}
		return;
	}

	float rotation = *whisker_ecs_sys_get_read_alias(rot, float);		

	const Rectangle source = {0, 0, 32, 32};
	Rectangle dest = {position.x, position.y, 48, 48};
	Vector2 origin = {dest.width / 2, dest.height / 2};

	const Color color = Fade((*player_state == ASTEROIDS_PLAYER_STATE_COOLDOWN) ? RED : WHITE, ((*player_state == ASTEROIDS_PLAYER_STATE_COOLDOWN && (int)(GetTime() * 100) % 2)) ? 0.4f : 1.0f);
	DrawTexturePro(asteroids_player_ship, source, dest, origin, rotation + 180, color);
}


void asteroids_system_draw_hud(whisker_ecs_system_update system)
{
	const int font_size = 64;
	const Color black = Fade(BLACK, 0.8f);
	const Color white = Fade(WHITE, 0.8f);
	const int x_pad = 16;
	const int y_pad = 8;
	const int shadow = 8;

	ASTEROIDS_PLAYER_STATE *player_state = whisker_ecs_sys_get_read_alias(p_state, ASTEROIDS_PLAYER_STATE);
	if (*player_state == ASTEROIDS_PLAYER_STATE_DEAD)
	{
		return;
	}

	// score
	int score = *whisker_ecs_sys_get_read_alias(score, int);		

	char* score_text = TextFormat("%d POINTS", score);
	float score_measure = MeasureText(score_text, font_size);

	int score_x = asteroids_screen_width - score_measure;
	int score_y = font_size + y_pad;

	DrawText(score_text, score_x, score_y, 48, black);
	DrawText(score_text, score_x + shadow, score_y + shadow, 48, white);

	// life
	int life = *whisker_ecs_sys_get_read_alias(life, int);		

	char* life_text = TextFormat("%d%% LIFE", life);
	float life_measure = MeasureText(life_text, font_size);

	int life_x = x_pad;
	int life_y = font_size + y_pad;

	DrawText(life_text, life_x, life_y, 48, black);
	DrawText(life_text, life_x + shadow, life_y + shadow, 48, white);
}

void asteroids_system_draw_game_over(whisker_ecs_system_update system)
{
	ASTEROIDS_PLAYER_STATE* player_state = whisker_ecs_sys_get_read_alias(p_state, ASTEROIDS_PLAYER_STATE);

	if (*player_state == ASTEROIDS_PLAYER_STATE_DEAD)
	{
		// game end
		int score = *whisker_ecs_sys_get_read_alias(score, int);		
		double hit_time = *whisker_ecs_sys_get_read_alias(hit_time, double);
		double creation_time = *whisker_ecs_sys_get_read_alias(ctime, double);

		const char* game_over = "Game Over!";
		const char* press_r = "Press R to restart";
		const char* score_text = TextFormat("POINTS: %d", score);

		/* DrawRectangle(0, 0, screen_width, screen_height, Fade(RAYWHITE, 0.8f)); */
		DrawText(score_text, asteroids_screen_width / 2 - MeasureText(score_text, 60) / 2, asteroids_screen_height * 0.15f, 60, WHITE);
		DrawText(game_over, asteroids_screen_width / 2 - MeasureText(game_over, 40) / 2, asteroids_screen_height / 2 - 10, 40, WHITE);
		DrawText(press_r, asteroids_screen_width / 2 - MeasureText(press_r, 20) / 2, asteroids_screen_height * 0.75f, 20, WHITE);

		int mins = (int)(hit_time - creation_time) / 60;
		int secs = (int)(hit_time - creation_time) % 60;

		DrawText(TextFormat("Time played: %d minutes, %d seconds", mins, secs), 20, asteroids_screen_height - 40, 20, WHITE);
	}
}

void asteroids_system_draw_frame_time(whisker_ecs_system_update system)
{
	asteroids_component_frametime *frametime = whisker_ecs_sys_get_write_alias(frametime, asteroids_component_frametime);
	float current_frametime = system.system->delta_time * 1000;

	// update frametime samples
	/* if (GetTime() - 0.1 > frametime->time || frametime->time == 0) */
	/* { */
		/* frametime->time = GetTime(); */
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
	char* frametime_strings[frametime_string_count] = {};
	frametime_strings[0] = TextFormat("ms/f cur  : %.4f", current_frametime);
	frametime_strings[1] = TextFormat("ms/f avg  : %.4f", average_frametime);
	frametime_strings[2] = TextFormat("ms/f min  : %.4f", frametime->min);
	frametime_strings[3] = TextFormat("ms/f spike: %.4f", frametime->max);

	const int font_size = 48;
	const int line_padding = 8;
	const int top_padding = (font_size + line_padding) * 2;
	for (int i = 0; i < frametime_string_count; ++i)
	{
		DrawText(frametime_strings[i], 16, (i + 1) * (line_padding + font_size) + top_padding, font_size, RED);
	}
}

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


WECS_SYSTEM(asteroids_velocity_2d,
{
	pos_2d->x += vel_2d->x * delta_time;
	pos_2d->y += vel_2d->y * delta_time;
},
	WECS_READS(Vector2, vel_2d, 0)
	WECS_READ_WRITES(Vector2, pos_2d, 1, 1)
)

WECS_SYSTEM(asteroids_asteroid_spawn,
{
	double time = GetTime();

	bool can_spawn = (time > *asteroids_asteroid_spawn + ASTEROID_SPAWN_RATE);

	if (can_spawn)
	{
		asteroids_spawn_asteroid();
		*asteroids_asteroid_spawn = time;
	}
}, 
WECS_READS(double, asteroids_asteroid_spawn, 0)
)

WECS_SYSTEM(asteroids_rotation_velocity,
{
	*rot += *rot_v * system.system->delta_time;
},
	WECS_READS(float, rot_v, 0)
	WECS_READ_WRITES(float, rot, 1, 1)
)

WECS_SYSTEM(asteroids_movement_direction,
{
	float radians = DEG2RAD * (*rot + 90.0f);
	pos_2d->x += PROJECTILE_VELOCITY * cos(radians) * system.system->delta_time;
	pos_2d->y += PROJECTILE_VELOCITY * sin(radians) * system.system->delta_time;
},
	WECS_HAS(t_move_dir, 0)
	WECS_READS(float, rot, 1)
	WECS_READ_WRITES(Vector2, pos_2d, 2, 2)
)

WECS_SYSTEM(asteroids_player_controller,
{
	if (*p_state == ASTEROIDS_PLAYER_STATE_DEAD && IsKeyPressed(KEY_R)) {
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
		*vel_2d = Vector2Add(*vel_2d, Vector2Scale(facing_direction, PLAYER_ACCELERATION * delta_time));

		float mag = Vector2Length(*vel_2d);
		if (mag > PLAYER_VELOCITY) {
			*vel_2d = Vector2Scale(*vel_2d, PLAYER_VELOCITY / mag);
		}
	}
	else
	{
		*vel_2d = Vector2Add(*vel_2d, Vector2Scale(*vel_2d, -PLAYER_DECELLERATION * delta_time));
	}

	// projectiles
	double now = GetTime();
	bool can_fire = (now > *fire_time + PROJECTILE_FIRE_RATE);

	if (IsKeyDown(KEY_SPACE) && can_fire) {
		asteroids_add_projectile(Vector2Add(*pos_2d, Vector2Scale(facing_direction, 24)), *rot);
		*fire_time = now;
	}
},
	WECS_HAS(t_player, 0)
	WECS_READS(ASTEROIDS_PLAYER_STATE, p_state, 1)
	WECS_READS(Vector2, pos_2d, 2)
	WECS_READ_WRITES(float, rot, 3, 3)
	WECS_READ_WRITES(float, rot_v, 4, 4)
	WECS_READ_WRITES(Vector2, vel_2d, 5, 5)
	WECS_WRITES(double, fire_time, 6)
)

WECS_SYSTEM(asteroids_screen_wrap,
{
	if (
			pos_2d->x < 0 ||
			pos_2d->y < 0 ||
			pos_2d->x > asteroids_screen_width ||
			pos_2d->y > asteroids_screen_height
			)
	{
		debug_printf("system:screen_wrap:e = %d\n", system.entity_id.index);

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
},
	WECS_HAS(t_screen_wrap, 0)
	WECS_READ_WRITES(Vector2, pos_2d, 1, 1)
)

WECS_SYSTEM(asteroids_draw_frame_time,
{
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
	const char* frametime_string = TextFormat("%2.2f %2.2f %2.2f ms/f", average_frametime, frametime->min, frametime->max);

	const int font_size = 32;
	DrawText(frametime_string, 20, asteroids_screen_height - (font_size + 8) + 4, font_size, BLACK);
	DrawText(frametime_string, 16, asteroids_screen_height - (font_size + 8), font_size, WHITE);

	size_t entity_count = whisker_ecs_e_count(system.entities);
	size_t entity_count_alive = whisker_ecs_e_alive_count(system.entities);

	size_t asteroid_count = 0;
	for (size_t si = 0; si < entity_count; ++si)
	{
		if (!system.entities->entities[si].alive)
		{
			continue;
		}
		whisker_ecs_entity_id se = system.entities->entities[si].id;

		if (WECS_HAS_TAG_E(t_ast, 2, se))
		{
			asteroid_count++;
		}
	}

	const char* s1 = TextFormat("AC %d", asteroid_count);
	const char* s2 = TextFormat("EA %d", entity_count_alive);
	const char* s3 = TextFormat("ET %d", entity_count);
	DrawText(s1, 20, asteroids_screen_height - ((font_size + 8) * 4) + 4, font_size, BLACK);
	DrawText(s1, 16, asteroids_screen_height - ((font_size + 8) * 4), font_size, WHITE);
	DrawText(s2, 20, asteroids_screen_height - ((font_size + 8) * 3) + 4, font_size, BLACK);
	DrawText(s2, 16, asteroids_screen_height - ((font_size + 8) * 3), font_size, WHITE);
	DrawText(s3, 20, asteroids_screen_height - ((font_size + 8) * 2) + 4, font_size, BLACK);
	DrawText(s3, 16, asteroids_screen_height - ((font_size + 8) * 2), font_size, WHITE);
},
	WECS_HAS(asteroids_draw_frame_time, 0)
	WECS_WRITES(asteroids_component_frametime, frametime, 1)
	WECS_READS_TAG(t_ast, 2)
)


WECS_SYSTEM(asteroids_player_death_on_life_depleted,
{
	if (*p_state == ASTEROIDS_PLAYER_STATE_DEFAULT && *life <= 0) {
		debug_printf("system:player_death_on_life_depleted\n");
		/* whisker_ecs_remove_component(asteroids_ecs, "vel_2d", sizeof(Vector2), system.entity_id); */
		*p_state = ASTEROIDS_PLAYER_STATE_DEAD;
	}
},
	WECS_HAS(t_player, 0)
	WECS_READS(int, life, 1)
	WECS_READ_WRITES(ASTEROIDS_PLAYER_STATE, p_state, 2, 2)
)

WECS_SYSTEM(asteroids_player_hit_cooldown,
{
	if (*p_state == ASTEROIDS_PLAYER_STATE_COOLDOWN && *hit_time + PLAYER_HIT_COOLDOWN < GetTime()) {
		debug_printf("system:player_hit_cooldown:returning to default state\n");
		*p_state = ASTEROIDS_PLAYER_STATE_DEFAULT;
	}
},
	WECS_HAS(t_player, 0)
	WECS_READS(double, hit_time, 1)
	WECS_READ_WRITES(ASTEROIDS_PLAYER_STATE, p_state, 2, 2)
)

WECS_SYSTEM(asteroids_collision,
{
	size_t entity_count = warr_length(system.system->archetype_entities);
	whisker_ecs_entity_id id = system.entity_id;

	for (size_t ci = 0; ci < entity_count; ++ci)
	{
		whisker_ecs_entity_id ce = system.system->archetype_entities[ci];

		// skip entities not matching the same archetype as the system, or
		// identitical entities as the one being processed
		if (id.index == ce.index) {
			continue;
		}

		Vector2 *colliding_position = WECS_GET_READ_E(pos_2d, 0, ce);		
		float *colliding_radius_size = WECS_GET_READ_E(radius, 1, ce);		

    	float distance = Vector2Distance(*pos_2d, *colliding_position);
    	if (distance <= (*radius + *colliding_radius_size))
    	{
			/* debug_printf("system:collision:%zu->%zu (on)\n", e, ce); */

			/* DrawCircle(position.x, position.y, radius_size, Fade(GREEN, 0.6f)); */
			/* DrawCircle(colliding_position.x, colliding_position.y, colliding_radius_size, Fade(BLUE, 0.6f)); */

			whisker_ecs_entity_id collision_e = whisker_ecs_create_entity_deferred(system.system->entities);

			asteroids_component_collision *col = WECS_GET_WRITE_E(collision, 2, collision_e);
			col->entity_a = id;
			col->entity_b = ce;
    	}
	}
},
	WECS_READS(Vector2, pos_2d, 0)
	WECS_READS(float, radius, 1)
	WECS_WRITES(asteroids_component_collision, collision, 2)
)

WECS_SYSTEM(asteroids_collision_cull,
{
	WECS_TAG_ON(t_cull, 1);
},
	WECS_HAS(collision, 0)
	WECS_WRITES_TAG(t_cull, 1)
)

WECS_SYSTEM(asteroids_destroy_offscreen,
{
	if (pos_2d->x < -(ASTEROID_OFF_SCREEN_PAD) ||
		pos_2d->y < -(ASTEROID_OFF_SCREEN_PAD) ||
		pos_2d->x > (asteroids_screen_width + ASTEROID_OFF_SCREEN_PAD) ||
		pos_2d->y > (asteroids_screen_height + ASTEROID_OFF_SCREEN_PAD)
		)
	{
		debug_printf("system:destroy_offscreen:e = %d\n", entity_index);
		WECS_TAG_ON(t_cull, 2);
	}
},
	WECS_HAS(t_screen_cull, 0)
	WECS_READS(Vector2, pos_2d, 1)
	WECS_WRITES_TAG(t_cull, 2)
)

WECS_SYSTEM(asteroids_projectile_collide_destroy,
{
	if (WECS_MATCHES_ARCHETYPE(0, collision->entity_a) && WECS_MATCHES_ARCHETYPE(1, collision->entity_b))
	{
		debug_printf("system:projectile_collide_destroy:%zu hit asteroid %zu\n", collision->entity_a, collision->entity_b);

		// destroy existing asteroid and projectile
		WECS_TAG_ON_E(t_cull, 1, collision->entity_a);    
		WECS_TAG_ON_E(t_cull, 1, collision->entity_b);    

		WECS_TAG_ON_E(t_ast_hit, 2, collision->entity_b);    
	}
},
	WECS_READS(asteroids_component_collision, collision, 0)
	WECS_WRITES_TAG(t_cull, 1)
	WECS_WRITES_TAG(t_ast_hit, 2)
	WECS_USES_ARCHETYPE(0, t_bullet)
	WECS_USES_ARCHETYPE(1, t_ast)
)

WECS_SYSTEM(asteroids_asteroid_respawn_on_hit,
{
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

	debug_printf("system:asteroid_respawn_on_destroy:spawn count %d from size %d\n", spawn_count, *ast_size);

	// note: the add asteroids triggers a component realloc leaving *pos_2d
	// invalid after the first loop
	Vector2 spawn_pos = *pos_2d;
	ASTEROIDS_ASTEROID_SIZE asteroid_size = *ast_size;
	for (int ii = 0; ii < spawn_count; ++ii)
	{
		debug_printf("system:asteroid_respawn_on_destroy:spawning (from %d) size %d\n", system.entity_id.index, asteroid_size);

		asteroids_add_asteroid(spawn_pos, (Vector2) {GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2)), GetRandomValue(-(ASTEROID_VELOCITY_MAX / 2), (ASTEROID_VELOCITY_MAX / 2))}, GetRandomValue(-360, 360) * DEG2RAD, GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), new_size);
	}
},
	WECS_HAS(t_ast_hit, 0)
	WECS_READS(ASTEROIDS_ASTEROID_SIZE, ast_size, 1)
	WECS_READS(Vector2, pos_2d, 2)
)

WECS_SYSTEM(asteroids_asteroid_hit_asteroid,
{
	// find asteroids that collided with other asteroids
	if (WECS_MATCHES_ARCHETYPE(0, collision->entity_a) && WECS_MATCHES_ARCHETYPE(0, collision->entity_b))
	{

		/* debug_printf("collision %d: %d hit asteroid %d\n", system.entity->id.index, collision->entity_a.index, collision->entity_b.index); */

		// asteroid a
		Vector2 *asteroida_velocity = WECS_GET_WRITE_E(vel_2d, 1, collision->entity_a);
		float *asteroida_rotation_velocity = WECS_GET_WRITE_E(rot_v, 2, collision->entity_a);		


		Vector2 *asteroida_position = WECS_GET_READ_E(pos_2d, 2, collision->entity_a);		
		Vector2 *asteroida_hit_by_position = WECS_GET_READ_E(pos_2d, 2, collision->entity_b);		

		Vector2 asteroida_nudge_direction = Vector2Normalize(Vector2Subtract(*asteroida_position, *asteroida_hit_by_position));


		// asteroid b
		Vector2 *asteroidb_velocity = WECS_GET_READ_E(vel_2d, 1, collision->entity_b);
		float *asteroidb_rotation_velocity = WECS_GET_READ_E(rot_v, 2, collision->entity_b);		

		/* debug_printf("system:asteroid_hit_asteroid 1:%d (%f) hit asteroid %d (%f)\n", collision->entity_a.index, *asteroida_rotation_velocity, collision->entity_b.index, *asteroidb_rotation_velocity); */

		*asteroida_rotation_velocity = (GetRandomValue(-180, 180));
		*asteroida_velocity = Vector2Scale(asteroida_nudge_direction, PLAYER_HIT_NUDGE_FORCE);
		*asteroidb_rotation_velocity = (GetRandomValue(-180, 180));

		/* debug_printf("system:asteroid_hit_asteroid 1:%d (%f) hit asteroid %d (%f)\n", collision->entity_a.index, *asteroida_rotation_velocity, collision->entity_b.index, *asteroidb_rotation_velocity); */
	}
},
	WECS_READS(asteroids_component_collision, collision, 0)
	WECS_USES_ARCHETYPE(0, t_ast,pos_2d,vel_2d,rot_v)
	WECS_WRITES_ALL(Vector2, vel_2d, 1)
	WECS_WRITES_ALL(float, rot_v, 2)
	WECS_READS_ALL(Vector2, vel_2d, 1)
	WECS_READS_ALL(float, rot_v, 2)
	WECS_READS_ALL(Vector2, pos_2d, 3)
)

WECS_SYSTEM(asteroids_asteroid_score,
{
	size_t entity_count = whisker_ecs_e_count(system.entities);
	whisker_ecs_entity_id id = system.entity_id;

	// looping all entities just to set the score on the player is a bit of a
	// hack
	for (size_t si = 0; si < entity_count; ++si)
	{
		whisker_ecs_entity_id se = system.entities->entities[si].id;

		if (WECS_HAS_TAG_E(score, 2, se))
		{
			int *score = WECS_GET_WRITE_E(score, 2, se);

			int add_score = (int)*ast_size * ASTEROID_SCORE;

			*score += add_score;

			debug_printf("system:asteroid_score:+%d points for %zu (%d total)\n", add_score, se, *score);
		}
	}
},
	WECS_HAS(t_ast_hit, 0)
	WECS_READS(ASTEROIDS_ASTEROID_SIZE, ast_size, 1)
	WECS_WRITES_ALL(int, score, 2)
	WECS_READS_TAG(score, 2)
)

WECS_SYSTEM(asteroids_player_hit_asteroid,
{
	// find asteroids colliding with the player
	if (WECS_MATCHES_ARCHETYPE(0, collision->entity_a) && WECS_MATCHES_ARCHETYPE(1, collision->entity_b))
	{
		ASTEROIDS_PLAYER_STATE *player_state = WECS_GET_WRITE_E(p_state, 1, collision->entity_b);

		// deal damage to the player, only in the default state
		// TODO: maybe figure out a better way to handle this state
		if (*player_state != ASTEROIDS_PLAYER_STATE_DEFAULT) {
			return;
		}

		*player_state = ASTEROIDS_PLAYER_STATE_HIT;

		double *hit_time = WECS_GET_WRITE_E(hit_time, 2, collision->entity_b);
		*hit_time = GetTime();

		asteroids_component_collision *col = WECS_GET_WRITE_E(hit_collision, 3, collision->entity_b);
		col->entity_a = collision->entity_a;
		col->entity_b = collision->entity_b;

		// TODO: refactor this to use system access macros
		int* player_life = WECS_GET_WRITE_E(life, 4, collision->entity_b);
		ASTEROIDS_ASTEROID_SIZE* asteroid_size = WECS_GET_READ_E(ast_size, 5, collision->entity_a);

		int damage = *asteroid_size * ASTEROID_DAMAGE;

		Vector2* asteroid_velocity = WECS_GET_WRITE_E(vel_2d, 6, collision->entity_a);
		*asteroid_velocity = Vector2Scale(*asteroid_velocity, ASTEROID_HIT_VELOCITY_REDUCTION);
		float* rotation_velocity = WECS_GET_WRITE_E(rot_v, 7, collision->entity_a);		
		*rotation_velocity += (GetRandomValue(-270, 270));

		debug_printf("system:player_damage:%zu hit player %zu (%d damage)\n", collision->entity_a, collision->entity_b, damage);

		*player_life -= damage;
		if (*player_life <= 0) {
			*player_life = 0;
		}
	}
},
	WECS_USES_ARCHETYPE(0, t_ast)
	WECS_USES_ARCHETYPE(1, t_player)
	WECS_READS(asteroids_component_collision, collision, 0)
	WECS_WRITES_ALL(ASTEROIDS_PLAYER_STATE, p_state, 1)
	WECS_WRITES_ALL(double, hit_time, 2)
	WECS_WRITES_ALL(asteroids_component_collision, hit_collision, 3)
	WECS_WRITES_ALL(int, life, 4)
	WECS_READS_ALL(ASTEROIDS_ASTEROID_SIZE, ast_size, 5)
	WECS_WRITES_ALL(Vector2, vel_2d, 6)
	WECS_WRITES_ALL(float, rot_v, 7)
)

WECS_SYSTEM(asteroids_player_hit_nudge,
{
	if (*p_state == ASTEROIDS_PLAYER_STATE_HIT) {
		debug_printf("system:player_hit_nudge:nudging player from the hit\n");

		Vector2 *hit_by_position = WECS_GET_READ_E(pos_2d, 3, hit_collision->entity_a);		

		Vector2 nudge_direction = Vector2Normalize(Vector2Subtract(*pos_2d, *hit_by_position));

		*rot_v += (GetRandomValue(-270, 270));

		debug_printf("system:player_hit_nudge:velocity before %fx%f\n", vel_2d->x, vel_2d->y);
		*vel_2d = Vector2Scale(nudge_direction, PLAYER_HIT_NUDGE_FORCE);

		debug_printf("system:player_hit_nudge:velocity after %fx%f\n", vel_2d->x, vel_2d->y);
	}
},
	WECS_HAS(t_player, 0)
	WECS_READS(ASTEROIDS_PLAYER_STATE, p_state, 1)
	WECS_READS(asteroids_component_collision, hit_collision, 2)
	WECS_READS(Vector2, pos_2d, 3)
	WECS_READ_WRITES(float, rot_v, 4, 4)
	WECS_READ_WRITES(Vector2, vel_2d, 5,5)
)

WECS_SYSTEM(asteroids_player_hit_to_recover,
{
	if (*p_state == ASTEROIDS_PLAYER_STATE_HIT) {
		debug_printf("system:player_hit_to_recover\n");
		*p_state = ASTEROIDS_PLAYER_STATE_COOLDOWN;
	}
},
	WECS_HAS(t_player, 0)
	WECS_READ_WRITES(ASTEROIDS_PLAYER_STATE, p_state, 1, 1)
)

WECS_SYSTEM(asteroids_entity_deferred_destroy,
{
	whisker_ecs_destroy_entity(asteroids_ecs->entities, system.entity_id);
},
	WECS_HAS(t_cull, 0)
)

// draw systems
WECS_SYSTEM(asteroids_draw_asteroid,
{
	DrawPolyLines(*pos_2d, 3, 16 * (int)*ast_size, *rot, WHITE);
},
	WECS_HAS(t_ast, 0)
	WECS_READS(float, rot, 1)
	WECS_READS(Vector2, pos_2d, 2)
	WECS_READS(ASTEROIDS_ASTEROID_SIZE, ast_size, 3)
)

WECS_SYSTEM(asteroids_draw_projectile,
{
	Rectangle rect = {};
	rect.x = pos_2d->x;
	rect.y = pos_2d->y;
	rect.width = PROJECTILE_WIDTH;
	rect.height = PROJECTILE_LENGTH;
	Vector2 origin = {};
	origin.x = rect.width / 2;
	origin.y = rect.height / 2;
	DrawRectanglePro(rect, origin, *rot, RED);
},
	WECS_HAS(t_bullet, 0)
	WECS_READS(float, rot, 1)
	WECS_READS(Vector2, pos_2d, 2)
)

WECS_SYSTEM(asteroids_draw_player,
{
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
},
	WECS_HAS(t_player, 0)
	WECS_READS(float, rot, 1)
	WECS_READS(Vector2, pos_2d, 2)
	WECS_READS(double, hit_time, 3)
	WECS_READS(ASTEROIDS_PLAYER_STATE, p_state, 4)
)

WECS_SYSTEM(asteroids_draw_hud,
{
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
},
	WECS_HAS(t_player, 0)
	WECS_READS(ASTEROIDS_PLAYER_STATE, p_state, 1)
	WECS_READS(int, score, 2)
	WECS_READS(int, life, 3)
)

WECS_SYSTEM(asteroids_draw_game_over,
{
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
},
	WECS_HAS(t_player, 0)
	WECS_READS(ASTEROIDS_PLAYER_STATE, p_state, 1)
	WECS_READS(int, score, 2)
	WECS_READS(int, life, 3)
	WECS_READS(double, hit_time, 4)
	WECS_READS(double, ctime, 5)
)

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

	// register systems
	system_asteroids_asteroid_spawn_init(asteroids_ecs);
	system_asteroids_velocity_2d_init(asteroids_ecs);
	system_asteroids_rotation_velocity_init(asteroids_ecs);
	system_asteroids_player_controller_init(asteroids_ecs);
	system_asteroids_movement_direction_init(asteroids_ecs);
	system_asteroids_screen_wrap_init(asteroids_ecs);
	system_asteroids_collision_init(asteroids_ecs);
	system_asteroids_destroy_offscreen_init(asteroids_ecs);
	system_asteroids_collision_cull_init(asteroids_ecs);
	system_asteroids_projectile_collide_destroy_init(asteroids_ecs);
	system_asteroids_asteroid_respawn_on_hit_init(asteroids_ecs);
	system_asteroids_asteroid_score_init(asteroids_ecs);
	system_asteroids_asteroid_hit_asteroid_init(asteroids_ecs);
	system_asteroids_player_hit_asteroid_init(asteroids_ecs);
	system_asteroids_player_hit_nudge_init(asteroids_ecs);
	system_asteroids_player_hit_cooldown_init(asteroids_ecs);
	system_asteroids_player_death_on_life_depleted_init(asteroids_ecs);
	system_asteroids_player_hit_to_recover_init(asteroids_ecs);
	system_asteroids_entity_deferred_destroy_init(asteroids_ecs);
	/*  */
	/* // draw */
	system_asteroids_draw_asteroid_init(asteroids_ecs);
	system_asteroids_draw_projectile_init(asteroids_ecs);
	system_asteroids_draw_player_init(asteroids_ecs);
	system_asteroids_draw_hud_init(asteroids_ecs);
	system_asteroids_draw_game_over_init(asteroids_ecs);
	/*  */

	if (DRAW_FRAMETIME)
	{
		system_asteroids_draw_frame_time_init(asteroids_ecs);
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
	whisker_ecs_entity_id e = whisker_ecs_create_entity_deferred(asteroids_ecs->entities);

	Vector2 position = asteroids_screen_center;

	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, pos_2d, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, vel_2d, Vector2, e, &((Vector2) { }));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, radius, float, e, (void*)&((float){PLAYER_RADIUS * 0.66f}));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, rot, float, e, (void*)&(float){180});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, rot_v, float, e, (void*)&(float){0});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, ctime, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, score, int, e, (void*)&(int){0});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, life, int, e, (void*)&(int){PLAYER_START_LIFE});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, p_state, ASTEROIDS_PLAYER_STATE, e, (void*)&(ASTEROIDS_PLAYER_STATE){ASTEROIDS_PLAYER_STATE_DEFAULT});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, fire_time, double, e, (void*)&(double){0});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, hit_time, double, e, (void*)&(double){0});
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, hit_collision, asteroids_component_collision, e, (void*)&(asteroids_component_collision){});

	// TODO: store this on a central game/world entity instead of the player
	/* whisker_ecs_set(asteroids_ecs, fps, asteroids_component_fps, e, (void*)&(asteroids_component_fps){0}); */
	/* whisker_ecs_set(asteroids_ecs, frametime, asteroids_component_frametime, e, (void*)&(asteroids_component_frametime){0}); */
	whisker_ecs_set_tag(asteroids_ecs->entities, t_player, e);    
	whisker_ecs_set_tag(asteroids_ecs->entities, t_screen_wrap, e);    
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
	whisker_ecs_entity_id e = whisker_ecs_create_entity_deferred(asteroids_ecs->entities);

	// set the entity component data
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, pos_2d, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, vel_2d, Vector2, e, &(velocity));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, ast_size, ASTEROIDS_ASTEROID_SIZE, e, (void*)&size);
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, radius, float, e, (void*)&((float){(ASTEROID_RADIUS * 0.6f) * size}));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, rot, float, e, (void*)&rotation);
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, rot_v, float, e, (void*)&(rotation_velocity));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, ctime, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set_tag(asteroids_ecs->entities, t_ast, e);    
	whisker_ecs_set_tag(asteroids_ecs->entities, t_screen_cull, e);    

	debug_printf("add_asteroid:entity %d size %d at %fx%f\n", e.index, size, position.x, position.y);
}

void asteroids_add_projectile(Vector2 position, float rotation)
{
	// create an entity id
	whisker_ecs_entity_id e = whisker_ecs_create_entity_deferred(asteroids_ecs->entities);

	// set the entity component data
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, pos_2d, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, rot, float, e, (void*)&rotation);
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, radius, float, e, (void*)&((float){((PROJECTILE_WIDTH + PROJECTILE_LENGTH) * 2) / 4}));

	// TODO: this is only because the recycled entity's data still exists and is
	// updated by the movement system because it looks for a POSITION component
	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, vel_2d, Vector2, e, &((Vector2){}));

	whisker_ecs_set(asteroids_ecs->entities, asteroids_ecs->components, ctime, double, e, (void*)&(double){GetTime()});

	whisker_ecs_set_tag(asteroids_ecs->entities, t_bullet, e);    
	whisker_ecs_set_tag(asteroids_ecs->entities, t_move_dir, e);    
	whisker_ecs_set_tag(asteroids_ecs->entities, t_screen_cull, e);    

	debug_printf("add_projectile:rot %f at %fx%f\n", rotation, position.x, position.y);
}

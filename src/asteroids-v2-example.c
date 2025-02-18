/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : asteroids-v2-example
 * @created     : Monday Feb 17, 2025 21:20:16 CST
 */

#include "whisker_std.h"
#include "raylib.h"
/* #include "raymath.h" */

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
void asteroids_system_movement_direction(float delta_time);
void asteroids_system_movement_velocity(float delta_time);
void asteroids_system_rotation_velocity(float delta_time);
void asteroids_system_collision(float delta_time);
void asteroids_system_collision_cull(float delta_time);
void asteroids_system_destroy_offscreen(float delta_time);
void asteroids_system_screen_wrap(float delta_time);
void asteroids_system_entity_deferred_destroy(float delta_time);

void asteroids_system_asteroid_spawn(float delta_time);
void asteroids_system_player_controller(float delta_time);
void asteroids_system_projectile_collide_destroy(float delta_time);
void asteroids_system_asteroid_respawn_on_hit(float delta_time);
void asteroids_system_asteroid_score(float delta_time);
void asteroids_system_asteroid_hit_asteroid(float delta_time);
void asteroids_system_player_hit_asteroid(float delta_time);
void asteroids_system_player_hit_nudge(float delta_time);
void asteroids_system_player_hit_cooldown(float delta_time);
void asteroids_system_player_hit_to_recover(float delta_time);
void asteroids_system_player_death_on_life_depleted(float delta_time);

void asteroids_system_draw_asteroid(float delta_time);
void asteroids_system_draw_projectile(float delta_time);
void asteroids_system_draw_player(whisker_ecs_entity_id entity, double delta_time, struct whisker_ecs_system *system);
void asteroids_system_draw_hud(float delta_time);
void asteroids_system_draw_game_over(float delta_time);
void asteroids_system_draw_fps(float delta_time);
void asteroids_system_draw_frame_time(whisker_ecs_entity_id e, double delta_time, struct whisker_ecs_system *system);

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

	asteroids_init_ecs();

	// create player entity
	asteroids_create_player_entity();

	// spawn asteroids
	for (int i = 0; i < ASTEROID_SPAWN_START; ++i)
	{
		asteroids_spawn_asteroid();
	}

	// update
	/* register_system(system_asteroid_spawn); */
	/* register_system(system_movement_velocity); */
	/* register_system(system_rotation_velocity); */
	/* register_system(system_player_controller); */
	/* register_system(system_movement_direction); */
	/* register_system(system_collision); */
	/* register_system(system_destroy_offscreen); */
	/* register_system(system_screen_wrap); */
	/* register_system(system_collision_cull); */
	/* register_system(system_projectile_collide_destroy); */
	/* register_system(system_asteroid_respawn_on_hit); */
	/* register_system(system_asteroid_score); */
	/* register_system(system_asteroid_hit_asteroid); */
	/* register_system(system_player_hit_asteroid); */
	/* register_system(system_player_hit_nudge); */
	/* register_system(system_player_hit_cooldown); */
	/* register_system(system_player_death_on_life_depleted); */
    /*  */
	/* // draw */
	/* register_system(system_draw_asteroid); */
	/* register_system(system_draw_projectile); */
	whisker_ecs_register_system(asteroids_ecs, asteroids_system_draw_player, "COMPONENT_PLAYER_STATE,COMPONENT_POSITION,COMPONENT_ROTATION,COMPONENT_HIT_TIME");
	/* register_system(system_draw_hud); */
	/* register_system(system_draw_game_over); */
    /*  */
	/* register_system(system_player_hit_to_recover); */
	/* register_system(system_entity_deferred_destroy); */
    /*  */
	/* if (DRAW_FPS) */
	/* { */
	/* 	register_system(system_draw_fps); */
	/* } */
	if (DRAW_FRAMETIME)
	{
		whisker_ecs_register_system(asteroids_ecs, asteroids_system_draw_frame_time, "COMPONENT_FRAMETIME,");
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
	whisker_ecs_e_create(asteroids_ecs->entities, &e);

	Vector2 position = asteroids_screen_center;

	whisker_ecs_set(asteroids_ecs, COMPONENT_POSITION, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs, COMPONENT_VELOCITY, Vector2, e, &((Vector2) { }));
	whisker_ecs_set(asteroids_ecs, COMPONENT_RADIUS_SIZE, float, e, (void*)&((float){PLAYER_RADIUS * 0.66f}));
	whisker_ecs_set(asteroids_ecs, COMPONENT_ROTATION, float, e, (void*)&(float){180});
	whisker_ecs_set(asteroids_ecs, COMPONENT_ROTATION_VELOCITY, float, e, (void*)&(float){0});
	whisker_ecs_set(asteroids_ecs, COMPONENT_CREATION_TIME, double, e, (void*)&(double){GetTime()});
	whisker_ecs_set(asteroids_ecs, COMPONENT_SCORE, int, e, (void*)&(int){0});
	whisker_ecs_set(asteroids_ecs, COMPONENT_LIFE, int, e, (void*)&(int){PLAYER_START_LIFE});
	whisker_ecs_set(asteroids_ecs, COMPONENT_PLAYER_STATE, ASTEROIDS_PLAYER_STATE, e, (void*)&(ASTEROIDS_PLAYER_STATE){ASTEROIDS_PLAYER_STATE_DEFAULT});
	whisker_ecs_set(asteroids_ecs, COMPONENT_HIT_TIME, double, e, (void*)&(double){0});
	whisker_ecs_set(asteroids_ecs, COMPONENT_HIT_COLLISION, asteroids_component_collision, e, (void*)&(asteroids_component_collision){});

	// TODO: store this on a central game/world entity instead of the player
	whisker_ecs_set(asteroids_ecs, COMPONENT_FPS, asteroids_component_fps, e, (void*)&(asteroids_component_fps){0});
	whisker_ecs_set(asteroids_ecs, COMPONENT_FRAMETIME, asteroids_component_frametime, e, (void*)&(asteroids_component_frametime){0});
	whisker_ecs_set_tag(asteroids_ecs, COMPONENT_TAG_PLAYER, e);    
	whisker_ecs_set_tag(asteroids_ecs, COMPONENT_TAG_SCREEN_WRAP, e);    
}

void asteroids_spawn_asteroid()
{
/* 	// choose spawn position off screen */
/* 	Vector2 position = { */
/* 		.x = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, screen_width + ASTEROID_OFF_SCREEN_PAD)), */
/* 		.y = (GetRandomValue(-ASTEROID_OFF_SCREEN_PAD, screen_height + ASTEROID_OFF_SCREEN_PAD)), */
/* 	}; */
/*  */
/* 	// set the position randomly to the left/right/top/bottom of the screen */
/* 	if (GetRandomValue(0, 1)) */
/* 	{ */
/* 		position.x = (position.x > screen_center.x) ? screen_width + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD; */
/* 	} */
/* 	else */
/* 	{ */
/* 		position.y = (position.y > screen_center.y) ? screen_height + ASTEROID_OFF_SCREEN_PAD : -ASTEROID_OFF_SCREEN_PAD; */
/* 	} */
/*  */
/* 	// set random velocity angle */
/* 	Vector2 velocity = Vector2Subtract(screen_center, position); */
/* 	velocity = Vector2Scale(Vector2Normalize(velocity), GetRandomValue(ASTEROID_VELOCITY_MIN, ASTEROID_VELOCITY_MAX)); */
/* 	velocity = Vector2Rotate(velocity, (float) GetRandomValue(-ASTEROID_RANDOM_ANGLE, ASTEROID_RANDOM_ANGLE)); */
/*  */
/* 	// set random size */
/* 	int size_i = (ASTEROID_SIZE)rand() % 3; */
/* 	ASTEROID_SIZE size = asteroid_sizes[size_i]; */
/*  */
/* 	add_asteroid(position, velocity, (float)(rand() % 360), (float)GetRandomValue(ASTEROID_ROTATION_VELOCITY_MIN, ASTEROID_ROTATION_VELOCITY_MAX), size); */
/* } */
/*  */
/* void asteroids_add_asteroid(Vector2 position, Vector2 velocity, float rotation, float rotation_velocity, ASTEROID_SIZE size) */
/* { */
/* 	// create an entity id */
/* 	size_t e = create_entity(); */
/*  */
/* 	// set the entity component data */
/* 	SET_COMPONENT(COMPONENT_POSITION, Vector2, e, &(position)); */
/* 	SET_COMPONENT(COMPONENT_VELOCITY, Vector2, e, &(velocity)); */
/* 	SET_COMPONENT(COMPONENT_ASTEROID_SIZE, ASTEROID_SIZE, e, (void*)&size); */
/* 	SET_COMPONENT(COMPONENT_RADIUS_SIZE, float, e, (void*)&((float){(ASTEROID_RADIUS * 0.6f) * size})); */
/* 	SET_COMPONENT(COMPONENT_ROTATION, float, e, (void*)&rotation); */
/* 	SET_COMPONENT(COMPONENT_ROTATION_VELOCITY, float, e, (void*)&(rotation_velocity)); */
/* 	SET_COMPONENT(COMPONENT_CREATION_TIME, double, e, (void*)&(double){GetTime()}); */
/* 	add_component_entity(COMPONENT_TAG_ASTEROID, e);     */
/* 	add_component_entity(COMPONENT_TAG_DESTROY_OFFSCREEN, e);     */
/*  */
/* 	printf("add_asteroid:size %d at %fx%f\n", size, position.x, position.y); */
}

void asteroids_add_projectile(Vector2 position, float rotation)
{
	// create an entity id
	whisker_ecs_entity_id e;
	whisker_ecs_e_create(asteroids_ecs->entities, &e);

	// set the entity component data
	whisker_ecs_set(asteroids_ecs, COMPONENT_POSITION, Vector2, e, &(position));
	whisker_ecs_set(asteroids_ecs, COMPONENT_ROTATION, float, e, (void*)&rotation);
	whisker_ecs_set(asteroids_ecs, COMPONENT_RADIUS_SIZE, float, e, (void*)&((float){((PROJECTILE_WIDTH + PROJECTILE_LENGTH) * 2) / 4}));

	// TODO: this is only because the recycled entity's data still exists and is
	// updated by the movement system because it looks for a POSITION component
	whisker_ecs_set(asteroids_ecs, COMPONENT_VELOCITY, Vector2, e, &((Vector2){}));

	whisker_ecs_set(asteroids_ecs, COMPONENT_CREATION_TIME, double, e, (void*)&(double){GetTime()});

	whisker_ecs_set_tag(asteroids_ecs, COMPONENT_TAG_PROJECTILE, e);    
	whisker_ecs_set_tag(asteroids_ecs, COMPONENT_TAG_MOVE_DIRECTION, e);    
	whisker_ecs_set_tag(asteroids_ecs, COMPONENT_TAG_DESTROY_OFFSCREEN, e);    

	printf("add_projectile:rot %f at %fx%f\n", rotation, position.x, position.y);
}

void asteroids_system_draw_player(whisker_ecs_entity_id e, double delta_time, struct whisker_ecs_system *system)
{
	ASTEROIDS_PLAYER_STATE* player_state = whisker_ecs_get(asteroids_ecs, COMPONENT_PLAYER_STATE, ASTEROIDS_PLAYER_STATE, e);

	Vector2 position = *whisker_ecs_get(asteroids_ecs, COMPONENT_POSITION, Vector2, e);		

	if (*player_state == ASTEROIDS_PLAYER_STATE_DEAD)
	{
		double hit_time = *whisker_ecs_get(asteroids_ecs, COMPONENT_HIT_TIME, double, e);		
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

	float rotation = *whisker_ecs_get(asteroids_ecs, COMPONENT_ROTATION, float, e);		

	const Rectangle source = {0, 0, 32, 32};
	Rectangle dest = {position.x, position.y, 48, 48};
	Vector2 origin = {dest.width / 2, dest.height / 2};

	const Color color = Fade((*player_state == ASTEROIDS_PLAYER_STATE_COOLDOWN) ? RED : WHITE, ((*player_state == ASTEROIDS_PLAYER_STATE_COOLDOWN && (int)(GetTime() * 100) % 2)) ? 0.4f : 1.0f);
	DrawTexturePro(asteroids_player_ship, source, dest, origin, rotation + 180, color);
}

void asteroids_system_draw_frame_time(whisker_ecs_entity_id e, double delta_time, struct whisker_ecs_system *system)
{
	asteroids_component_frametime* frametime = whisker_ecs_get(asteroids_ecs, COMPONENT_FRAMETIME, asteroids_component_frametime, e);
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

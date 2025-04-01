/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:17 CST
 */

#include "whisker_std.h"
#include "whisker_macros.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_trie.h"
#include "whisker_sparse_set.h"
#include "whisker_time.h"
#include "whisker_thread_pool.h"

#ifndef WHISKER_ECS_H
#define WHISKER_ECS_H

/*******************************************************************************
*                               Macro Functions                               *
*******************************************************************************/

/**************************
*  ECS Interface Macros  *
**************************/

// macros
#define whisker_ecs_set_named(w, n, t, e, v) whisker_ecs_set_named_component(w, #n, sizeof(t), e, v)
#define whisker_ecs_get_named(w, n, e) whisker_ecs_get_named_component(w, #n, e)
#define whisker_ecs_remove_named(w, n, t, e) (t*) whisker_ecs_remove_named_component(w, #n, e)
#define whisker_ecs_has_named(w, n, e) whisker_ecs_has_named_component(w, #n, e)

#define whisker_ecs_set_named_tag(w, n, e) whisker_ecs_set_named_component(w, #n, sizeof(bool), e, &(bool){0})
#define whisker_ecs_remove_named_tag(w, n, e) whisker_ecs_remove_named_component(w, #n, e)

#define whisker_ecs_set(w, n, t, e, v) whisker_ecs_set_component(w, n, sizeof(t), e, v)
#define whisker_ecs_get(w, n, e) whisker_ecs_get_component(w, n, e)
#define whisker_ecs_remove(w, n, t, e) (t*) whisker_ecs_remove_component(w, n, e)

#define whisker_ecs_set_tag(w, n, e) whisker_ecs_set_component(w, n, sizeof(bool), e, &(bool){0})
#define whisker_ecs_remove_tag(w, n, e) whisker_ecs_remove_component(w, n, e)
#define whisker_ecs_has(w, n, e) whisker_ecs_has_component(w, n, e)


/***********************
*  ECS System Macros  *
***********************/

/* get component value from the given system iterator
*
*  this macro takes an iterator and a local component index and returns the
*  component value directly from the cached component array inside the iterator,
*  using the entity at the current cursor.
*/
#define whisker_ecs_itor_get(itor, idx) \
	itor->component_arrays[idx]->dense + itor->component_arrays_cursors[idx] * itor->component_arrays[idx]->element_size


/****************************
*  ECS Entity Pool Macros  *
****************************/

/* set a component on the pool's prototype entity
*
*  when requesting a new entity from a pool, the components and their values are
*  copied to the new entity from the "prototype entity".
*  entity pools use prototypical inheritence to define how a requested entity is
*  represented in terms of it's assigned components and their values.
*  the "prototype entity" is an entity which exists in each pool purely to hold
*  these values.
*
*  the below macros offer various methods of setting these prototype components
*  and tags
*/
#define whisker_ecs_p_set_prototype_component(p, c, s, v) \
	whisker_ecs_set_entity_pool_component_f(p, c, s, v)
#define whisker_ecs_p_set_prototype_named_component(p, c, s, v) \
	whisker_ecs_set_entity_pool_named_component_f(p, #c, sizeof(s), v)

#define whisker_ecs_p_set_prototype_tag(p, c) \
	whisker_ecs_set_entity_pool_component_f(p, c, sizeof(bool), &(bool){0})
#define whisker_ecs_p_set_prototype_named_tag(p, c) \
	whisker_ecs_set_entity_pool_named_component_f(p, #c, sizeof(bool), &(bool){0})


/*******************************************************************************
*                              Macro Definitions                              *
*******************************************************************************/

/*************************
*  Realloc Block Sizes  *
*************************/
/* reallocation of buffers is based on a grow-only block size
*
*  the following definitions control the sizes of the reallocation when a buffer
*  requires a capacity increase.
*  the realloc block size also defines the initial allocation size for the
*  specified buffer.
*/

#define WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE (16384 / sizeof(whisker_ecs_entity))
#define WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE (16384 / sizeof(whisker_ecs_entity_index))
#define WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE (16384 / sizeof(whisker_ecs_entity_deferred_action))

#define WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE (256 / sizeof(whisker_sparse_set *))
#define WHISKER_ECS_COMPONENT_REALLOC_BLOCK_SIZE_MULTIPLIER 1024
#define WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE (16384 / sizeof(struct whisker_ecs_component_deferred_action))

// the deferred action data is a byte buffer, so it doesn't calculate any opimal
// size for the block
#define WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE 16384

#define WHISKER_ECS_POOL_REALLOC_BLOCK_SIZE (16384 / sizeof(whisker_ecs_entity_id))


/*************************************
*  System Scheduler Process Phases  *
*************************************/
/* the system scheduler uses process phases to control when systems execute
*
*  by default the ECS init function defines some default process phases.
*/

#define WHISKER_ECS_PROCESS_PHASE_ON_STARTUP "w_phase_on_startup"
#define WHISKER_ECS_PROCESS_PHASE_PRE_LOAD "w_phase_pre_load"
#define WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE "w_phase_pre_update"
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE "w_phase_fixed_update"
#define WHISKER_ECS_PROCESS_PHASE_ON_UPDATE "w_phase_on_update"
#define WHISKER_ECS_PROCESS_PHASE_POST_UPDATE "w_phase_post_update"
#define WHISKER_ECS_PROCESS_PHASE_FINAL "w_phase_final"
#define WHISKER_ECS_PROCESS_PHASE_PRE_RENDER "w_phase_pre_render"
#define WHISKER_ECS_PROCESS_PHASE_ON_RENDER "w_phase_on_render"
#define WHISKER_ECS_PROCESS_PHASE_POST_RENDER "w_phase_post_render"
#define WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER "w_phase_final_render"

// render stages run uncapped by default
#define WHISKER_ECS_PROCESS_PHASE_PRE_RENDER_UNCAPPED true
#define WHISKER_ECS_PROCESS_PHASE_ON_RENDER_UNCAPPED true
#define WHISKER_ECS_PROCESS_PHASE_POST_RENDER_UNCAPPED true
#define WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER_UNCAPPED true

// phases reserved for system use
#define WHISKER_ECS_PROCESS_PHASE_RESERVED "w_phase_reserved"
#define WHISKER_ECS_PROCESS_PHASE_PRE_PHASE_ "w_phase_pre_phase_"
#define WHISKER_ECS_PROCESS_PHASE_POST_PHASE_ "w_phase_post_phase_"

/* fixed update process phase
*
*  the fixed update phase is different to the default phases
*/
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_RATE 60
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_CLAMP true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_SNAP true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_AVERAGE true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION_CLAMP true

/* default process phase
*
*  the default phase runs with an update target of 60, however it only allows 1
*  update to be performed. this effectively disables frame accumulation and
*  "catch-up" to ensure synced time steps don't trigger systems more than once.
*/
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE 60
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_UNCAPPED false
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_CLAMP true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_SNAP true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_AVERAGE true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION_CLAMP true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_UPDATE_COUNT_MAX 1

/* process phase time step indexes
*
*  during the initialisation of the default process phases, time steps are
*  created in the following order.
*/
#define WHISKER_ECS_PROCESS_PHASE_TIME_STEP_DEFAULT 0
#define WHISKER_ECS_PROCESS_PHASE_TIME_STEP_FIXED 1
#define WHISKER_ECS_PROCESS_PHASE_TIME_STEP_RENDERING 2


/******************************
*  System Threading Options  *
******************************/
/* systems can specify a threading mode during registration
*
*  AUTO: calculate the optimal number of cores (CPU threads / 2, or core count)
*  MAIN_THREAD: run the system on the main thread (aka, 0 threads)
*  1-N: manually specify the number of cores to run the system on
*/
#define WHISKER_ECS_PROCESS_THREADED_AUTO -1
#define WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD 0


/*******************************************************************************
*                                    Types                                    *
*******************************************************************************/
/* this section details the types used throughout the ECS
*
*  the types are split into sections by the domain:
*  - entities
*  - components
*  - systems
*/


/********************
*  Entities Types  *
********************/
// the entities types relate to the entity functionality of the ECS.
// a lot of things are entities and use entities in some way.

/* the main entity ID is a uint32_t
*
*  this index is considered the "real" entity ID.
*  it's the most stable, and does not change when the entity version changes.
*/
typedef uint32_t whisker_ecs_entity_index;


/* the full entity ID is a uint64_t
*
*  this type isn't used within the code except for the entity type definition.
*/
typedef uint64_t whisker_ecs_entity_id_raw;


/* deferred entity action enum
*
*  when destroying an entity in a deferred way, a deferred action is created
*  with one of these types. it specifies the action to perform on the entity
*  when processing the deferred actions.
*/
typedef enum WHISKER_ECS_ENTITY_DEFERRED_ACTION  
{
	WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE,
	WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY,
} WHISKER_ECS_ENTITY_DEFERRED_ACTION;


/* the main entity union type
*
*  everything which deals with entities works with this union type.
*  it allows accessing the uint32_t index, as well as the version.
*  optionally, the version can be discarded and replaced with 2 entity indexes
*  as entity_a and entity_b, allowing portable "pairs" as single entity IDs
*/
typedef struct whisker_ecs_entity_id
{
    union {
    	// the full raw uint64 ID
        whisker_ecs_entity_id_raw id;

        // the entity index + generation version
        // this is used for implementing alive checks
        struct {
            whisker_ecs_entity_index index;
            whisker_ecs_entity_index version;
        };

        // the relationship style A + B
        struct {
            whisker_ecs_entity_index entity_a;
            whisker_ecs_entity_index entity_b;
        };

        // currently reserved and subject to change
        struct {
            uint16_t short1;
            uint16_t short2;
            uint16_t short3;
            uint16_t short4;
        };
    };
} whisker_ecs_entity_id;


/* entity struct used by world's entity list
*
*  the entity struct is used as the type for the master array of entities.
*/
typedef struct whisker_ecs_entity
{
	// the full entity ID union
    whisker_ecs_entity_id id;

    // set when this entity is currently destroyed
    // note: this is not used for alive checks
    _Atomic bool destroyed;

    // an unmanaged entity is excluded from being processed during iterations
    // and process phase updates.
    // it is neither destroyed nor active.
    _Atomic bool unmanaged;

	// generic pointer to an unspecified managed by object, usually an entity
	// pool. when an entity is managed by something else, the usual destroy
	// methods are implemented to return to the pool its managed by.
	// when NULL, it's returned to the world's entity list.
    void *managed_by;

    // pointer to current name, if any
    char* name;
} whisker_ecs_entity;


/* struct for holding a deferred entity action request
*
*  currently the action only accepts a type of action and the entity ID
*/
typedef struct whisker_ecs_entity_deferred_action
{
	whisker_ecs_entity_id id;
	WHISKER_ECS_ENTITY_DEFERRED_ACTION action;
	
} whisker_ecs_entity_deferred_action;


/* a generic array struct definition for entity IDs
*
*  this struct is defined to be used generically as an entity ID array by other
*  implementations
*/
whisker_arr_declare_struct(whisker_ecs_entity_id, whisker_ecs_entity_id_array);



/* the world's entity state struct
*
*  the E in ECS is the entities, makes up part of the world by storing the state
*  related to the current entities.
*/
typedef struct whisker_ecs_entities
{
	// current list of entities used by the system
	whisker_arr_declare(whisker_ecs_entity, entities);

	// stack of destroyed entities, used when recycling
	whisker_arr_declare(whisker_ecs_entity_index, destroyed_entities);

	// trie of entity names mapping to indexes, allows for fast by-name lookup
	whisker_trie *entity_names;

	// stack of deferred actions to process
	whisker_arr_declare(whisker_ecs_entity_deferred_action, deferred_actions);

	// mutexes
	pthread_mutex_t deferred_actions_mutex;
	pthread_mutex_t create_entity_mutex;
} whisker_ecs_entities;


/**********************
*  Components Types  *
**********************/
// component types are used by the component array container management

/* component deferred actions type
*
*  actions performed on components are deferred, and given one of these types to
*  determine how to process the action during the later processing stage.
*  "DUMMY" actions are used to inform the system of a change, but without
*  performing any actual data action.
*/
enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION
{ 
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET,
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE,
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL,
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_ADD,
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_REMOVE,
};


/* component deferred action struct
*
*  each deferred action includes a component ID and an entity ID, along with the
*  data offset and size in the current deferred actions data buffer.
*
*/
struct whisker_ecs_component_deferred_action
{
	// component and entity ID to perform action on
	whisker_ecs_entity_id component_id;
	whisker_ecs_entity_id entity_id;

	// offset and size of the data in the buffer
	size_t data_offset;
	size_t data_size;

	// type of action
	enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action;

	// propagate: not used internally, but allows creating a deferred action which
	// indicates not to propagate the action to other interested modules e.g. the
	// component change event module.
	bool propagate;
};


/* the main component container for the world
*
*  the C in ECS, holds the component state as an array of sparse set pointers
*  for each component, accessed by entity index as component ID.
*/
typedef struct whisker_ecs_components
{
	// component sparse sets
	whisker_arr_declare(whisker_sparse_set *, components);

	// managed array of active component IDs
	whisker_arr_declare(whisker_ecs_entity_id, component_ids);

	// array of deferred actions
	whisker_arr_declare(struct whisker_ecs_component_deferred_action, deferred_actions);

	// deferred actions data buffer
	whisker_arr_declare(unsigned char, deferred_actions_data);

	// mutexes
	pthread_mutex_t grow_components_mutex;
	pthread_mutex_t deferred_actions_mutex;
} whisker_ecs_components;


/* component sort request struct
*
*  a component sort request is dispatched to a thread, so it needs to know the
*  current component container pointer and the component ID to sort.
*/
struct whisker_ecs_component_sort_request 
{
	whisker_ecs_components *components;
	whisker_ecs_entity_id component_id;
};



/***********************
*  Entity Pool Types  *
***********************/
/* struct holding the current state of an entity pool
*
*  an entity pool holds a list of pre-created entities which systems and modules
*  can use to request entities of dynamic archetypes, based on prototypical
*  inheritence.
*/
typedef struct whisker_ecs_pool
{
	// the entity ID of the prototypical entity
	whisker_ecs_entity_id prototype_entity_id;

	// list of component IDs attached to the prototype entity
	whisker_arr_declare(whisker_ecs_entity_id, component_ids);

	// sparse set acting as cache to quickly check if a component has been set
	whisker_sparse_set *component_ids_set;

	// array of entities used by the pool
	whisker_arr_declare(whisker_ecs_entity_id, entity_pool);

	// mutex
	pthread_mutex_t entity_pool_mutex;

	// inital count of entities to fill the pool with
	size_t inital_size;

	// count of entities to fill the pool after the initial size has been
	// drained
	size_t realloc_block_size;

	// whether or not to enable "propagate" on the deferred component actions
	_Atomic bool propagate_component_changes;

	// the ECS world this pool is used by
	struct whisker_ecs_world *world;

	// stat counters
	_Atomic size_t stat_cache_misses;
	_Atomic size_t stat_total_requests;
	_Atomic size_t stat_total_returns;
} whisker_ecs_pool;


/*******************
*  Systems Types  *
*******************/
/* types used by the system scheduler and systems
*
*  the scheduler is a combination of process phases, system
*  definitions, time steps, and system iterators.
*/

/* struct defining an individual process phase
*
*  a process phase runs during the main update, scheduled based on the attached
*  time step configuration.
*  process phases are registered as entities with an attached name, and systems
*  then register using the named process phase rather than an index or ID.
*/
typedef struct whisker_ecs_system_process_phase
{
	// the entity ID of the phase
	whisker_ecs_entity_id id;

	// the ID of the time step to use for this phase
	size_t time_step_id;

	// allows excluding from the normal phase scheduler
	bool manual_scheduling;
} whisker_ecs_system_process_phase;


/* struct defining a process phase's time step
*
*  each process phase has a time step, a cached update count and a bool to
*  indicate if the time step has been advanced this update
*/
typedef struct whisker_ecs_system_process_phase_time_step
{
	whisker_time_step time_step;
	size_t update_count;
	bool updated;
} whisker_ecs_system_process_phase_time_step;


/* system iterator struct used to iterate sparse sets
*
*  at the core of ECS systems is the iterator struct.
*  it's a state object representing the current iteration through one or more
*  component sparse sets.
*  systems can create and run any number of iterators specifying read, write and
*  optional components.
*/
typedef struct whisker_ecs_system_iterator
{
	// the master index points to the sparse set we're currently iterating
	size_t master_index;

	// current cursor position in the master iterator
	size_t cursor;
	size_t cursor_max;
	size_t count;
	whisker_ecs_entity_id entity_id;

	// array of component name IDs to match with
	whisker_arr_declare(whisker_ecs_entity_id, component_ids_rw);
	whisker_arr_declare(whisker_ecs_entity_id, component_ids_w);
	whisker_arr_declare(whisker_ecs_entity_id, component_ids_opt);

	// component arrays, including read/write/optional
	whisker_arr_declare(whisker_sparse_set *, component_arrays);
	whisker_arr_declare(size_t, component_arrays_cursors);
	
	// pointer to the current ECS world
	struct whisker_ecs_world *world;
} whisker_ecs_system_iterator;


/* system context struct passed to ECS systems
*
*  each ECS system will receive a pointer to a thread-local context.
*  the context provides access to the ECS world, and various other components of
*  the system definition including the current phase time step, the
*  pre-processed delta time, details on the thread ID and max thread count, and
*  the list of system iterators.
*/
typedef struct whisker_ecs_system_context
{
	// entity ID of system
	whisker_ecs_entity_id system_entity_id;

	// iterators used by this context
	whisker_sparse_set *iterators;

	// pointer to the current ECS world
	struct whisker_ecs_world *world;

	// pointer to the system's time step instance
	whisker_time_step *process_phase_time_step;

	// delta time since the last system update
	double delta_time;

	// managed thread id
	uint64_t thread_id;
	uint64_t thread_max;

	// system pointer from main system
	void (*system_ptr)(struct whisker_ecs_system_context*);
} whisker_ecs_system_context;

/* main system struct for a system definition
*
*  each registered system ends up as a system struct in the world's systems
*  list. it includes the system entity ID, the process phase ID, and other
*  details set at registration time such as the pointer to the system function,
*  thread configuration, and the process phase's time step.
*/
typedef struct whisker_ecs_system
{
	// entity IDs for the system and process phase
	whisker_ecs_entity_id entity_id;
	whisker_ecs_entity_id process_phase_id;

	// function pointer to execute with this system
	void (*system_ptr)(struct whisker_ecs_system_context*);

	// thread configuration
	int8_t thread_id;
	int8_t thread_count;

	// time since last update was ran
	double last_update;

	// the computed delta time from the process phase's time step
	double delta_time;

	// pointer to the process phase's time step
	whisker_time_step *process_phase_time_step;

	// the main ECS world instance this system is in
	struct whisker_ecs_world *world;

	// system contexts for each thread
	whisker_arr_declare(whisker_ecs_system_context, thread_contexts);

	// a thread pool instance, executes threaded work
	// note: if the system is registered with 0 cores, the thread pool is never
	// activated.
	whisker_thread_pool *thread_pool;
} whisker_ecs_system;


/* main struct holding systems registered to the ECS world
*
*  every ECS world has a list of systems, process phases and time steps used by
*  the scheduler.
*  the scheduler itself uses entities, components and iterators to iterate
*  systems in a process phase.
*/
typedef struct whisker_ecs_systems
{
	// list of systems registered
	whisker_arr_declare(whisker_ecs_system, systems);

	// list of process phases registered
	whisker_arr_declare(whisker_ecs_system_process_phase, process_phases);

	// list of process phase time steps registered
	whisker_arr_declare(whisker_ecs_system_process_phase_time_step, process_phase_time_steps);

	// ID of system to use for the main system scheduler
	size_t system_id;
} whisker_ecs_systems;


/*************************
*  ECS interface types  *
*************************/

/* main ECS world struct
*
*  the ECS world struct holds a world's entities, components and systems
*  containers.
*  things which deal with the ECS accept an ECS world struct.
*/
struct whisker_ecs_world
{
	whisker_ecs_entities *entities;
	whisker_ecs_components *components;
	whisker_ecs_systems *systems;
};


/* ECS state struct holding everything related to an ECS instance
*
*  this state struct holds the current ECS world, the system scheduler context,
*  and component sort requests.
*/
typedef struct whisker_ecs
{
	// currently active ECS world
	struct whisker_ecs_world *world;

	// system update context used by the core system scheduler
	whisker_ecs_system_context system_update_context;

	// thread pool for disaptching general work tasks
	whisker_thread_pool *general_thread_pool;

	// array of component sort requests
	whisker_arr_declare(struct whisker_ecs_component_sort_request, component_sort_requests);

	// IDs of the pre and post phase process phase to execute during each phase
	// update by the scheduler
	size_t process_phase_pre_idx;
	size_t process_phase_post_idx;
} whisker_ecs;

/*******************************************************************************
*                            Function Definitions                             *
*******************************************************************************/
// functions ending in _ are considered to be private functions used internally
// by other parts of the system. they are offered as part of the API however
// only should be ran if explicity done so.


/************************************************
*  Data Structure Creation and Initialisation  *
************************************************/
// functions are offered to manage the lifecycle and correctly initialise each
// part of the ECS's public and internal state.


/* ECS main state struct
*
*  creating the main state struct is the easiest way to create an ECS object.
*  it automatically initialises the state, an ECS world struct and all required
*  containers.
*/
whisker_ecs *whisker_ecs_create();
void whisker_ecs_free(whisker_ecs *ecs);


/* ECS world struct
*
*  the ECS world is used by all public API functions.
*  to create and init one, it requires the entities, components and systems
*  container structs.
*/
struct whisker_ecs_world *whisker_ecs_world_create();
struct whisker_ecs_world *whisker_ecs_world_create_and_init(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems);
void whisker_ecs_world_init(struct whisker_ecs_world *world, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems);


// internal containers
// the ECS manages its own instances, however nothing stops instances being
// created using their respective functions.

/* entity container
*
*  the entity container is used by the ECS to hold entities
*/
whisker_ecs_entities *whisker_ecs_create_and_init_entities_container_();
whisker_ecs_entities *whisker_ecs_create_entities_container_();
void whisker_ecs_init_entities_container_(whisker_ecs_entities *entities);
void whisker_ecs_free_entities_container_(whisker_ecs_entities *entities);
void whisker_ecs_free_entities_all_(whisker_ecs_entities *entities);


/* components container
*
*  the components container is the ECS's database
*/
whisker_ecs_components * whisker_ecs_create_components_container();
void whisker_ecs_init_components_container(whisker_ecs_components *components);
whisker_ecs_components *whisker_ecs_create_and_init_components_container();
void whisker_ecs_free_components_container(whisker_ecs_components *components);
void whisker_ecs_free_components_container_all(whisker_ecs_components *components);


/* systems container
*
*  the systems container holds the ECS's scheduler configuration
*/
whisker_ecs_systems * whisker_ecs_create_systems_container();
void whisker_ecs_init_systems_container(whisker_ecs_systems *systems);
whisker_ecs_systems * whisker_ecs_create_and_init_systems_container();
void whisker_ecs_free_systems_container(whisker_ecs_systems *systems);
void whisker_ecs_free_systems_container_all(whisker_ecs_systems *systems);

/* system context struct
*
*  each registered system gets a system context struct for every thread it
*  spawns. in the case that it spawns no threads, it spawns 0 + 1, where 1 acts
*  as the default system context used by the main thread.
*
*  system contexts don't need to be created for normal usage, they are created
*  and managed. systems are passed an instance of this struct.
*/
whisker_ecs_system_context *whisker_ecs_create_system_context();
whisker_ecs_system_context *whisker_ecs_create_and_init_system_context(whisker_ecs_system *system);
void whisker_ecs_init_system_context(whisker_ecs_system_context *context, whisker_ecs_system *system);
void whisker_ecs_free_system_context(whisker_ecs_system_context *context);
void whisker_ecs_free_system_context_all(whisker_ecs_system_context *context);


/* system struct
*
*  Further details on how to use the function
*  one exception is the system struct, which is created during registration of a
*  system. there's no function to create a system instance, however there is one
*  to free the allocations inside the system struct.
*/
void whisker_ecs_free_system(whisker_ecs_system *system);


/* system iterator
*
*  an iterator is used to iterate the results of a component set query.
*  they are created automatically when issuing a query request inside a system.
*/
whisker_ecs_system_iterator *whisker_ecs_create_iterator();
void whisker_ecs_free_iterator(whisker_ecs_system_iterator *itor);


/* entity pool
*
*  entity pools are used to create prototypical inheritence for entities.
*  each pool requires to be first created and initialised before using it with
*  any modules requiring a pool instance.
*
*  note: pools are not managed automatically and must be used explicity.
*/
whisker_ecs_pool *whisker_ecs_create_entity_pool();
whisker_ecs_pool *whisker_ecs_create_and_init_entity_pool(struct whisker_ecs_world *world, size_t count, size_t realloc_count);
void whisker_ecs_init_entity_pool(whisker_ecs_pool *pool, struct whisker_ecs_world *world, size_t count, size_t realloc_count);
void whisker_ecs_free_entity_pool(whisker_ecs_pool *pool);
void whisker_ecs_free_entity_pool_all(whisker_ecs_pool *pool);

/******************************
*  ECS public API functions  *
******************************/


whisker_ecs *whisker_ecs_create();
void whisker_ecs_free(whisker_ecs *ecs);

// system functions
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_context*), char *system_name, char *process_phase_name, size_t thread_count);
size_t whisker_ecs_register_process_phase_time_step(whisker_ecs *ecs, whisker_time_step time_step);
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, size_t time_step_id);
void whisker_ecs_set_process_phase_order(whisker_ecs *ecs, char **phase_names, size_t phase_count);

// system update functions
void whisker_ecs_update(whisker_ecs *ecs, double delta_time);

// system update deferred actions functions
void whisker_ecs_update_process_deferred_actions_(whisker_ecs *ecs);
void whisker_ecs_update_pre_process_destroyed_entities_(whisker_ecs *ecs);
void whisker_ecs_update_process_deferred_component_actions_(whisker_ecs *ecs);
void whisker_ecs_update_process_changed_components_(whisker_ecs *ecs);
void whisker_ecs_sort_component_thread_func_(void *component_sort_request, whisker_thread_pool_context *t);
void whisker_ecs_sort_component_thread_func_all_(void *component_sort_request, whisker_thread_pool_context *t);
void whisker_ecs_update_process_deferred_entity_actions_(whisker_ecs *ecs);

// entity shortcut functions
whisker_ecs_entity_id whisker_ecs_create_entity(struct whisker_ecs_world *world);
whisker_ecs_entity_id whisker_ecs_create_named_entity(struct whisker_ecs_world *world, char* name);
void whisker_ecs_destroy_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id);
void whisker_ecs_soft_destroy_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id);
void whisker_ecs_soft_revive_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_create_entity_deferred(struct whisker_ecs_world *world);
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(struct whisker_ecs_world *world, char* name);
void whisker_ecs_destroy_entity_deferred(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id);
bool whisker_ecs_is_alive(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id);
void whisker_ecs_create_deferred_entity_action(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, WHISKER_ECS_ENTITY_DEFERRED_ACTION action);

// component functions
whisker_ecs_entity_id whisker_ecs_component_id(struct whisker_ecs_world *world, char* component_name);
void *whisker_ecs_get_named_component(struct whisker_ecs_world *world, char *component_name, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_named_component(struct whisker_ecs_world *world, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
void whisker_ecs_remove_named_component(struct whisker_ecs_world *world, char *component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_named_component(struct whisker_ecs_world *world, char *component_name, whisker_ecs_entity_id entity_id);

void *whisker_ecs_get_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
void whisker_ecs_remove_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_create_deferred_component_action(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action, bool propagate);

// built-in systems
void whisker_ecs_system_deregister_startup_phase(whisker_ecs_system_context *context);

//////////////////
//  ECS entity  //
//////////////////


// entity management functions
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_e_create_deferred(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_e_create_new_deferred(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_e_create_(whisker_ecs_entities *entities);
whisker_ecs_entity_index whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities);
whisker_ecs_entity_id  whisker_ecs_e_create_new_(whisker_ecs_entities *entities);
void whisker_ecs_e_set_name(whisker_ecs_entities *entities, char *name, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name);
whisker_ecs_entity_id whisker_ecs_e_create_named_deferred(whisker_ecs_entities *entities, char *name);
whisker_ecs_entity_id whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char *name);
void whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_e_destroy_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_e_add_deffered_action(whisker_ecs_entities *entities, whisker_ecs_entity_deferred_action action);
void whisker_ecs_e_process_deferred(whisker_ecs_entities *entities);
void whisker_ecs_e_make_unmanaged(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_e_make_managed(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);

// utility functions
whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id);
whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name);
bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
size_t whisker_ecs_e_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities);
size_t whisker_ecs_e_destroyed_count(whisker_ecs_entities *entities);
struct whisker_ecs_entity_id_array* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names);
int whisker_ecs_e_compare_entity_ids_(const void *id_a, const void *id_b);
void whisker_ecs_e_sort_entity_array(whisker_ecs_entity_id *entities, size_t length);

/////////////////////
//  ECS component  //
/////////////////////





// component array management
void whisker_ecs_create_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size);
void whisker_ecs_grow_components_container_(whisker_ecs_components *components, size_t capacity);
whisker_sparse_set *whisker_ecs_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);
void whisker_ecs_free_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);
void whisker_ecs_sort_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);

// component management
void whisker_ecs_set_component_(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component);
void whisker_ecs_remove_component_(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_remove_all_components_(whisker_ecs_components *components, whisker_ecs_entity_id entity_id);


//////////////////
//  ECS system  //
//////////////////


void whisker_ecs_update_system(whisker_ecs_system *system, whisker_ecs_system_context *context);
void whisker_ecs_update_system_thread_func_(void *context, whisker_thread_pool_context *t);
void whisker_ecs_reset_process_phase_time_steps_(whisker_ecs_systems *systems);
void whisker_ecs_update_process_phase(struct whisker_ecs_world *world, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context);

// system iterator functions
whisker_ecs_system_iterator *whisker_ecs_query(whisker_ecs_system_context *context, size_t itor_index, char *read_components, char *write_components, char *optional_components);
bool whisker_ecs_iterate(whisker_ecs_system_iterator *itor);
void whisker_ecs_init_iterator(whisker_ecs_system_context *context, whisker_ecs_system_iterator *itor, char *read_components, char *write_components, char *optional_components);


/////////////////
//  ECS world  //
/////////////////

struct whisker_ecs_world *whisker_ecs_world_create();
struct whisker_ecs_world *whisker_ecs_world_create_and_init(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems);
void whisker_ecs_world_init(struct whisker_ecs_world *world, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems);


////////////////
//  ECS pool  //
////////////////



void whisker_ecs_set_entity_pool_component_f(whisker_ecs_pool *pool, whisker_ecs_entity_id component_id, size_t component_size, void *prototype_value);
void whisker_ecs_set_entity_pool_named_component_f(whisker_ecs_pool *pool, char* component_name, size_t component_size, void *prototype_value);
void whisker_ecs_set_entity_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id prototype_entity_id);
whisker_ecs_entity_id whisker_ecs_request_pool_entity(whisker_ecs_pool *pool);
whisker_ecs_entity_id whisker_ecs_create_pool_entity_deferred(whisker_ecs_pool *pool);
void whisker_ecs_init_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id, bool propagate_component_changes);
void whisker_ecs_deinit_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id, bool propagate_component_changes);
void whisker_ecs_return_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id);
void whisker_ecs_realloc_pool_entities(whisker_ecs_pool *pool);
void whisker_ecs_create_and_return_pool_entity(whisker_ecs_pool *pool, size_t count);
void whisker_ecs_add_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id);


#endif /* WHISKER_ECS_H */

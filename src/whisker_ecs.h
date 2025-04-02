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
#define w_set_named_t(w, n, t, e, v) w_set_named_component(w, #n, sizeof(t), e, v)
#define w_set_named_tag(w, n, e) w_set_named_component(w, #n, sizeof(bool), e, &(bool){0})
#define w_set_t(w, n, t, e, v) w_set_component(w, n, sizeof(t), e, v)
#define w_set_tag(w, n, e) w_set_component(w, n, sizeof(bool), e, &(bool){0})


/***********************
*  ECS System Macros  *
***********************/

/* get component value from the given system iterator
*
*  this macro takes an iterator and a local component index and returns the
*  component value directly from the cached component array inside the iterator,
*  using the entity at the current cursor.
*/
#define w_itor_get(itor, idx) \
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
#define w_pool_set_prototype_component(p, c, s, v) \
	w_set_entity_pool_component_f(p, c, sizeof(s), v)
#define w_pool_set_prototype_named_component(p, c, s, v) \
	w_set_entity_pool_named_component_f(p, #c, sizeof(s), v)

#define w_pool_set_prototype_tag(p, c) \
	w_set_entity_pool_component_f(p, c, sizeof(bool), &(bool){0})
#define w_pool_set_prototype_named_tag(p, c) \
	w_set_entity_pool_named_component_f(p, #c, sizeof(bool), &(bool){0})


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

#define W_ENTITY_REALLOC_BLOCK_SIZE (16384 / sizeof(struct w_entity))
#define W_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE (16384 / sizeof(w_entity_idx))
#define W_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE (16384 / sizeof(struct w_entity_deferred_action))

#define W_COMPONENT_SET_REALLOC_BLOCK_SIZE (256 / sizeof(w_sparse_set *))
#define W_COMPONENT_REALLOC_BLOCK_SIZE_MULTIPLIER 1024
#define W_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE (16384 / sizeof(struct w_component_deferred_action))

// the deferred action data is a byte buffer, so it doesn't calculate any opimal
// size for the block
#define W_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE 16384

#define W_POOL_REALLOC_BLOCK_SIZE (16384 / sizeof(w_entity_id))


/*************************************
*  System Scheduler Process Phases  *
*************************************/
/* the system scheduler uses process phases to control when systems execute
*
*  by default the ECS init function defines some default process phases.
*/

#define W_PHASE_ON_STARTUP "w_phase_on_startup"
#define W_PHASE_PRE_LOAD "w_phase_pre_load"
#define W_PHASE_PRE_UPDATE "w_phase_pre_update"
#define W_PHASE_FIXED_UPDATE "w_phase_fixed_update"
#define W_PHASE_ON_UPDATE "w_phase_on_update"
#define W_PHASE_POST_UPDATE "w_phase_post_update"
#define W_PHASE_FINAL "w_phase_final"
#define W_PHASE_PRE_RENDER "w_phase_pre_render"
#define W_PHASE_ON_RENDER "w_phase_on_render"
#define W_PHASE_POST_RENDER "w_phase_post_render"
#define W_PHASE_FINAL_RENDER "w_phase_final_render"

// render stages run uncapped by default
#define W_PHASE_PRE_RENDER_UNCAPPED true
#define W_PHASE_ON_RENDER_UNCAPPED true
#define W_PHASE_POST_RENDER_UNCAPPED true
#define W_PHASE_FINAL_RENDER_UNCAPPED true

// phases reserved for system use
#define W_PHASE_RESERVED "w_phase_reserved"
#define W_PHASE_PRE_PHASE_ "w_phase_pre_phase_"
#define W_PHASE_POST_PHASE_ "w_phase_post_phase_"

/* fixed update process phase
*
*  the fixed update phase is different to the default phases
*/
#define W_PHASE_FIXED_UPDATE_RATE 60
#define W_PHASE_FIXED_UPDATE_DELTA_CLAMP true
#define W_PHASE_FIXED_UPDATE_DELTA_SNAP true
#define W_PHASE_FIXED_UPDATE_DELTA_AVERAGE true
#define W_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION true
#define W_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION_CLAMP true

/* default process phase
*
*  the default phase runs with an update target of 60, however it only allows 1
*  update to be performed. this effectively disables frame accumulation and
*  "catch-up" to ensure synced time steps don't trigger systems more than once.
*/
#define W_PHASE_DEFAULT_RATE 60
#define W_PHASE_DEFAULT_UNCAPPED false
#define W_PHASE_DEFAULT_DELTA_CLAMP true
#define W_PHASE_DEFAULT_DELTA_SNAP true
#define W_PHASE_DEFAULT_DELTA_AVERAGE true
#define W_PHASE_DEFAULT_DELTA_ACCUMULATION true
#define W_PHASE_DEFAULT_DELTA_ACCUMULATION_CLAMP true
#define W_PHASE_DEFAULT_UPDATE_COUNT_MAX 1

/* process phase time step indexes
*
*  during the initialisation of the default process phases, time steps are
*  created in the following order.
*/
#define W_PHASE_TIME_STEP_DEFAULT 0
#define W_PHASE_TIME_STEP_FIXED 1
#define W_PHASE_TIME_STEP_RENDERING 2


/******************************
*  System Threading Options  *
******************************/
/* systems can specify a threading mode during registration
*
*  AUTO: calculate the optimal number of cores (CPU threads / 2, or core count)
*  MAIN_THREAD: run the system on the main thread (aka, 0 threads)
*  1-N: manually specify the number of cores to run the system on
*/
#define W_SCHED_THREAD_AUTO -1
#define W_SCHED_THREAD_MAIN_THREAD 0


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
typedef uint32_t w_entity_idx;


/* the full entity ID is a uint64_t
*
*  this type isn't used within the code except for the entity type definition.
*/
typedef uint64_t w_entity_id_raw;


/* deferred entity action enum
*
*  when destroying an entity in a deferred way, a deferred action is created
*  with one of these types. it specifies the action to perform on the entity
*  when processing the deferred actions.
*/
enum W_ENTITY_DEFERRED_ACTION  
{
	W_ENTITY_DEFERRED_ACTION_CREATE,
	W_ENTITY_DEFERRED_ACTION_DESTROY,
};


/* the main entity union type
*
*  everything which deals with entities works with this union type.
*  it allows accessing the uint32_t index, as well as the version.
*  optionally, the version can be discarded and replaced with 2 entity indexes
*  as entity_a and entity_b, allowing portable "pairs" as single entity IDs
*/
typedef union {
    // the full raw uint64 ID
    w_entity_id_raw id;

    // the entity index + generation version
    // this is used for implementing alive checks
    struct {
        w_entity_idx index;
        w_entity_idx version;
    };

    // the relationship style A + B
    struct {
        w_entity_idx entity_a;
        w_entity_idx entity_b;
    };

    // currently reserved and subject to change
    struct {
        uint16_t short1;
        uint16_t short2;
        uint16_t short3;
        uint16_t short4;
    };
} w_entity_id;


/* entity struct used by world's entity list
*
*  the entity struct is used as the type for the master array of entities.
*/
struct w_entity
{
	// the full entity ID union
    w_entity_id id;

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
};


/* struct for holding a deferred entity action request
*
*  currently the action only accepts a type of action and the entity ID
*/
struct w_entity_deferred_action
{
	w_entity_id id;
	enum W_ENTITY_DEFERRED_ACTION action;
	
};


/* a generic array struct definition for entity IDs
*
*  this struct is defined to be used generically as an entity ID array by other
*  implementations
*/
w_array_declare_struct(w_entity_id, w_entity_id_arr);



/* the world's entity state struct
*
*  the E in ECS is the entities, makes up part of the world by storing the state
*  related to the current entities.
*/
struct w_entities
{
	// current list of entities used by the system
	w_array_declare(struct w_entity, entities);

	// stack of destroyed entities, used when recycling
	w_array_declare(w_entity_idx, destroyed_entities);

	// trie of entity names mapping to indexes, allows for fast by-name lookup
	w_trie_node *entity_names;

	// stack of deferred actions to process
	w_array_declare(struct w_entity_deferred_action, deferred_actions);

	// mutexes
	pthread_mutex_t deferred_actions_mutex;
	pthread_mutex_t create_entity_mutex;
};


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
enum W_COMPONENT_DEFERRED_ACTION
{ 
	W_COMPONENT_DEFERRED_ACTION_SET,
	W_COMPONENT_DEFERRED_ACTION_REMOVE,
	W_COMPONENT_DEFERRED_ACTION_REMOVE_ALL,
	W_COMPONENT_DEFERRED_ACTION_DUMMY_ADD,
	W_COMPONENT_DEFERRED_ACTION_DUMMY_REMOVE,
};


/* component deferred action struct
*
*  each deferred action includes a component ID and an entity ID, along with the
*  data offset and size in the current deferred actions data buffer.
*
*/
struct w_component_deferred_action
{
	// component and entity ID to perform action on
	w_entity_id component_id;
	w_entity_id entity_id;

	// offset and size of the data in the buffer
	size_t data_offset;
	size_t data_size;

	// type of action
	enum W_COMPONENT_DEFERRED_ACTION action;

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
struct w_components
{
	// component sparse sets
	w_array_declare(w_sparse_set *, components);

	// managed array of active component IDs
	w_array_declare(w_entity_id, component_ids);

	// array of deferred actions
	w_array_declare(struct w_component_deferred_action, deferred_actions);

	// deferred actions data buffer
	w_array_declare(unsigned char, deferred_actions_data);

	// mutexes
	pthread_mutex_t grow_components_mutex;
	pthread_mutex_t deferred_actions_mutex;
};


/* component sort request struct
*
*  a component sort request is dispatched to a thread, so it needs to know the
*  current component container pointer and the component ID to sort.
*/
struct w_component_sort_request 
{
	struct w_world *world;
	w_entity_id component_id;
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
struct w_pool
{
	// the entity ID of the prototypical entity
	w_entity_id prototype_entity_id;

	// list of component IDs attached to the prototype entity
	w_array_declare(w_entity_id, component_ids);

	// sparse set acting as cache to quickly check if a component has been set
	w_sparse_set *component_ids_set;

	// array of entities used by the pool
	w_array_declare(w_entity_id, entity_pool);

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
	struct w_world *world;

	// stat counters
	_Atomic size_t stat_cache_misses;
	_Atomic size_t stat_total_requests;
	_Atomic size_t stat_total_returns;
};


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
struct w_process_phase
{
	// the entity ID of the phase
	w_entity_id id;

	// the ID of the time step to use for this phase
	size_t time_step_id;

	// allows excluding from the normal phase scheduler
	bool manual_scheduling;
};


/* struct defining a process phase's time step
*
*  each process phase has a time step, a cached update count and a bool to
*  indicate if the time step has been advanced this update
*/
struct w_process_phase_time_step
{
	w_time_step time_step;
	size_t update_count;
	bool updated;
};


/* system iterator struct used to iterate sparse sets
*
*  at the core of ECS systems is the iterator struct.
*  it's a state object representing the current iteration through one or more
*  component sparse sets.
*  systems can create and run any number of iterators specifying read, write and
*  optional components.
*/
struct w_iterator
{
	// the master index points to the sparse set we're currently iterating
	size_t master_index;

	// current cursor position in the master iterator
	size_t cursor;
	size_t cursor_max;
	size_t count;
	w_entity_id entity_id;

	// array of component name IDs to match with
	w_array_declare(w_entity_id, component_ids_rw);
	w_array_declare(w_entity_id, component_ids_w);
	w_array_declare(w_entity_id, component_ids_opt);

	// component arrays, including read/write/optional
	w_array_declare(w_sparse_set *, component_arrays);
	w_array_declare(size_t, component_arrays_cursors);
	
	// pointer to the current ECS world
	struct w_world *world;
};


/* system context struct passed to ECS systems
*
*  each ECS system will receive a pointer to a thread-local context.
*  the context provides access to the ECS world, and various other components of
*  the system definition including the current phase time step, the
*  pre-processed delta time, details on the thread ID and max thread count, and
*  the list of system iterators.
*/
struct w_sys_context
{
	// entity ID of system
	w_entity_id system_entity_id;

	// iterators used by this context
	w_sparse_set *iterators;

	// pointer to the current ECS world
	struct w_world *world;

	// pointer to the system's time step instance
	w_time_step *process_phase_time_step;

	// delta time since the last system update
	double delta_time;

	// managed thread id
	uint64_t thread_id;
	uint64_t thread_max;

	// system pointer from main system
	void (*system_ptr)(struct w_sys_context*);
};

/* main system struct for a system definition
*
*  each registered system ends up as a system struct in the world's systems
*  list. it includes the system entity ID, the process phase ID, and other
*  details set at registration time such as the pointer to the system function,
*  thread configuration, and the process phase's time step.
*/
struct w_system
{
	// entity IDs for the system and process phase
	w_entity_id entity_id;
	w_entity_id process_phase_id;

	// function pointer to execute with this system
	void (*system_ptr)(struct w_sys_context*);

	// thread configuration
	int8_t thread_id;
	int8_t thread_count;

	// time since last update was ran
	double last_update;

	// the computed delta time from the process phase's time step
	double delta_time;

	// pointer to the process phase's time step
	w_time_step *process_phase_time_step;

	// the main ECS world instance this system is in
	struct w_world *world;

	// system contexts for each thread
	w_array_declare(struct w_sys_context, thread_contexts);

	// a thread pool instance, executes threaded work
	// note: if the system is registered with 0 cores, the thread pool is never
	// activated.
	w_thread_pool *thread_pool;
};


/* main struct holding systems registered to the ECS world
*
*  every ECS world has a list of systems, process phases and time steps used by
*  the scheduler.
*  the scheduler itself uses entities, components and iterators to iterate
*  systems in a process phase.
*/
struct w_systems
{
	// list of systems registered
	w_array_declare(struct w_system, systems);

	// list of process phases registered
	w_array_declare(struct w_process_phase, process_phases);

	// list of process phase time steps registered
	w_array_declare(struct w_process_phase_time_step, process_phase_time_steps);

	// ID of system to use for the main system scheduler
	size_t system_id;
};


/*************************
*  ECS interface types  *
*************************/

/* main ECS world struct
*
*  the ECS world struct holds a world's entities, components and systems
*  containers.
*  things which deal with the ECS accept an ECS world struct.
*/
struct w_world
{
	struct w_entities *entities;
	struct w_components *components;
	struct w_systems *systems;
};


/* ECS state struct holding everything related to an ECS instance
*
*  this state struct holds the current ECS world, the system scheduler context,
*  and component sort requests.
*/
struct w_ecs
{
	// currently active ECS world
	struct w_world *world;

	// system update context used by the core system scheduler
	struct w_sys_context system_update_context;

	// thread pool for disaptching general work tasks
	w_thread_pool *general_thread_pool;

	// array of component sort requests
	w_array_declare(struct w_component_sort_request, component_sort_requests);

	// IDs of the pre and post phase process phase to execute during each phase
	// update by the scheduler
	size_t process_phase_pre_idx;
	size_t process_phase_post_idx;
};

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
struct w_ecs *w_ecs_create();
void w_ecs_free(struct w_ecs *ecs);


/* ECS world struct
*
*  the ECS world is used by all public API functions.
*  to create and init one, it requires the entities, components and systems
*  container structs.
*/
struct w_world *w_world_create();
struct w_world *w_world_create_and_init(struct w_entities *entities, struct w_components *components, struct w_systems *systems);
void w_world_init(struct w_world *world, struct w_entities *entities, struct w_components *components, struct w_systems *systems);


// internal containers
// the ECS manages its own instances, however nothing stops instances being
// created using their respective functions.

/* entity container
*
*  the entity container is used by the ECS to hold entities
*/
struct w_entities *w_create_and_init_entities_container_();
struct w_entities *w_create_entities_container_();
void w_init_entities_container_(struct w_entities *entities);
void w_free_entities_container_(struct w_entities *entities);
void w_free_entities_all_(struct w_entities *entities);


/* components container
*
*  the components container is the ECS's database
*/
struct w_components * w_create_components_container();
void w_init_components_container(struct w_components *components);
struct w_components *w_create_and_init_components_container();
void w_free_components_container(struct w_components *components);
void w_free_components_container_all(struct w_components *components);


/* systems container
*
*  the systems container holds the ECS's scheduler configuration
*/
struct w_systems * w_create_systems_container();
void w_init_systems_container(struct w_systems *systems);
struct w_systems * wcreate_and_init_systems_container();
void w_free_systems_container(struct w_systems *systems);
void w_free_systems_container_all(struct w_systems *systems);

/* system context struct
*
*  each registered system gets a system context struct for every thread it
*  spawns. in the case that it spawns no threads, it spawns 0 + 1, where 1 acts
*  as the default system context used by the main thread.
*
*  system contexts don't need to be created for normal usage, they are created
*  and managed. systems are passed an instance of this struct.
*/
struct w_sys_context *w_create_system_context();
struct w_sys_context *w_create_and_init_system_context(struct w_system *system);
void w_init_system_context(struct w_sys_context *context, struct w_system *system);
void w_free_system_context(struct w_sys_context *context);
void w_free_system_context_all(struct w_sys_context *context);


/* system struct
*
*  Further details on how to use the function
*  one exception is the system struct, which is created during registration of a
*  system. there's no function to create a system instance, however there is one
*  to free the allocations inside the system struct.
*/
void w_free_system(struct w_system *system);


/* system iterator
*
*  an iterator is used to iterate the results of a component set query.
*  they are created automatically when issuing a query request inside a system.
*/
struct w_iterator *w_create_iterator();
void w_free_iterator(struct w_iterator *itor);


/* entity pool
*
*  entity pools are used to create prototypical inheritence for entities.
*  each pool requires to be first created and initialised before using it with
*  any modules requiring a pool instance.
*
*  note: pools are not managed automatically and must be used explicity.
*/
struct w_pool *w_create_entity_pool();
struct w_pool *w_create_and_init_entity_pool(struct w_world *world, size_t count, size_t realloc_count);
void w_init_entity_pool(struct w_pool *pool, struct w_world *world, size_t count, size_t realloc_count);
void w_free_entity_pool(struct w_pool *pool);
void w_free_entity_pool_all(struct w_pool *pool);


/***********************
*  Entity Management  *
***********************/
/* entity management is the basic way to interface with the ECS
*
*  entities are ID indexes used to represent the existance of a thing in the ECS
*  world. because they are just IDs, they cannot "exist" and therefore there's
*  no dedicated way to create an entity object.
*
*  working with entities is done by passing the ECS world object to the entity
*  management functions.
*/

/* create an entity
*
*  entities are created by issuing a creation request, either with or without a
*  name. if there are no recycled entities a new one is created, otherwise a
*  recycled one is provided.
*
*  recycled entities have their version count increased by +1 to ensure they are
*  unique compared to the entity they were before being destroyed, despite
*  sharing the same index.
*/
w_entity_id w_create_entity(struct w_world *world);
w_entity_id w_create_named_entity(struct w_world *world, char* name);

/* create/destroy an entity - deferred
*
*  issuing a deferred entity request defers the action until the end of the
*  current process phase.
*  currently supported actions are creation and destruction.
*/
w_entity_id w_create_entity_deferred(struct w_world *world);
w_entity_id w_create_named_entity_deferred(struct w_world *world, char* name);
void w_destroy_entity_deferred(struct w_world *world, w_entity_id entity_id);
void w_create_deferred_entity_action(struct w_world *world, w_entity_id entity_id, enum W_ENTITY_DEFERRED_ACTION action);

/* destroy an entity
*
*  when an entity destruction request is made, the entity is immediately
*  destroyed and recycled for use by future creation requests.
*/
void w_destroy_entity(struct w_world *world, w_entity_id entity_id);

/* entity alive check
*
*  entities get their version increased when destroyed/returned to a pool. this
*  allows performing an alive check on the entity by comparing a previously
*  stored entity ID with the same index in the master list.
*  if the version has changed, or the entity has been destroyed, the alive check
*  returns false.
*/
bool w_is_entity_alive(struct w_world *world, w_entity_id entity_id);

/* set entity as managed/unmanaged
*
*  by default all created entities are "managed", meaning they are expected to
*  be returned to the world once destroyed via a destruction request.
*  an unmanaged entity is not managed by world, but still exists. when unmanaged
*  it is not included in any query result, and excluded from process phases.
*/
void w_set_entity_unmanaged(struct w_world *world, w_entity_id entity_id);
void w_set_entity_managed(struct w_world *world, w_entity_id entity_id);

/* set entity name
*
*  entities support being named. each name is unique and allows looking up an
*  entity by it's name.
*/
void w_set_entity_name(struct w_world *world, w_entity_id entity_id, char *name);


/* entity utilities
*
*  the following are various utility functions to work with entities
*/
// get an entity struct by ID
struct w_entity* w_get_entity(struct w_world *world, w_entity_id entity_id);
// get an entity struct by name
struct w_entity* w_get_named_entity(struct w_world *world, char* entity_name);

// get an entity ID from a raw value
w_entity_id w_entity_id_from_raw(w_entity_id_raw id);
size_t w_entity_count(struct w_world *world);
size_t w_alive_entity_count(struct w_world *world);
size_t w_destroyed_entity_count(struct w_world *world);
struct w_entity_id_arr* w_batch_create_named_entities(struct w_world *world, char* entity_names);
int w_compared_entity_ids_(const void *id_a, const void *id_b);
void w_sort_entity_array_(w_entity_id *entities, size_t length);

/******************
*  Entity Pools  *
******************/
/* entity pools allow pre-allocated entities and prototypical inheritence
*
*  in addition to creating and destroying entities directly with the world, you
*  can create an entity pool and set some default components and values on the
*  pool. this allows making requests to the pool to obtain an entity with these
*  components pre-existing.
*
*  each pool has a "prototype entity" containing the components. each requested
*  entity is a clone of this entity and it's component's values.
*/

/* request/return entity from/to pool
*
*  the basic usage of the pool involves requesting an entity and later returning
*  it. the pool automatically handles the initial allocation size, and further
*  reallocations if required.
*/
w_entity_id w_request_pool_entity(struct w_pool *pool);
void w_return_pool_entity(struct w_pool *pool, w_entity_id entity_id);

/* set pool prototype entity component
*
*  to make use of prototypical inheritence in a pool, components need to be set.
*  this automatically manages the underlying prototype entity that the
*  components end up on.
*/
void w_set_entity_pool_component_f(struct w_pool *pool, w_entity_id component_id, size_t component_size, void *prototype_value);
void w_set_entity_pool_named_component_f(struct w_pool *pool, char* component_name, size_t component_size, void *prototype_value);

/* set a different prototype entity for the pool
*
*  the prototype entity can be changed at runtime. every subsequently created
*  and destroyed/returned entities will be morphed into the new prototype
*  entities components without further actions.
*/
void w_set_entity_pool_entity(struct w_pool *pool, w_entity_id prototype_entity_id);


/* entity pool internal functions
*
*  the following functions are used by the pool to perform operations. they are
*  available to be called to provide more fine-grained control or usage of the
*  pool.
*/

// create a plain entity managed by the pool
w_entity_id w_create_pool_entity_deferred_(struct w_pool *pool);

/* init/deinit pool entity
*
*  for the pool to function entities need to be initialised when requested, and
*  deinitialised when being returned.
*/
void w_init_pool_entity_(struct w_pool *pool, w_entity_id entity_id, bool propagate_component_changes);
void w_deinit_pool_entity_(struct w_pool *pool, w_entity_id entity_id, bool propagate_component_changes);

// the following functions are used to reallocate the pools entities, create an
// amount and return them to the pool, or add an entity directly to the pool
// without initialising it
void w_realloc_pool_entities_(struct w_pool *pool);
void w_create_and_return_pool_entity_(struct w_pool *pool, size_t count);
void w_add_pool_entity_(struct w_pool *pool, w_entity_id entity_id);


/*****************************************
*  Component Array & Component Actions  *
*****************************************/
/* components are the ECS's data store
*
*  all data interactions with the ECS is done via components and of different
*  types and entity IDs. each entity can have a single component of any provided
*  ID or named component.
*/

/* get or create component ID by name
*
*  under the hood, components are entities themselves and leverage the named
*  entity feature to allow for uniquely named components.
*  this function takes a name, and returns an entity ID corresponding to the
*  component ID it's linked to.
*  if the component ID does not exist, the ID will be created. subsequent calls
*  for the same name will return the same ID, so the function can be considered
*  idempotent.
*/
w_entity_id w_component_id(struct w_world *world, char* component_name);

/* get component by ID or name
*
*  before obtaining the component from an entity, the the component needs to
*  exist in the entity. if it doesn't, it will return NULL.
*/
void *w_get_component(struct w_world *world, w_entity_id component_id, w_entity_id entity_id);
void *w_get_named_component(struct w_world *world, char *component_name, w_entity_id entity_id);

/* set component by ID or name
*
*  setting a component on an entity is an idempotent action. the first time a
*  component is set, it will create and initialise a backing storage set
*  and subsequently set the component for the given entity.
*  setting the same component overwrites the existing data for this component if
*  it exists already on the entity.
*  
*  note: component mutation actions are processed after each process phase has
*  finished running as a deferred action process.
*/
void *w_set_component(struct w_world *world, w_entity_id component_id, size_t component_size, w_entity_id entity_id, void *value);
void *w_set_named_component(struct w_world *world, char *component_name, size_t component_size, w_entity_id entity_id, void *value);

/* remove component by ID or name
*
*  removing a component from an entity is similar to setting it. if the
*  component does not exist, no action is taken except for the creation of the
*  underlying backing set for the provided component ID.
*
*  note: component mutation actions are processed after each process phase has
*  finished running as a deferred action process.
*/
void w_remove_component(struct w_world *world, w_entity_id component_id, w_entity_id entity_id);
void w_remove_named_component(struct w_world *world, char *component_name, w_entity_id entity_id);

/* check if an entity has the given component by ID or name
*
*  this function allows checking if an entity contains the given component. it
*  returns either true or false.
*  note: this will also return false if the underlying component set
*  doesn't exist for the provided ID or name.
*/
bool w_has_component(struct w_world *world, w_entity_id component_id, w_entity_id entity_id);
bool w_has_named_component(struct w_world *world, char *component_name, w_entity_id entity_id);


/* component sparse set management
*
*  the components container holds different component sparse sets. these functions
*  allow performing actions on the sparse sets container or sets themselves. 
*/

/* sort a component sparse set
*
*  sorting components happens as an automatic deferred action, however it can be
*  triggered any time for a specific component ID.
*/
void w_sort_component_array(struct w_world *world, w_entity_id component_id);

// create a component array with the given component ID and component type size
void w_create_component_array(struct w_world *world, w_entity_id component_id, size_t component_size);

// get a component array with the given component ID
w_sparse_set *w_get_component_array(struct w_world *world, w_entity_id component_id);

// internal function to increase the size of the components container to the
// required capacity
// note: this includes a mutex lock
void w_grow_components_container_(struct w_world *world, size_t capacity);


/* create a deferred component action
*
*  this internal function can be used to create a deferred component action
*  without using the public component set/remove API functions.
*  a possible reason to do that would be to custom the "action" and set
*  "propagate" to false to disable modules picking up this action.
*/
void w_create_deferred_component_action_(struct w_world *world, w_entity_id component_id, size_t component_size, w_entity_id entity_id, void *value, enum W_COMPONENT_DEFERRED_ACTION action, bool propagate);


/* set/remove components - non-deferred
*
*  these functions are the backing functions to set/remove components from a
*  component set. they are the same functions called during the processing of
*  deferred component actions.
*  note: if called manually must be followed by a component sort request to
*  uphold the integrity of the set, and are NOT thread-safe functions.
*/
void w_set_component_(struct w_world *world, w_entity_id component_id, size_t component_size, w_entity_id entity_id, void* component);
void w_remove_component_(struct w_world *world, w_entity_id component_id, w_entity_id entity_id);
void w_remove_all_components_(struct w_world *world, w_entity_id entity_id);


/**************************************
*  System Scheduling and Management  *
**************************************/
/* system scheduler and process phases
*
*  the scheduler runs systems in a defined higher-order and at set intervals.
*  each frame is made up of multiple process phases, process phases are made up
*  of synced time-steps and a registered systems. systems contain the logic
*  which operates on the world.
*/

/* register a system function
*
*  systems are single functions with a single argument: a system context object.
*  registering a systems requires giving the system a name and the name of a
*  process phase, as well as the desired threading configuration to run the
*  system with.
*/
struct w_system *w_register_system(struct w_ecs *ecs, void (*system_ptr)(struct w_sys_context*), char *system_name, char *process_phase_name, size_t thread_count);

/* register a process phase
*
*  process phases make up a frame's processing, and are executed in the defined
*  process phase order. registering a new one will append it to the end of the
*  existing process phase list.
*/
w_entity_id w_register_process_phase(struct w_ecs *ecs, char *phase_name, size_t time_step_id);

/* register a process phase time step
*
*  each process phase accepts a time step index, which is obtained by
*  registering the time step. time steps are synced across process phases with
*  the same index. this means, if a time step is provided with frame
*  accumulation enabled and this results in 2 frame updates, every phase using
*  this time step will be updated 2 times.
*/
size_t w_register_process_phase_time_step(struct w_ecs *ecs, w_time_step time_step);

/* set a custom process phase order
*
*  the ECS has a default process phase order allowing registering systems
*  without any additional setup or configuration. however, the process phase
*  list can be entirely customised by providing a char ** of process phase
*  names.
*
*  it's recommended to first register each custom phase along with custom time
*  steps, then pass in the desired order to this function. it will find and use
*  existing process phases, otherwise it will use the default time step index
*  (0) and register it as a new process phase.
*/
void w_set_process_phase_order(struct w_ecs *ecs, char **phase_names, size_t phase_count);

/* update systems
*
*  to perform a frame iteration, issue an update call providing the ECS state
*  object and an optional delta time override. currently, the delta time is not
*  used, however it may be used in future for something else.
*/
void w_scheduler_update(struct w_ecs *ecs, double delta_time);
void w_scheduler_update_system(struct w_system *system, struct w_sys_context *context);
void w_scheduler_update_process_phase(struct w_world *world, struct w_process_phase *process_phase, struct w_sys_context *default_context);
void w_scheduler_update_system_thread_func_(void *context, w_thread_pool_context *t);
void w_scheduler_reset_process_phase_time_steps_(struct w_systems *systems);


// system iterator functions
struct w_iterator *w_query(struct w_sys_context *context, size_t itor_index, char *read_components, char *write_components, char *optional_components);
bool w_iterate(struct w_iterator *itor);
void w_init_iterator(struct w_sys_context *context, struct w_iterator *itor, char *read_components, char *write_components, char *optional_components);


/* deferred actions processing
*
*  the ECS operates using deferred actions for mutations of components,
*  component sorting, and entity actions.
*  these internal functions are called by the main update function during
*  process phases.
*  they can be called during a system to perform an early commit.
*  since they are not public functions, they won't be further documented.
*/
void w_process_deferred_actions_(struct w_ecs *ecs);
void w_pre_process_destroyed_entities_(struct w_ecs *ecs);
void w_process_deferred_component_actions_(struct w_ecs *ecs);
void w_process_changed_components_(struct w_ecs *ecs);
void w_sort_component_thread_func_(void *component_sort_request, w_thread_pool_context *t);
void w_sort_component_thread_func_all_(void *component_sort_request, w_thread_pool_context *t);
void w_process_deferred_entity_actions_(struct w_ecs *ecs);



/* built-in ECS systems
*
*  the following functions are built-in systems providing functionalality used
*  by the ECS.
*/
// handles deregistration of the default ON_STARTUP phase to ensure it only runs
// a single time.
void w_system_deregister_startup_phase(struct w_sys_context *context);

/*************************
*  Internal Entity API  *
*************************/
/* 
*  the public entity API functions provide access to entity management features.
*  these functions use a combination, or all of, the backing entity API
*  functions. 
*  the backing functions provide much more explicit access and usage to the
*  entity management functionality of the ECS world.
*/

// entity management functions
w_entity_id w_entity_api_create_(struct w_world *world);
w_entity_id w_entity_api_create_deferred_(struct w_world *world);
w_entity_id w_entity_api_create_new_deferred_(struct w_world *world);
w_entity_id w_entity_api_create_unsafe_(struct w_world *world);
w_entity_idx w_entity_api_pop_recycled_(struct w_world *world);
w_entity_id  w_entity_api_create_new_(struct w_world *world);
void w_entity_api_set_name_(struct w_world *world, char *name, w_entity_id entity_id);
w_entity_id w_entity_api_create_named_(struct w_world *world, char *name);
w_entity_id w_entity_api_create_named_deferred_(struct w_world *world, char *name);
w_entity_id w_entity_api_create_named_unsafe_(struct w_world *world, char *name);
void w_entity_api_recycle_(struct w_world *world, w_entity_id entity_id);
void w_entity_api_destroy_(struct w_world *world, w_entity_id entity_id);
void w_entity_api_destroy_deferred_(struct w_world *world, w_entity_id entity_id);

#endif /* WHISKER_ECS_H */

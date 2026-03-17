/**
 * @author      : ElGatoPanzon
 * @file        : whisker_serialisation
 * @created     : Sunday Mar 15, 2026 21:24:35 CST
 * @description : Component serialisation hooks for world save/load and migrations
 */

#include "whisker_serialisation.h"

void wm_serialisation_init(struct w_ecs_world *world)
{
	struct wm_serialisation_registry *registry = w_mem_xmalloc_t(*registry);

	for (int i = 0; i < WM_SERIALISATION_HOOK_REGISTRY_COUNT; ++i)
		w_hook_registry_init(&registry->hooks[i]);

	w_ecs_singleton_set(world, WM_SERIALISATION_REGISTRY_NAME, registry);

	// register serialisation hooks
	wm_serialise_register_hooks_bool_(world);
}

void wm_serialisation_free(struct w_ecs_world *world)
{
	struct wm_serialisation_registry *registry = w_ecs_singleton_get(world, WM_SERIALISATION_REGISTRY_NAME);

	if (!registry) return;

	for (int i = 0; i < WM_SERIALISATION_HOOK_REGISTRY_COUNT; ++i)
		w_hook_registry_free(&registry->hooks[i]);

	free(registry);
}

w_pack32x2 w_serialisation_register_component_hooks(struct w_ecs_world *world, uint type_id, w_hook_fn serialise_hook, w_hook_fn deserialise_hook)
{
	struct wm_serialisation_registry *registry = w_ecs_singleton_get(world, WM_SERIALISATION_REGISTRY_NAME);

	w_pack32x2 result = to_w_pack(w_pack32x2, UINT64_MAX);
	if (!registry) return result;

	result.left = w_hook_registry_register_hook(&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_COMPONENT_SERIALISE], type_id, serialise_hook);

	result.right = w_hook_registry_register_hook(&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_COMPONENT_DESERIALISE], type_id, deserialise_hook);

	return result;
}

uint32_t w_serialisation_register_lifecycle_hook(struct w_ecs_world *world, enum WM_SERIALISATION_LIFECYCLE_HOOK hook_type, w_hook_fn hook_fn)
{
	struct wm_serialisation_registry *registry = w_ecs_singleton_get(world, WM_SERIALISATION_REGISTRY_NAME);
	if (!registry) return UINT32_MAX;

	return w_hook_registry_register_hook(&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE], (uint) hook_type, hook_fn);
}


/*****************************
*  serialisation functions  *
*****************************/

bool w_serialisation_dump_to_buffer(struct w_ecs_world *world, struct wm_serialisation_ctx *ctx)
{
	struct wm_serialisation_registry *registry = w_ecs_singleton_get(world, WM_SERIALISATION_REGISTRY_NAME);

	if (!registry) return false;

	// init the ctx
	w_array_init_t(ctx->buffer, WM_SERIALISATION_BUFFER_BLOCK_SIZE);
	ctx->entities_saved = 0;
	ctx->components_saved = 0;
	ctx->err = 0;
	ctx->err_message = NULL;

	// serialisation rules:
	// 1. only serialise entities with names
	// 2. exclude named entities with WM_SERIALISATION_NO_SERIALISE_TAG_NAME tag component
	// 3. all components with type w_entity_id serialise as name not ID
	
	// prepare list of persistent entities
	ctx->entities_length = 0;
	ctx->entities_size = 0;
	ctx->entities = w_serialisation_get_persistent_entity_list(world, &ctx->entities_length, &ctx->entities_size);

	ctx->entities_saved = ctx->entities_length;

	// prepare list of component IDs
	ctx->components_length = 0;
	ctx->components_size = 0;
	ctx->components = w_serialisation_get_components_list(world, &ctx->components_length, &ctx->components_size);

	ctx->components_saved = ctx->components_length;

	// step 0: trigger pre-save lifecycle hooks
	w_hook_registry_run_hooks(
		&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE],
		WM_SERIALISATION_LIFECYCLE_HOOK_PRE_SAVE, 
		world, ctx
	);

	// step 1: write metadata
	uint32_t version = w_serialisation_get_version(registry, ctx);

	// # whisker save
	w_serialisation_push_ctx_command_(ctx, "# whisker save");
	// # created YYYY-MM-DD HH:MM:SS
	char *timestamp = w_strftime("%Y-%m-%d %H:%M:%S");
	w_serialisation_push_ctx_command_f_(ctx, "# created %s", timestamp);
	free(timestamp);
	// # version X
	w_serialisation_push_ctx_command_f_(ctx, "# version %u", version);

	w_serialisation_push_ctx_command_f_(ctx, "# entities %u", ctx->entities_length);
	w_serialisation_push_ctx_command_f_(ctx, "# components %u", ctx->components_length);
	
	
	// step 2: write entities commands
	// entity command: entity "name"
	for (size_t i = 0; i < ctx->entities_length; ++i)
	{
		w_serialisation_push_ctx_command_f_(ctx, "entity \"%s\"", w_ecs_get_entity_name(world, ctx->entities[i]));
	}
	
	
	// step 3: component data
	// component command: set "entity_name" "comp_name" type_name [params...]
	
	// temp component ctx
	struct wm_serialisation_component_ctx comp_ctx;
	comp_ctx.ctx = ctx;
	
	// loop over component IDs, fetch entry, execute hooks
	for (size_t ci = 0; ci < ctx->components_length; ++ci)
	{
		w_entity_id component_entity = ctx->components[ci];
		struct w_component_entry *entry = w_component_registry_get_entry(&world->components, component_entity);

		// assign comp ctx values
		comp_ctx.component_entity_id = component_entity;
		comp_ctx.component_name = w_ecs_get_entity_name(world, component_entity);
		comp_ctx.component_type_id = entry->type_id;
		comp_ctx.component_type_name = W_COMPONENT_TYPE_NAME(entry->type_id);
		comp_ctx.component_entry = entry;

		w_sparse_bitset_for_each(&entry->data_bitset) {
			comp_ctx.entity = i;
			comp_ctx.entity_name = w_ecs_get_entity_name(world, i);

			// clear params buffer
			comp_ctx.hook_params_buffer[0] = '\0';

			// run serialise hooks
			w_hook_registry_run_hooks(
				&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_COMPONENT_SERIALISE],
				entry->type_id, 
				world, &comp_ctx
			);

			// write component set commands
			w_serialisation_push_ctx_command_f_(ctx, "set \"%s\" \"%s\" %s %s", comp_ctx.entity_name, comp_ctx.component_name, comp_ctx.component_type_name, comp_ctx.hook_params_buffer);
		};
	}

	// step 4: trigger post-save lifecycle hooks
	w_hook_registry_run_hooks(
		&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE],
		WM_SERIALISATION_LIFECYCLE_HOOK_POST_SAVE, 
		world, ctx
	);

	return ctx->err == 0;
}

uint32_t w_serialisation_get_version(struct wm_serialisation_registry *registry, struct wm_serialisation_ctx *ctx)
{
	// check bounds for WM_SERIALISATION_LIFECYCLE_HOOK_PRE_SAVE
	if (WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION >= registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE].hook_groups_length)
		return 0;

	uint32_t version = registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE]
		.hook_groups[WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION].hooks_length;

	return version;
}

w_entity_id *w_serialisation_get_persistent_entity_list(struct w_ecs_world *world, _Atomic size_t *entities_length, _Atomic size_t *entities_size)
{
	size_t block_size = 1024 * sizeof(w_entity_id);
	w_array_declare(w_entity_id, e);
	e_length = 0;
	w_array_init_t(e, block_size);

	w_string_table_id *name_ids = world->entities.entity_to_name;
	size_t names_max_length = world->entities.entity_to_name_length;

	// loop over entity registry entity to name table, ignoring invalid IDs
	for (size_t i = 0; i < names_max_length; ++i)
	{
		if (name_ids[i] == W_STRING_TABLE_INVALID_ID) continue;	

		// push entity
		w_array_ensure_alloc_block_size(e, e_length + 1, block_size);
		e[e_length++] = i;
	}

	*entities_length = e_length;
	*entities_size = e_size;

	return e;
}

w_entity_id *w_serialisation_get_components_list(struct w_ecs_world *world, _Atomic size_t *components_length, _Atomic size_t *components_size)
{
	size_t block_size = 1024 * sizeof(w_entity_id);
	w_array_declare(w_entity_id, c);
	c_length = 0;
	w_array_init_t(c, block_size);

	w_sparse_bitset_for_each(&world->components.entries_bitset) {
		w_array_ensure_alloc_block_size(c, c_length + 1, block_size);
		c[c_length++] = i;
	};

	*components_length = c_length;
	*components_size = c_size;

	return c;
}

void w_serialisation_push_ctx_command_(struct wm_serialisation_ctx *ctx, const char *line)
{
	size_t len = strlen(line);

	// ensure buffer capacity
	w_array_ensure_alloc_block_size(ctx->buffer, ctx->buffer_length + len + 2, WM_SERIALISATION_BUFFER_BLOCK_SIZE);

	// write line into buffer
	memcpy(&ctx->buffer[ctx->buffer_length], line, len);
	ctx->buffer_length += len;

	// new line + null terminator
	ctx->buffer[ctx->buffer_length++] = '\n';
	ctx->buffer[ctx->buffer_length] = '\0';
}

void w_serialisation_push_ctx_command_f_(struct wm_serialisation_ctx *ctx, const char *fmt, ...)
{
	char local_buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(local_buf, sizeof(local_buf), fmt, args);
	va_end(args);
	w_serialisation_push_ctx_command_(ctx, local_buf);
}



/*******************************
*  deserialisation functions  *
*******************************/

bool w_deserialisation_parse_metadata_(struct wm_deserialisation_ctx *ctx, const char *line, uint32_t *expected_entities, uint32_t *expected_components)
{
	if (line[0] != '#' || line[1] != ' ') return false;

	if (strcmp(line, "# whisker save") == 0)
		return true;
	if (strncmp(line, "# version ", 10) == 0) {
		ctx->version = (uint32_t)strtoul(line + 10, NULL, 10);
		return true;
	}
	if (strncmp(line, "# created ", 10) == 0)
		return true;
	if (strncmp(line, "# entities ", 11) == 0) {
		*expected_entities = (uint32_t)strtoul(line + 11, NULL, 10);
		return true;
	}
	if (strncmp(line, "# components ", 13) == 0) {
		*expected_components = (uint32_t)strtoul(line + 13, NULL, 10);
		return true;
	}

	return false;
}

bool w_deserialisation_parse_entity_(struct w_ecs_world *world, struct wm_deserialisation_ctx *ctx, char *line, int line_num)
{
	if (strncmp(line, "entity \"", 8) != 0) return false;

	char *name_start = line + 8;
	char *name_end = strchr(name_start, '"');
	if (!name_end) {
		ctx->err = 1;
		ctx->err_message = "malformed entity command: missing closing quote";
		ctx->err_line = line_num;
		return true;
	}

	// temporarily null-terminate the name
	*name_end = '\0';
	w_ecs_request_entity_with_name(world, name_start);
	*name_end = '"';

	ctx->entities_loaded++;
	return true;
}

bool w_deserialisation_parse_set_(struct w_ecs_world *world, struct wm_serialisation_registry *registry, struct wm_deserialisation_ctx *ctx, char *line, int line_num)
{
	if (strncmp(line, "set \"", 5) != 0) return false;

	// format: set "entity_name" "comp_name" type_name params...
	char *p = line + 5;

	// extract entity name
	char *entity_name_start = p;
	char *entity_name_end = strchr(p, '"');
	if (!entity_name_end) {
		ctx->err = 1;
		ctx->err_message = "malformed set command: missing entity name closing quote";
		ctx->err_line = line_num;
		return true;
	}

	*entity_name_end = '\0';
	p = entity_name_end + 1;

	// expect ' "' between entity and component names
	if (p[0] != ' ' || p[1] != '"') {
		*entity_name_end = '"';
		ctx->err = 1;
		ctx->err_message = "malformed set command: expected space+quote after entity name";
		ctx->err_line = line_num;
		return true;
	}
	p += 2;

	// extract component name
	char *comp_name_start = p;
	char *comp_name_end = strchr(p, '"');
	if (!comp_name_end) {
		*entity_name_end = '"';
		ctx->err = 1;
		ctx->err_message = "malformed set command: missing component name closing quote";
		ctx->err_line = line_num;
		return true;
	}

	*comp_name_end = '\0';
	p = comp_name_end + 1;

	// expect ' ' before type name
	if (*p != ' ') {
		*entity_name_end = '"';
		*comp_name_end = '"';
		ctx->err = 1;
		ctx->err_message = "malformed set command: expected space after component name";
		ctx->err_line = line_num;
		return true;
	}
	p++;

	// extract type name (space-delimited or end of line)
	char *type_start = p;
	char *type_end = strchr(p, ' ');
	char *params = "";
	char saved_type_end = '\0';

	if (type_end) {
		saved_type_end = *type_end;
		*type_end = '\0';
		params = type_end + 1;
	}

	// resolve type_id from type name
	uint32_t type_id = W_COMPONENT_TYPE_FROM_NAME(type_start);
	size_t data_size = W_COMPONENT_TYPE_SIZE(type_id);

	// restore type_end
	if (type_end) *type_end = saved_type_end;

	// look up entity and component
	w_entity_id entity_id = w_ecs_get_entity_by_name(world, entity_name_start);
	w_entity_id comp_entity_id = w_ecs_get_component_by_name(world, comp_name_start);

	// restore null terminators
	*entity_name_end = '"';
	*comp_name_end = '"';

	if (entity_id == W_ENTITY_INVALID) {
		ctx->err = 1;
		ctx->err_message = "set command references unknown entity";
		ctx->err_line = line_num;
		return true;
	}

	if (type_id == UINT32_MAX) {
		ctx->err = 1;
		ctx->err_message = "set command references unknown type";
		ctx->err_line = line_num;
		return true;
	}

	// allocate value buffer on stack and zero it
	uint8_t value_buf[256] = {0};

	// set up deserialisation component context
	struct wm_deserialisation_component_ctx dcomp_ctx = {
		.ctx = ctx,
		.entity = entity_id,
		.entity_name = NULL,
		.component_entity_id = comp_entity_id,
		.component_name = NULL,
		.component_type_id = type_id,
		.component_type_name = NULL,
		.component_entry = w_component_registry_get_entry(&world->components, comp_entity_id),
		.value_buffer = (char *)value_buf,
	};
	strncpy(dcomp_ctx.hook_params_buffer, params, sizeof(dcomp_ctx.hook_params_buffer) - 1);
	dcomp_ctx.hook_params_buffer[sizeof(dcomp_ctx.hook_params_buffer) - 1] = '\0';

	// run deserialise hooks to populate value_buf
	w_hook_registry_run_hooks(
		&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_COMPONENT_DESERIALISE],
		type_id,
		world, &dcomp_ctx
	);

	// set component data on the entity
	w_ecs_set_component_(world, type_id, comp_entity_id, entity_id, value_buf, data_size);
	ctx->components_loaded++;

	return true;
}

bool w_serialisation_restore_from_buffer(struct w_ecs_world *world, char *buffer, size_t buf_len, struct wm_deserialisation_ctx *ctx)
{
	struct wm_serialisation_registry *registry = w_ecs_singleton_get(world, WM_SERIALISATION_REGISTRY_NAME);

	if (!registry) return false;

	// init the ctx
	w_array_init_t(ctx->unparsed, WM_SERIALISATION_BUFFER_BLOCK_SIZE);
	ctx->version = 0;
	ctx->entities_loaded = 0;
	ctx->components_loaded = 0;
	ctx->err = 0;
	ctx->err_message = NULL;
	ctx->err_line = -1;

	ctx->buffer = buffer;
	ctx->buffer_length = buf_len;
	ctx->buffer_cursor = buffer;

	// deserialisation rules:
	// 1. metadata is at the top of the save
	// 2. entity commands defined next
	// 3. component set commands last

	// step 0: trigger pre-load lifecycle hooks
	w_hook_registry_run_hooks(
		&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE],
		WM_SERIALISATION_LIFECYCLE_HOOK_PRE_LOAD,
		world, ctx
	);

	uint32_t expected_entities = 0;
	uint32_t expected_components = 0;
	bool header_found = false;
	int line_num = 0;

	char *cursor = buffer;
	char *end = buffer + buf_len;

	while (cursor < end && ctx->err == 0) {
		// find end of line
		char *line_end = memchr(cursor, '\n', end - cursor);
		if (!line_end) line_end = end;

		// temporarily null-terminate
		char saved = *line_end;
		*line_end = '\0';
		line_num++;

		size_t line_len = line_end - cursor;

		// skip empty lines
		if (line_len == 0) {
			*line_end = saved;
			cursor = line_end + 1;
			continue;
		}

		// step 1: metadata
		if (w_deserialisation_parse_metadata_(ctx, cursor, &expected_entities, &expected_components)) {
			if (strcmp(cursor, "# whisker save") == 0)
				header_found = true;
		}
		// step 2: entity commands
		else if (w_deserialisation_parse_entity_(world, ctx, cursor, line_num)) {
			// handled (or error set)
		}
		// step 3: set component commands
		else if (w_deserialisation_parse_set_(world, registry, ctx, cursor, line_num)) {
			// handled (or error set)
		}
		// unrecognized line
		else {
			w_array_ensure_alloc_block_size(ctx->unparsed, ctx->unparsed_length + 1, 1024);
			ctx->unparsed[ctx->unparsed_length++] = strdup(cursor);
		}

		*line_end = saved;
		cursor = line_end + 1;
	}

	// validate header
	if (ctx->err == 0 && !header_found) {
		ctx->err = 1;
		ctx->err_message = "missing # whisker save header";
		ctx->err_line = 0;
	}

	// step 4: trigger migration hooks (version-aware, skip first N where N = saved version)
	w_hook_registry_run_hooks_from_index(
		&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE],
		WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION,
		ctx->version, world, ctx
	);

	// step 5: trigger post-load lifecycle hooks (always run all, no version skipping)
	w_hook_registry_run_hooks(
		&registry->hooks[WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE],
		WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD,
		world, ctx
	);

	return ctx->err == 0;
}

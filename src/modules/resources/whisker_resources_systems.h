/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_resources_systems
 * @created     : Sunday May 17, 2026 16:15:36 CST
 * @description : 
 */

#include "whisker_resources.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"

#ifndef WHISKER_RESOURCES_SYSTEMS_H
#define WHISKER_RESOURCES_SYSTEMS_H

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
    #define stat _stat
#endif

#define _set_file_invalid_and_continue(entity, err) \
	w_log_entity_error(resources, "validation failed: %s (error %s)", file_path, #err); \
	w_set_value(entity, resource_file_invalid, err); continue;

// handle resource file path validation
w_ecs_system(
	wm_resources_lifecycle_ensure_resources_valid,
	WM_PHASE_PRE_LOAD,
	w_query(
		// we need the resource sting path
		w_query_r(resource_file_path_string_id),

		// skip if the resource is already valid/invalid
		w_query_n(resource_file_valid),
		w_query_n(resource_file_invalid),
	),
{
	// first check file path string resolves and is valid
	const char *file_path = w_string_from_id(*w_query_get(resource_file_path_string_id));

	if (!file_path || strlen(file_path) == 0)
	{
		_set_file_invalid_and_continue(entity, WM_RESOURCES_FILE_INVALID_FILE_PATH);
	}

	// attempt to read the file
	struct stat st;

    // stat() returns 0 on success, -1 on error (sets errno)
    if (stat(file_path, &st) != 0)
    {
        switch (errno)
        {
            case ENOENT: 
            	_set_file_invalid_and_continue(entity,WM_RESOURCES_FILE_INVALID_NOT_FOUND);
            case EACCES:
            	_set_file_invalid_and_continue(entity, WM_RESOURCES_FILE_INVALID_NOT_READABLE);
            default:
            	_set_file_invalid_and_continue(entity, WM_RESOURCES_FILE_INVALID_UNKNOWN);
        }
    }

    // check it's a regular file (not directory)
    if (!S_ISREG(st.st_mode)) {
        _set_file_invalid_and_continue(entity, WM_RESOURCES_FILE_INVALID_NOT_READABLE);
    }

    // check size
    if (st.st_size == 0) {
        _set_file_invalid_and_continue(entity, WM_RESOURCES_FILE_INVALID_NO_SIZE);
    }

    size_t file_size = (size_t)st.st_size;
	
	// record file size after successful stat and mark valid
	w_set_value(entity, resource_file_size_bytes, file_size);
	w_set_tag(entity, resource_file_valid, true);

	w_log_entity_debug(resources, "validation passed: %s (size %zu)", file_path, file_size);
});


// resource hot request handler
w_ecs_system(
	wm_resources_lifecycle_handle_hot_request,
	WM_PHASE_ON_LOAD,
	w_query(
		// file path so we can load the data
		w_query_r(resource_file_path_string_id),
		// size of file, so we load exactly this much
		w_query_r(resource_file_size_bytes),

		// only load valid files
		w_query_h(resource_file_valid),

		// handler for hot requests
		w_query_h(req_resource_hot),

		// optional to exclude already hot
		w_query_o(resource_hot),
	),
{
	w_set_tag(entity, req_resource_hot, false);

	// skip already hot resources
	if (w_query_get_opt(resource_hot))
	{
		continue;
	}

	// allocate space for file data and load
	const char *file_path = w_string_from_id(*w_query_get(resource_file_path_string_id));
	size_t file_size = *w_query_get(resource_file_size_bytes);

	// note: we do +1 so that we can get native string resource support
	// without extra effort
	void *ptr = wm_managed_alloc_malloc(world, resource_hot_data_handle, entity, file_size + 1);
	((uint8_t *)(ptr))[file_size] = '\0';

	bool success = true;

	// open file in binary read mode
	FILE *f = fopen(file_path, "rb"); 
	if (!f) {
		// failed to open file
		w_log_entity_fatal(resources, "failed to open file: %s: %s", file_path, strerror(errno));
		success = false;
	}
	// read file contents into ptr
	size_t read_bytes = fread(ptr, 1, file_size, f); 
	if (read_bytes != file_size) {
		// read error or unexpected file size
		w_log_entity_fatal(resources, "failed to read entire file: %s: %s", file_path, ferror(f) ? strerror(errno) : "unexpected file size");
		success = false;
		fclose(f);
	}
	// close file
	if (fclose(f) != 0) {
		// failed to close file
		w_log_entity_fatal(resources, "failed to close file: %s: %s", file_path, strerror(errno));
		success = false;
	}

	// if no errors set handle data size and set hot
	if (success)
	{
		w_log_entity_info(resources, "hot and ready: %s", file_path);

		w_set_value(entity, resource_file_size_bytes, file_size);
		w_set_value(entity, resource_hot_data_size, file_size);
		w_set_tag(entity, resource_hot, true);
	}
});

// resource cold request handler
w_ecs_system(
	wm_resources_lifecycle_handle_cold_request,
	WM_PHASE_ON_LOAD,
	w_query(
		// handler tag for cold resource requests
		w_query_h(req_resource_cold),

		// only hot resources
		w_query_h(resource_hot),

		// the data handle so we can free it
		w_query_r(resource_hot_data_handle),
	),
{	
	w_set_tag(entity, req_resource_cold, false);

	// skip already cold resources
	if (!w_query_get_opt(resource_hot))
	{
		continue;
	}
	
	// free the handle
	wm_managed_alloc_free_handle(world, resource_hot_data_handle, entity);

	// remove hot tag
	w_set_tag(entity, resource_hot, false);

	w_log_entity_debug(resources, "unloaded hot resource");

});

#endif /* WHISKER_RESOURCES_SYSTEMS_H */


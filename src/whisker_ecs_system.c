/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:56 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_block_array.h"

#include "whisker_ecs_system.h"

void whisker_ecs_s_free_components(whisker_block_array *system_components)
{
	// TODO: find a better way to handle this since looping over every single
	// block isn't great
	for (int i = 0; i < warr_length(system_components->blocks); ++i)
	{
		if (system_components->blocks[i] != NULL)
		{
			for (int ii = 0; ii < system_components->block_size; ++ii)
			{
				if (((whisker_ecs_system*)system_components->blocks[i])[ii].system_ptr != NULL)
				{
					warr_free(((whisker_ecs_system*)system_components->blocks[i])[ii].write_archetype);
				}
			}
		}
	}
}


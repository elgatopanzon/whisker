/**
 * @author      : {{cookiecutter.author}}
 * @file        : whisker_{{cookiecutter.module_name}}
 * @created     : {{cookiecutter.created}}
 * @description : {{cookiecutter.description}}
 */

#ifndef WHISKER_{{cookiecutter.module_name | upper}}_H
#define WHISKER_{{cookiecutter.module_name | upper}}_H

#include "whisker.h"

// initialize the {{cookiecutter.module_name}} module
void {{cookiecutter.module_prefix}}_init(struct w_ecs_world *world);

// cleanup the {{cookiecutter.module_name}} module
void {{cookiecutter.module_prefix}}_free(struct w_ecs_world *world);

#endif /* WHISKER_{{cookiecutter.module_name | upper}}_H */

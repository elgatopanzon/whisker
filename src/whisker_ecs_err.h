/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_err
 * @created     : Thursday Feb 13, 2025 18:56:50 CST
 */

#include "whisker_std.h"

#ifndef WHISKER_ECS_ERR_H
#define WHISKER_ECS_ERR_H

// errors
typedef enum E_WHISKER_ECS 
{
	E_WHISKER_ECS_OK = 0,
	E_WHISKER_ECS_UNKNOWN = 1,
	E_WHISKER_ECS_MEM = 2,
	E_WHISKER_ECS_ARR = 3,
	E_WHISKER_ECS_DICT = 4,
	E_WHISKER_ECS_UPDATE_SYSTEM = 5,
} E_WHISKER_ECS;
extern const char *E_WHISKER_ECS_STR[];

typedef enum E_WHISKER_ECS_SYS
{
	E_WHISKER_ECS_SYS_OK = 0,
	E_WHISKER_ECS_SYS_UNKNOWN = 1,
	E_WHISKER_ECS_SYS_MEM = 2,
	E_WHISKER_ECS_SYS_ARR = 3,
	E_WHISKER_ECS_SYS_DICT = 4,
} E_WHISKER_ECS_SYS;
extern const char *E_WHISKER_ECS_SYS_STR[];

#endif /* WHISKER_ECS_ERR_H */

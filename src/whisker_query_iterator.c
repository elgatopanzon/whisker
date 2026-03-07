/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_query_iterator
 * @created     : Friday Mar 06, 2026 19:23:53 CST
 */

#include "whisker_std.h"

#include "whisker_query_iterator.h"

void w_query_iterator_begin(struct w_query_iterator *itor, struct w_query *query)
{
	itor->get_cursor = 0;
	itor->query = query;
}

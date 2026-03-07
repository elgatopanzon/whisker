/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_query_iterator
 * @created     : Friday Mar 06, 2026 18:06:14 CST
 * @description : iterate over the slices of a query struct
 */

#include "whisker_std.h"
#include "whisker_query_registry.h"
#include "whisker_ecs_world.h"

#ifndef WHISKER_QUERY_ITERATOR_H
#define WHISKER_QUERY_ITERATOR_H

#define w_query_for_each_archetype_slice_loop_(block, stype, length) \
	for (size_t i = 0; i < length; ++i) \
	{ \
		struct w_query_archetype_slice slice = itor.query->archetype_slices_##stype[i]; \
		for (size_t s = 0; s < slice.slice_length; ++s) \
		{ \
			itor.entity_id = slice.start_id + s; \
			itor.get_cursor = 0; \
			block; \
		} \
	} \

#define w_query_for_each(w, q, block) {\
	static struct w_query *_q_ = NULL; \
	struct w_query_iterator itor; \
	if (!_q_) _q_ = w_query_registry_get_query(&(w)->queries, q); \
	w_query_iterator_begin(&itor, _q_); \
	size_t dense_length = itor.query->archetype_slices_dense_length; \
	size_t sparse_length = itor.query->archetype_slices_sparse_length; \
	w_query_for_each_archetype_slice_loop_(block, dense, dense_length); \
	w_query_for_each_archetype_slice_loop_(block, sparse, sparse_length); \
}; \

#define w_itor_get_optional_impl_(T) ( \
    { \
        while (itor.get_cursor < itor.query->terms_length && itor.query->terms[itor.get_cursor].access_type == W_QUERY_ACCESS_NONE) { \
            itor.get_cursor++; \
        } \
        struct w_query_term *term = &itor.query->terms[itor.get_cursor]; \
        struct w_sparse_bitset *bitset = itor.query->bitset_cache.bitsets[itor.get_cursor]; \
        void *result = (term->access_type == W_QUERY_ACCESS_OPTIONAL && (!bitset || !w_sparse_bitset_get(bitset, itor.entity_id))) \
            ? NULL \
            : w_component_get_entry(term->component_entry, itor.entity_id, T); \
        itor.get_cursor++; \
        result; \
    } \
)

#define w_itor_get_optional(T) \
	(T *)w_itor_get_optional_impl_(T)

#define w_itor_get(T) \
	({ \
		struct w_component_entry *_ent_ = itor.query->terms[itor.get_cursor++].component_entry; \
		(T *)((_ent_)->data + (itor.entity_id * (_ent_)->type_size)); \
	})

struct w_query_iterator 
{
	struct w_query *query;
	size_t get_cursor;
	w_entity_id entity_id;
};

// init a fresh iterator for the provided query
void w_query_iterator_begin(struct w_query_iterator *itor, struct w_query *query);

/* void test_system(struct w_ecs_world *world, double delta_time) */
/* { */
/* 	w_query_for_each(world, "has comp1, read comp2, write comp3, optional comp4", { */
/* 		int *comp2 = w_itor_get(int);  */
/* 		float *comp3 = w_itor_get(float);  */
/* 		double *comp4 = w_itor_get_optional(double);  */
/*  */
/* 		if (comp4) */
/* 		{ */
/* 			*comp4 = (*comp2 * *comp3) * delta_time; */
/* 		} */
/* 	}); */
/* } */


#endif /* WHISKER_QUERY_ITERATOR_H */


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
			w_entity_id entity = itor.entity_id; \
			(void)entity; \
			block; \
		} \
	} \

#define w_query_for_each(w, q, block) {\
	static struct w_query *_q_ = NULL; \
	struct w_query_iterator itor; \
	if (!_q_) _q_ = w_query_registry_get_query(&(w)->queries, q); \
	w_query_rebuild_cache(&(w)->queries, _q_); \
	w_query_iterator_begin(&itor, _q_); \
	size_t dense_length = itor.query->archetype_slices_dense_length; \
	size_t sparse_length = itor.query->archetype_slices_sparse_length; \
	w_query_for_each_archetype_slice_loop_(block, dense, dense_length); \
	w_query_for_each_archetype_slice_loop_(block, sparse, sparse_length); \
}; \

#define w_itor_get_optional_impl_(T) ( \
    { \
        while (itor.get_cursor < itor.query->terms_length && \
               (itor.query->terms[itor.get_cursor].access_type == W_QUERY_ACCESS_NONE || \
                itor.query->terms[itor.get_cursor].access_type == W_QUERY_ACCESS_NOT)) { \
            itor.get_cursor++; \
        } \
        struct w_query_term *term = &itor.query->terms[itor.get_cursor]; \
        void *result = (term->access_type == W_QUERY_ACCESS_OPTIONAL && (!term->component_entry || !w_sparse_bitset_get(&term->component_entry->data_bitset, itor.entity_id))) \
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

#define w_itor_get_read_ptr(T)  ((const T *)w_itor_get(T))
#define w_itor_get_read(T)  *w_itor_get_read_ptr(T)
#define w_itor_get_write(T) w_itor_get(T)

/*******************************************
*  old: legacy string-based query macros  *
*******************************************/
#define w_query_read(name) "read " name ", "
#define w_query_write(name) "write " name ", "
#define w_query_optional(name) "optional " name ", "
#define w_query_not(name) "not " name ", "


/*********************************
*  new: type-safe query macros  *
*********************************/
#define w_query_part_(access, comp) \
	((void)sizeof(comp), #access " " #comp)

#define w_query_r(comp) \
	w_query_part_(read, comp)
#define w_query_w(comp) \
	w_query_part_(write, comp)
#define w_query_o(comp) \
	w_query_part_(optional, comp)
#define w_query_n(comp) \
	w_query_part_(not, comp)
#define w_query_h(comp) \
	w_query_part_(read, comp) // has doesnt exist yet
							  //
#define w_query_part_generic_(access, comp, gname) \
	((void)sizeof(comp), #access " " #comp "_" #gname)

#define w_query_r_g(comp, gname) \
	w_query_part_generic_(read, comp, gname)
#define w_query_w_g(comp, gname) \
	w_query_part_generic_(write, comp, gname)
#define w_query_o_g(comp, gname) \
	w_query_part_generic_(optional, comp, gname)
#define w_query_n_g(comp, gname) \
	w_query_part_generic_(not, comp, gname)
#define w_query_h_g(comp, gname) \
	w_query_part_generic_(read, comp, gname) // has doesnt exist yet

#define w_query(...) \
    ({ \
        static char _cached[1024] = {0}; \
        static bool _init = false; \
        if (!_init) { \
            const char* _parts[] = {__VA_ARGS__}; \
            size_t _count = sizeof(_parts) / sizeof(_parts[0]); \
            _cached[0] = 0; \
            for (size_t _i = 0; _i < _count; ++_i) { \
                if (_i > 0) strcat(_cached, ", "); \
                strcat(_cached, _parts[_i]); \
            } \
            _init = true; \
        } \
        (char*)_cached; \
    })

#define w_query_get(T) \
	({ \
		W_ECS_SET_COMP_ID_CACHE(T); \
		struct w_component_entry *_ent_ = itor.query->terms[itor.query->component_id_to_terms[T##_component_id_]].component_entry; \
		(T *)((_ent_)->data + (itor.entity_id * (_ent_)->type_size)); \
	})

#define w_query_get_opt(T) \
	({ \
		W_ECS_SET_COMP_ID_CACHE(T); \
		w_entity_id _qgo_comp_id_ = T##_component_id_; \
		T *_qgo_result_ = NULL; \
		if (_qgo_comp_id_ != W_ENTITY_INVALID && \
			_qgo_comp_id_ < itor.query->component_id_to_terms_size / sizeof(itor.query->component_id_to_terms[0])) { \
			w_entity_id _qgo_term_idx_ = itor.query->component_id_to_terms[_qgo_comp_id_]; \
			if (_qgo_term_idx_ < itor.query->terms_length) { \
				struct w_query_term *_qgo_term_ = &itor.query->terms[_qgo_term_idx_]; \
				if (_qgo_term_->component_entry != NULL && _qgo_term_->component_id == _qgo_comp_id_) { \
					if (w_sparse_bitset_get(&_qgo_term_->component_entry->data_bitset, itor.entity_id)) { \
						_qgo_result_ = (T *)((_qgo_term_->component_entry)->data + (itor.entity_id * (_qgo_term_->component_entry)->type_size)); \
					} \
				} \
			} \
		} \
		_qgo_result_; \
	})

#define w_query_get_opt_or_default(T) \
	({ \
	T *opt = w_query_get_opt(T); \
	if (!opt) { \
		opt = w_ecs_frame_malloc(world, sizeof(T)); \
		*opt = T##_get_default(); \
	} \
	opt; \
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


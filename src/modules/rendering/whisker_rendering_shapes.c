/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_shapes
 * @created     : Thursday May 14, 2026 16:07:17 CST
 */

#include "whisker_rendering_shapes.h"

void w_rendering_shape_apply_vert_offset(w_vec3 *verts, size_t from, size_t to, w_vec3 offset)
{
	for (size_t i = from; i < to; ++i)
	{
		verts[i] = w_vec3_add(verts[i], offset);
	}
}

void w_rendering_shape_apply_vert_scale(w_vec3 *verts, size_t from, size_t to, w_vec3 scale)
{
	for (size_t i = from; i < to; ++i)
	{
		verts[i] = w_vec3_mul(verts[i], scale);
	}
}

// returns false if degenerate (caller should bail)
bool w_rendering_shape_sanitize_circle_params(float *diameter, float *start_rad, float *end_rad, int *segments)
{
    // sanitize diameter
    if (*diameter <= 0) *diameter = 1;
    // sanitize start/end angle
    if (*end_rad < *start_rad)
    {
        float swap = *start_rad;
        *start_rad = *end_rad;
        *end_rad = swap;
    }

    // degenerate check
    if (*end_rad - *start_rad < 0.001f) return false;
    // calculate minimum segments
    int segments_min = (int)ceilf((*end_rad - *start_rad) / (W_PI / 2));
    if (*segments < segments_min)
    {
        float th = acosf(2 * powf(1 - 1.0f / *diameter, 2) - 1);
        *segments = (int)ceilf((*end_rad - *start_rad) / th);
        if (*segments <= 0) *segments = segments_min;
    }
    return true;
}

int w_rendering_shape_generate_circle_verts(w_vec3 *out_verts, int offset, int segments, float start_rad, float end_rad, w_vec3 vert_offset, bool face_up)
{
	// calculate steps
    float step_length = (end_rad - start_rad)/(float)segments;

	for (int i = 0; i < segments; i++)
    {
        float angle0 = start_rad + i * step_length;
    	float angle1 = start_rad + (i + 1) * step_length;

		// flip winding direction to CW if not face up
		if (!face_up)
		{
			float angletmp = angle0;
			angle0 = angle1;
			angle1 = angletmp;
		}

        // center vertex
        out_verts[offset + i * 3 + 0].x = vert_offset.x;
        out_verts[offset + i * 3 + 0].y = vert_offset.y;
        out_verts[offset + i * 3 + 0].z = vert_offset.z;

        // first edge vertex (CCW)
        out_verts[offset + i * 3 + 1].x = vert_offset.x + cosf(angle1);
        out_verts[offset + i * 3 + 1].y = vert_offset.y + 0.0f;
        out_verts[offset + i * 3 + 1].z = vert_offset.z + sinf(angle1);

        // second edge vertex (CCW)
        out_verts[offset + i * 3 + 2].x = vert_offset.x + cosf(angle0);
        out_verts[offset + i * 3 + 2].y = vert_offset.y + 0.0f;
        out_verts[offset + i * 3 + 2].z = vert_offset.z + sinf(angle0);
    }

	return W_CIRCLE_VERTS_COUNT(segments);
}

int w_rendering_shape_generate_circle_outline_verts(w_vec3 *out_verts, int offset, int segments, float start_rad, float end_rad, w_vec3 vert_offset)
{
	// calculate steps
    float step_length = (end_rad - start_rad)/(float)segments;

    // if its not a full circle we need to generate connecting lines
    bool is_full_circle = (end_rad - start_rad) >= (2.0f * W_PI - 0.001f);

	for (int i = 0; i < segments; i++)  
	{  
    	float angle0 = start_rad + i * step_length;  
    	float angle1 = start_rad + (i + 1) * step_length;

    	// first point of the line (CCW)  
    	out_verts[offset + i * 2 + 0].x = vert_offset.x + cosf(angle1);  
    	out_verts[offset + i * 2 + 0].y = vert_offset.y + 0.0f;  
    	out_verts[offset + i * 2 + 0].z = vert_offset.z + sinf(angle1);  

    	// second point of the line (CCW)  
    	out_verts[offset + i * 2 + 1].x = vert_offset.x + cosf(angle0);  
    	out_verts[offset + i * 2 + 1].y = vert_offset.y + 0.0f;  
    	out_verts[offset + i * 2 + 1].z = vert_offset.z + sinf(angle0);  
	}
    
    // add radial lines for partial circles (pie slice edges)
    if (!is_full_circle)
    {
        int base = segments * 2;
        
        // center to start
        out_verts[offset + base + 0] = vert_offset;
        out_verts[offset + base + 1] = ((w_vec3){vert_offset.x + cosf(start_rad), vert_offset.y + 0, vert_offset.z + sinf(start_rad)});
        
        // center to end
        out_verts[offset + base + 2] = vert_offset;
        out_verts[offset + base + 3] = ((w_vec3){vert_offset.x + cosf(end_rad), vert_offset.y + 0, vert_offset.z + sinf(end_rad)});
    }

	return W_CIRCLE_OUTLINE_VERTS_COUNT(segments, is_full_circle);
}

int w_rendering_shape_generate_cylinder_sides_verts(w_vec3 *out_verts, int start_index, int segments, float diameter_top, float diameter_bottom, float half_height)
{
    float step = (2.0f * W_PI) / (float)segments;

    for (int i = 0; i < segments; i++)
    {
        float angle0 = i * step;
        float angle1 = (i + 1) * step;

        // four corners of the quad
        w_vec3 top_left = {
            cosf(angle0) * diameter_top,
            half_height,
            sinf(angle0) * diameter_top
        };
        w_vec3 top_right = {
            cosf(angle1) * diameter_top,
            half_height,
            sinf(angle1) * diameter_top
        };
        w_vec3 bot_left = {
            cosf(angle0) * diameter_bottom,
            -half_height,
            sinf(angle0) * diameter_bottom
        };
        w_vec3 bot_right = {
            cosf(angle1) * diameter_bottom,
            -half_height,
            sinf(angle1) * diameter_bottom
        };

        int base = start_index + i * 6;

        // triangle 1 (CCW from outside)
		out_verts[base + 0] = top_left;
		out_verts[base + 1] = bot_right;
		out_verts[base + 2] = bot_left;
		// triangle 2 (CCW from outside)
		out_verts[base + 3] = top_left;
		out_verts[base + 4] = top_right;
		out_verts[base + 5] = bot_right;
    }

    return segments * 6;
}

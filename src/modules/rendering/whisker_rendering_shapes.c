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

int w_rendering_shape_generate_capsule_verts(w_vec3 *out_verts, int start_index, int segments, int rings, float diameter, float half_length)
{
    float sphere_radius = diameter;
    float az_step = (2.0f * W_PI) / (float)segments;
    int idx = start_index;

    // Top hemisphere: theta from 0 (pole) to PI/2 (equator)
    // all Y offset by +half_length
    w_vec3 top_pole = {0.0f, sphere_radius + half_length, 0.0f};
    float theta0 = (W_PI * 0.5f) / (float)rings;
    float r0 = sinf(theta0) * sphere_radius;
    float y0 = cosf(theta0) * sphere_radius + half_length;

    for (int i = 0; i < segments; i++)
    {
        float az0 = i * az_step;
        float az1 = (i + 1) * az_step;

        // CCW from outside: pole, ring[az1], ring[az0]
        out_verts[idx + i * 3 + 0] = top_pole;
        out_verts[idx + i * 3 + 1] = (w_vec3){cosf(az1) * r0, y0, sinf(az1) * r0};
        out_verts[idx + i * 3 + 2] = (w_vec3){cosf(az0) * r0, y0, sinf(az0) * r0};
    }
    idx += segments * 3;

    // top hemisphere bands
    for (int ri = 0; ri < rings - 1; ri++)
    {
        float theta_top = (ri + 1) * (W_PI * 0.5f) / (float)rings;
        float theta_bot = (ri + 2) * (W_PI * 0.5f) / (float)rings;

        float r_top = sinf(theta_top) * sphere_radius;
        float y_top = cosf(theta_top) * sphere_radius + half_length;
        float r_bot = sinf(theta_bot) * sphere_radius;
        float y_bot = cosf(theta_bot) * sphere_radius + half_length;

        for (int i = 0; i < segments; i++)
        {
            float az0 = i * az_step;
            float az1 = (i + 1) * az_step;

            w_vec3 tl = {cosf(az0) * r_top, y_top, sinf(az0) * r_top};
            w_vec3 tr = {cosf(az1) * r_top, y_top, sinf(az1) * r_top};
            w_vec3 bl = {cosf(az0) * r_bot, y_bot, sinf(az0) * r_bot};
            w_vec3 br = {cosf(az1) * r_bot, y_bot, sinf(az1) * r_bot};

            int base = idx + i * 6;

            // tri 1 (CCW from outside)
            out_verts[base + 0] = tl;
            out_verts[base + 1] = br;
            out_verts[base + 2] = bl;
            // tri 2 (CCW from outside)
            out_verts[base + 3] = tl;
            out_verts[base + 4] = tr;
            out_verts[base + 5] = br;
        }
        idx += segments * 6;
    }

    // Bottom hemisphere: theta from PI/2 (equator) to PI (pole)
    // all Y offset by -half_length, winding matches sphere bottom half
    for (int ri = 0; ri < rings - 1; ri++)
    {
        float theta_top = (W_PI * 0.5f) + ri * (W_PI * 0.5f) / (float)rings;
        float theta_bot = (W_PI * 0.5f) + (ri + 1) * (W_PI * 0.5f) / (float)rings;

        float r_top = sinf(theta_top) * sphere_radius;
        float y_top = cosf(theta_top) * sphere_radius - half_length;
        float r_bot = sinf(theta_bot) * sphere_radius;
        float y_bot = cosf(theta_bot) * sphere_radius - half_length;

        for (int i = 0; i < segments; i++)
        {
            float az0 = i * az_step;
            float az1 = (i + 1) * az_step;

            w_vec3 tl = {cosf(az0) * r_top, y_top, sinf(az0) * r_top};
            w_vec3 tr = {cosf(az1) * r_top, y_top, sinf(az1) * r_top};
            w_vec3 bl = {cosf(az0) * r_bot, y_bot, sinf(az0) * r_bot};
            w_vec3 br = {cosf(az1) * r_bot, y_bot, sinf(az1) * r_bot};

            int base = idx + i * 6;

            // tri 1 (CCW from outside)
            out_verts[base + 0] = tl;
            out_verts[base + 1] = br;
            out_verts[base + 2] = bl;
            // tri 2 (CCW from outside)
            out_verts[base + 3] = tl;
            out_verts[base + 4] = tr;
            out_verts[base + 5] = br;
        }
        idx += segments * 6;
    }

    // bottom pole fan
    w_vec3 bot_pole = {0.0f, -sphere_radius - half_length, 0.0f};
    float theta_last = (W_PI * 0.5f) + (rings - 1) * (W_PI * 0.5f) / (float)rings;
    float r_last = sinf(theta_last) * sphere_radius;
    float y_last = cosf(theta_last) * sphere_radius - half_length;

    for (int i = 0; i < segments; i++)
    {
        float az0 = i * az_step;
        float az1 = (i + 1) * az_step;

        // CCW from outside: pole, ring[az0], ring[az1]
        out_verts[idx + i * 3 + 0] = bot_pole;
        out_verts[idx + i * 3 + 1] = (w_vec3){cosf(az0) * r_last, y_last, sinf(az0) * r_last};
        out_verts[idx + i * 3 + 2] = (w_vec3){cosf(az1) * r_last, y_last, sinf(az1) * r_last};
    }
    idx += segments * 3;

    // cylinder sides connecting hemisphere equators
    idx += w_rendering_shape_generate_cylinder_sides_verts(
        out_verts, idx, segments,
        sphere_radius, sphere_radius, half_length);

    return idx - start_index;
}

int w_rendering_shape_generate_sphere_verts(w_vec3 *out_verts, int start_index, int segments, int rings, float diameter, float angle_start, float angle_end)
{
    float sphere_radius = diameter;
    float az_step = (angle_end - angle_start) / (float)segments;
    int idx = start_index;

    // top pole fan: pole connects to ring 0
    w_vec3 top_pole = {0.0f, sphere_radius, 0.0f};
    float theta0 = W_PI / (rings + 1);
    float r0 = sinf(theta0) * sphere_radius;
    float y0 = cosf(theta0) * sphere_radius;

    for (int i = 0; i < segments; i++)
    {
        float az0 = angle_start + i * az_step;
        float az1 = angle_start + (i + 1) * az_step;

        // CCW from outside: pole, ring[az1], ring[az0]
        out_verts[idx + i * 3 + 0] = top_pole;
        out_verts[idx + i * 3 + 1] = (w_vec3){cosf(az1) * r0, y0, sinf(az1) * r0};
        out_verts[idx + i * 3 + 2] = (w_vec3){cosf(az0) * r0, y0, sinf(az0) * r0};
    }
    idx += segments * 3;

    // bands between adjacent rings
    for (int ri = 0; ri < rings - 1; ri++)
    {
        float theta_top = (ri + 1) * W_PI / (rings + 1);
        float theta_bot = (ri + 2) * W_PI / (rings + 1);

        float r_top = sinf(theta_top) * sphere_radius;
        float y_top = cosf(theta_top) * sphere_radius;
        float r_bot = sinf(theta_bot) * sphere_radius;
        float y_bot = cosf(theta_bot) * sphere_radius;

        for (int i = 0; i < segments; i++)
        {
            float az0 = angle_start + i * az_step;
            float az1 = angle_start + (i + 1) * az_step;

            w_vec3 top_left  = {cosf(az0) * r_top, y_top, sinf(az0) * r_top};
            w_vec3 top_right = {cosf(az1) * r_top, y_top, sinf(az1) * r_top};
            w_vec3 bot_left  = {cosf(az0) * r_bot, y_bot, sinf(az0) * r_bot};
            w_vec3 bot_right = {cosf(az1) * r_bot, y_bot, sinf(az1) * r_bot};

            int base = idx + i * 6;

            // tri 1 (CCW from outside)
            out_verts[base + 0] = top_left;
            out_verts[base + 1] = bot_right;
            out_verts[base + 2] = bot_left;
            // tri 2 (CCW from outside)
            out_verts[base + 3] = top_left;
            out_verts[base + 4] = top_right;
            out_verts[base + 5] = bot_right;
        }
        idx += segments * 6;
    }

    // bottom pole fan: last ring connects to pole
    w_vec3 bot_pole = {0.0f, -sphere_radius, 0.0f};
    float theta_last = rings * W_PI / (rings + 1);
    float r_last = sinf(theta_last) * sphere_radius;
    float y_last = cosf(theta_last) * sphere_radius;

    for (int i = 0; i < segments; i++)
    {
        float az0 = angle_start + i * az_step;
        float az1 = angle_start + (i + 1) * az_step;

        // CCW from outside: pole, ring[az0], ring[az1]
        out_verts[idx + i * 3 + 0] = bot_pole;
        out_verts[idx + i * 3 + 1] = (w_vec3){cosf(az0) * r_last, y_last, sinf(az0) * r_last};
        out_verts[idx + i * 3 + 2] = (w_vec3){cosf(az1) * r_last, y_last, sinf(az1) * r_last};
    }
    idx += segments * 3;

    // wedge sides for partial spheres
    bool is_partial = (angle_end - angle_start)
        < (2.0f * W_PI - 0.001f);
    if (is_partial)
    {
        w_vec3 center = {0.0f, 0.0f, 0.0f};

        for (int side = 0; side < 2; side++)
        {
            float az = (side == 0)
                ? angle_start : angle_end;

            for (int i = 0; i <= rings; i++)
            {
                w_vec3 p0, p1;

                if (i == 0)
                {
                    p0 = top_pole;
                }
                else
                {
                    float th = i * W_PI / (rings + 1);
                    float r = sinf(th) * sphere_radius;
                    float y = cosf(th) * sphere_radius;
                    p0 = (w_vec3){
                        cosf(az) * r, y, sinf(az) * r
                    };
                }

                if (i == rings)
                {
                    p1 = bot_pole;
                }
                else
                {
                    float th = (i + 1) * W_PI
                        / (rings + 1);
                    float r = sinf(th) * sphere_radius;
                    float y = cosf(th) * sphere_radius;
                    p1 = (w_vec3){
                        cosf(az) * r, y, sinf(az) * r
                    };
                }

                out_verts[idx++] = center;
                if (side == 0)
                {
                    // start side: CCW from outside
                    out_verts[idx++] = p0;
                    out_verts[idx++] = p1;
                }
                else
                {
                    // end side: reversed winding
                    out_verts[idx++] = p1;
                    out_verts[idx++] = p0;
                }
            }
        }
    }

    return idx - start_index;
}

void w_rendering_shape_generate_hexahedron_verts(w_vec3 *out_verts, const w_tricell8 *cell)
{
    const w_vec3 *corners = (const w_vec3 *)cell;

    for (size_t i = 0; i < W_SHAPE_CUBE_TRIS_LEN; i++)
    {
        // map -1/+1 to corner index: x + y*2 + z*4
        int idx = (w_shape_cube_tris[i].x > 0.0f ? 1 : 0)
                + (w_shape_cube_tris[i].y > 0.0f ? 2 : 0)
                + (w_shape_cube_tris[i].z > 0.0f ? 4 : 0);

        out_verts[i] = corners[idx];
    }
}

void w_rendering_shape_generate_hexahedron_outline_verts(w_vec3 *out_verts, const w_tricell8 *cell)
{
    const w_vec3 *corners = (const w_vec3 *)cell;

    for (size_t i = 0; i < W_SHAPE_CUBE_LINES_LEN; i++)
    {
        int idx = (w_shape_cube_lines[i].x > 0.0f ? 1 : 0)
                + (w_shape_cube_lines[i].y > 0.0f ? 2 : 0)
                + (w_shape_cube_lines[i].z > 0.0f ? 4 : 0);

        out_verts[i] = corners[idx];
    }
}

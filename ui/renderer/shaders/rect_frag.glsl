#version 330 core

flat in vec4 color;
flat in vec2 center;
flat in vec2 size;
flat in float corner_radius;

out vec4 frag_color;

uniform vec2 resolution;

float roundedBoxSDF(vec2 pos, vec2 cen, vec2 cor, float radius) {
    vec2 p = pos - cen;
    vec2 q = abs(p) - cor + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
    // return length(max(abs(pos), cor) - cor) - radius;
}

void main() {
    vec2 pixel_pos = gl_FragCoord.xy;

    vec2 bottom_left = center - size / 2;
    vec2 bottom_right = center + vec2(size.x / 2, -size.y / 2);
    vec2 top_left = center + vec2(-size.x / 2, size.y / 2);
    vec2 top_right = center + size / 2;

    bottom_left += corner_radius;
    bottom_right += vec2(-corner_radius, corner_radius);
    top_left += vec2(corner_radius, -corner_radius);
    top_right -= corner_radius;

    // if (pixel_pos.x < bottom_left.x && pixel_pos.y < bottom_left.y) {
    //     if (distance(pixel_pos, bottom_left) > corner_radius) {
    //         discard;
    //     }
    // }
    // if (pixel_pos.x > bottom_right.x && pixel_pos.y < bottom_right.y) {
    //     if (distance(pixel_pos, bottom_right) > corner_radius) {
    //         discard;
    //     }
    // }
    // if (pixel_pos.x < top_left.x && pixel_pos.y > top_left.y) {
    //     if (distance(pixel_pos, top_left) > corner_radius) {
    //         discard;
    //     }
    // }
    // if (pixel_pos.x > top_right.x && pixel_pos.y > top_right.y) {
    //     if (distance(pixel_pos, top_right) > corner_radius) {
    //         discard;
    //     }
    // }

    // float distance = roundedBoxSDF(top_left, center, size, corner_radius);
    // if (distance < -400) {
    //     discard;
    // }

    pixel_pos /= resolution;
    vec2 pos = vec2(0.5, 0.5);
    vec2 size = vec2(0.16, 0.02);
    float d = length(max(abs(pixel_pos - pos), size) - size) - 0.05;
    if (d < 0.2) {
        discard;
    }

    frag_color = color;
}

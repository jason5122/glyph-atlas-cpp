#version 330 core

layout(location = 0) in vec2 pixelPos;

uniform float width;
uniform float height;

vec2 pixelToClipSpace(vec2 point, vec2 resolution) {
    vec2 vertexCoords = pixelPos / resolution;  // Normalize to [0.0, 1.0].
    // vertexCoords.y = 1.0 - vertexCoords.y;  // Set origin to top left instead of bottom left.
    vec2 vertexPosition = (vertexCoords * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
    return vertexPosition;
}

void main() {
    gl_Position.xy = pixelToClipSpace(pixelPos, vec2(width, height));
    gl_Position.zw = vec2(0.0, 1.0);
}

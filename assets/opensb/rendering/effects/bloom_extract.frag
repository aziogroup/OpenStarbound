#version 140

uniform sampler2D mainBuffer;
uniform float bloomThreshold;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

void main() {
  vec3 color = texture(mainBuffer, fragmentTextureCoordinate).rgb;
  float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
  float contribution = smoothstep(bloomThreshold, bloomThreshold + 0.3, brightness);
  outColor = vec4(color * contribution, 1.0);
}

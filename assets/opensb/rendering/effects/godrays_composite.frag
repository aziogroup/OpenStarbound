#version 140

uniform sampler2D mainBuffer;
uniform sampler2D godrayBuffer;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

void main() {
  vec3 scene = texture(mainBuffer, fragmentTextureCoordinate).rgb;
  vec3 godrays = texture(godrayBuffer, fragmentTextureCoordinate).rgb;

  // Additive composite
  vec3 result = scene + godrays;

  outColor = vec4(result, 1.0);
}

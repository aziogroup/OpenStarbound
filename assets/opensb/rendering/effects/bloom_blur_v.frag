#version 140

uniform sampler2D sourceBuffer;
uniform vec2 sourceBufferSize;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

void main() {
  vec2 texelSize = 1.0 / sourceBufferSize;
  vec3 result = vec3(0.0);

  // 13-tap Gaussian kernel
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0, -6.0) * texelSize).rgb * 0.002216;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0, -5.0) * texelSize).rgb * 0.008764;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0, -4.0) * texelSize).rgb * 0.026995;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0, -3.0) * texelSize).rgb * 0.064759;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0, -2.0) * texelSize).rgb * 0.120985;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0, -1.0) * texelSize).rgb * 0.176033;
  result += texture(sourceBuffer, fragmentTextureCoordinate).rgb * 0.199471;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0,  1.0) * texelSize).rgb * 0.176033;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0,  2.0) * texelSize).rgb * 0.120985;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0,  3.0) * texelSize).rgb * 0.064759;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0,  4.0) * texelSize).rgb * 0.026995;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0,  5.0) * texelSize).rgb * 0.008764;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(0.0,  6.0) * texelSize).rgb * 0.002216;

  outColor = vec4(result, 1.0);
}

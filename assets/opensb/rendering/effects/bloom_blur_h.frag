#version 140

uniform sampler2D sourceBuffer;
uniform vec2 sourceBufferSize;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

void main() {
  vec2 texelSize = 1.0 / sourceBufferSize;
  vec3 result = vec3(0.0);

  // 13-tap Gaussian kernel
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(-6.0, 0.0) * texelSize).rgb * 0.002216;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(-5.0, 0.0) * texelSize).rgb * 0.008764;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(-4.0, 0.0) * texelSize).rgb * 0.026995;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(-3.0, 0.0) * texelSize).rgb * 0.064759;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(-2.0, 0.0) * texelSize).rgb * 0.120985;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2(-1.0, 0.0) * texelSize).rgb * 0.176033;
  result += texture(sourceBuffer, fragmentTextureCoordinate).rgb * 0.199471;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2( 1.0, 0.0) * texelSize).rgb * 0.176033;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2( 2.0, 0.0) * texelSize).rgb * 0.120985;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2( 3.0, 0.0) * texelSize).rgb * 0.064759;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2( 4.0, 0.0) * texelSize).rgb * 0.026995;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2( 5.0, 0.0) * texelSize).rgb * 0.008764;
  result += texture(sourceBuffer, fragmentTextureCoordinate + vec2( 6.0, 0.0) * texelSize).rgb * 0.002216;

  outColor = vec4(result, 1.0);
}

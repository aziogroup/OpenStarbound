#version 140

uniform sampler2D mainBuffer;
uniform float fogDensity;
uniform float weatherIntensity;
uniform float dayLevel;
uniform float gameTime;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

// Hash-based noise (no texture needed)
float hash(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float a = hash(i);
  float b = hash(i + vec2(1.0, 0.0));
  float c = hash(i + vec2(0.0, 1.0));
  float d = hash(i + vec2(1.0, 1.0));
  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
  float f = 0.0;
  f += 0.5000 * noise(p); p *= 2.01;
  f += 0.2500 * noise(p); p *= 2.02;
  f += 0.1250 * noise(p); p *= 2.03;
  f += 0.0625 * noise(p);
  return f / 0.9375;
}

void main() {
  vec3 scene = texture(mainBuffer, fragmentTextureCoordinate).rgb;

  // Animated fog using FBM noise
  vec2 fogUV = fragmentTextureCoordinate * vec2(4.0, 2.0);
  fogUV.x += gameTime * 0.05;
  float fogNoise = fbm(fogUV);

  // Fog density modulated by weather and settings
  float density = fogDensity * (0.3 + weatherIntensity * 0.7);
  float fogAlpha = fogNoise * density;

  // Fog color - slightly tinted based on time of day
  vec3 fogColor = mix(vec3(0.15, 0.18, 0.25), vec3(0.6, 0.55, 0.5), dayLevel);

  // Simple light scatter approximation
  vec3 scattered = fogColor * (0.5 + dayLevel * 0.5);

  vec3 result = mix(scene, scattered, clamp(fogAlpha, 0.0, 0.6));
  outColor = vec4(result, 1.0);
}

#version 140

uniform sampler2D mainBuffer;
uniform float weatherIntensity;
uniform float gameTime;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

float hash(float n) {
  return fract(sin(n) * 43758.5453);
}

void main() {
  vec3 scene = texture(mainBuffer, fragmentTextureCoordinate).rgb;

  if (weatherIntensity < 0.3) {
    outColor = vec4(scene, 1.0);
    return;
  }

  // Lightning timing: random flashes during storms
  float flashPeriod = 3.0 + hash(floor(gameTime * 0.3)) * 5.0;
  float flashTime = mod(gameTime, flashPeriod);

  float intensity = 0.0;
  if (flashTime < 0.05) {
    // Sharp rise
    intensity = flashTime / 0.05;
  } else if (flashTime < 0.35) {
    // Exponential decay
    intensity = exp(-(flashTime - 0.05) * 10.0);
  }

  // Secondary flash
  if (flashTime > 0.15 && flashTime < 0.2) {
    intensity = max(intensity, (0.2 - flashTime) / 0.05 * 0.5);
  }

  // Modulate by weather intensity (storms only)
  intensity *= smoothstep(0.3, 0.7, weatherIntensity);

  // Flash position - random horizontal offset
  float flashX = hash(floor(gameTime * 0.3) + 1.0);
  float falloff = 1.0 - abs(fragmentTextureCoordinate.x - flashX) * 0.5;
  falloff = max(falloff, 0.3); // minimum global flash

  // Vertical falloff - brighter at top
  falloff *= 0.5 + fragmentTextureCoordinate.y * 0.5;

  vec3 flashColor = vec3(0.9, 0.92, 1.0);
  vec3 result = scene + flashColor * intensity * falloff * 0.8;

  outColor = vec4(result, 1.0);
}

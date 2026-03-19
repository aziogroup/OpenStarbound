#version 140

uniform sampler2D mainBuffer;
uniform float godrayDensity;
uniform float dayLevel;
uniform float gameTime;
uniform vec2 screenSize;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

void main() {
  // Sun position - moves across screen based on time of day
  vec2 sunPos = vec2(gameTime, 0.85);

  vec2 uv = fragmentTextureCoordinate;
  vec2 deltaUV = (uv - sunPos);

  // Scale for radial blur
  float decay = 0.96;
  float density = godrayDensity * 0.5;
  float weight = 0.15;
  float illuminationDecay = 1.0;

  deltaUV *= 1.0 / 32.0; // 32 samples

  vec3 result = vec3(0.0);
  vec2 sampleUV = uv;

  for (int i = 0; i < 32; i++) {
    sampleUV -= deltaUV;
    vec3 samp = texture(mainBuffer, clamp(sampleUV, 0.0, 1.0)).rgb;

    // Use bright parts as light source (sky)
    float brightness = dot(samp, vec3(0.2126, 0.7152, 0.0722));
    samp *= smoothstep(0.5, 1.0, brightness);

    samp *= illuminationDecay * weight;
    result += samp;
    illuminationDecay *= decay;
  }

  // Modulate by day level - strongest at sunrise/sunset
  float timeModulation = 1.0 - abs(dayLevel * 2.0 - 1.0);
  timeModulation = pow(timeModulation, 0.5) * 0.8 + 0.2;

  result *= density * timeModulation;

  outColor = vec4(result, 1.0);
}

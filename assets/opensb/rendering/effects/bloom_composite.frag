#version 140

uniform sampler2D mainBuffer;
uniform sampler2D bloomBuffer;
uniform vec2 mainBufferSize;
uniform float bloomIntensity;
uniform float aoIntensity;
uniform float colorBleedIntensity;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

// --- Ambient Occlusion ---
// Darkens areas that are already in shadow near bright boundaries (depth at cave walls, cliff faces)
float computeAO(vec2 uv) {
  if (aoIntensity <= 0.0) return 1.0;

  vec2 texel = 1.0 / mainBufferSize;
  vec3 lum = vec3(0.2126, 0.7152, 0.0722);

  float center = dot(texture(mainBuffer, uv).rgb, lum);

  // Only apply AO to dark/medium areas (shadows, caves)
  // Bright areas (sky, well-lit surfaces) don't need AO
  if (center > 0.6) return 1.0;

  // Sample in a wider radius to detect nearby bright areas
  float brightNearby = 0.0;
  float darkNearby = 0.0;

  for (int y = -3; y <= 3; y += 2) {
    for (int x = -3; x <= 3; x += 2) {
      if (x == 0 && y == 0) continue;
      float s = dot(texture(mainBuffer, uv + vec2(float(x), float(y)) * texel * 3.0).rgb, lum);
      if (s > center + 0.1) brightNearby += s - center;
      if (s < center) darkNearby += center - s;
    }
  }

  // Near a bright/dark boundary AND on the dark side = occluded
  float proximity = clamp(brightNearby * 2.0, 0.0, 1.0);
  // Deeper shadows get more AO
  float depthFactor = 1.0 - center;

  float ao = 1.0 - proximity * depthFactor * aoIntensity * 0.5;
  return clamp(ao, 0.5, 1.0);
}

// --- Light Color Bleeding ---
// Bright colored areas bleed their color into nearby darker pixels
vec3 applyColorBleed(vec3 scene, vec2 uv) {
  if (colorBleedIntensity <= 0.0) return scene;

  vec2 texel = 1.0 / mainBufferSize;
  vec3 lum = vec3(0.2126, 0.7152, 0.0722);

  float centerLum = dot(scene, lum);

  // Accumulate bright colored light from surrounding area
  vec3 bleedColor = vec3(0.0);
  float totalWeight = 0.0;

  // Sample at larger distances (8-16 pixels) to simulate light spreading
  for (int y = -2; y <= 2; y++) {
    for (int x = -2; x <= 2; x++) {
      if (x == 0 && y == 0) continue;
      vec2 offset = vec2(float(x), float(y)) * texel * 6.0;
      vec3 sampleColor = texture(mainBuffer, uv + offset).rgb;
      float sampleLum = dot(sampleColor, lum);

      // Only bleed from brighter pixels into darker ones
      float lumDiff = sampleLum - centerLum;
      if (lumDiff > 0.05) {
        float weight = lumDiff;
        bleedColor += sampleColor * weight;
        totalWeight += weight;
      }
    }
  }

  if (totalWeight < 0.01) return scene;

  bleedColor /= totalWeight;

  // Bleed strength: stronger for darker pixels near bright sources
  float bleedAmount = clamp(totalWeight * (1.0 - centerLum) * colorBleedIntensity * 0.15, 0.0, 0.25);

  return mix(scene, bleedColor, bleedAmount);
}

void main() {
  vec2 uv = fragmentTextureCoordinate;
  vec3 scene = texture(mainBuffer, uv).rgb;
  vec3 bloom = texture(bloomBuffer, uv).rgb;

  // Ambient Occlusion - deepen shadows near lit/unlit boundaries
  float ao = computeAO(uv);
  scene *= ao;

  // Light Color Bleeding - bright lights tint nearby dark areas
  scene = applyColorBleed(scene, uv);

  // Bloom glow
  scene += bloom * bloomIntensity;

  scene = min(scene, vec3(1.0));
  outColor = vec4(scene, 1.0);
}

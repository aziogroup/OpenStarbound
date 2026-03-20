#version 140

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform bool lightMapEnabled;
uniform vec2 lightMapSize;
uniform sampler2D lightMap;
uniform float lightMapMultiplier;

// GPU lighting uniforms
uniform bool gpuLightingEnabled;
uniform sampler2D occlusionMap;
uniform vec2 occlusionMapSize;
uniform sampler2D lightData;
uniform int lightCount;
uniform vec2 occlusionOffset;
uniform vec2 lightMapWorldOrigin;

in vec2 fragmentTextureCoordinate;
flat in int fragmentTextureIndex;
in vec4 fragmentColor;
in float fragmentLightMapMultiplier;
in vec2 fragmentLightMapCoordinate;

out vec4 outColor;

vec4 cubic(float v) {
  vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
  vec4 s = n * n * n;
  float x = s.x;
  float y = s.y - 4.0 * s.x;
  float z = s.z - 4.0 * s.y + 6.0 * s.x;
  float w = 6.0 - x - y - z;
  return vec4(x, y, z, w);
}

vec4 bicubicSample(sampler2D tex, vec2 texcoord, vec2 texscale) {
  texcoord = texcoord - vec2(0.5, 0.5);
  float fx = fract(texcoord.x);
  float fy = fract(texcoord.y);
  texcoord.x -= fx;
  texcoord.y -= fy;
  vec4 xcubic = cubic(fx);
  vec4 ycubic = cubic(fy);
  vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
  vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
  vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;
  vec4 sample0 = texture(tex, vec2(offset.x, offset.z) * texscale);
  vec4 sample1 = texture(tex, vec2(offset.y, offset.z) * texscale);
  vec4 sample2 = texture(tex, vec2(offset.x, offset.w) * texscale);
  vec4 sample3 = texture(tex, vec2(offset.y, offset.w) * texscale);
  float sx = s.x / (s.x + s.y);
  float sy = s.z / (s.z + s.w);
  return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

vec3 sampleLight(vec2 coord, vec2 scale) {
  const float threshold = 1.0;
  vec3 rgb = bicubicSample(lightMap, coord, scale).rgb;
  vec3 lower = min(rgb, threshold);
  vec3 upper = max(rgb, threshold) - threshold;
  return lower + (upper / (vec3(1.) + upper));
}

// GPU ray-march shadow (0.0 = fully shadowed, 1.0 = fully lit)
// Uses Beer-Lambert absorption for smooth, band-free shadows
float rayMarchShadow(vec2 from, vec2 to) {
  vec2 dir = to - from;
  float dist = length(dir);
  if (dist < 0.01) return 1.0;
  dir /= dist;
  int steps = int(min(dist * 1.5, 32.0));
  float stepSize = dist / float(max(steps, 1));
  float absorption = 0.0;
  for (int i = 1; i < steps; i++) {
    vec2 samplePos = from + dir * (float(i) * stepSize);
    vec2 uv = (samplePos - occlusionOffset) / occlusionMapSize;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) continue;
    float occ = texture(occlusionMap, uv).r;
    absorption += occ * stepSize;
    if (absorption > 3.0) return 0.0;
  }
  return exp(-absorption * 3.0);
}

void main() {
  vec4 texColor;
  if (fragmentTextureIndex == 3)
    texColor = texture(texture3, fragmentTextureCoordinate);
  else if (fragmentTextureIndex == 2)
    texColor = texture(texture2, fragmentTextureCoordinate);
  else if (fragmentTextureIndex == 1)
    texColor = texture(texture1, fragmentTextureCoordinate);
  else
    texColor = texture(texture0, fragmentTextureCoordinate);

  if (texColor.a <= 0.0)
    discard;

  vec4 finalColor = texColor * fragmentColor;
  float finalLightMapMultiplier = fragmentLightMapMultiplier * lightMapMultiplier;
  if (texColor.a == 0.99607843137)
    finalColor.a = fragmentColor.a;
  else if (gpuLightingEnabled && finalLightMapMultiplier > 0.0) {
    vec2 worldPos = fragmentLightMapCoordinate + lightMapWorldOrigin;
    vec3 cpuLight = sampleLight(fragmentLightMapCoordinate, 1.0 / lightMapSize);
    // GPU shadow only darkens, never brightens
    float shadowFactor = 1.0;
    for (int i = 0; i < lightCount && i < 128; i++) {
      vec4 d0 = texelFetch(lightData, ivec2(i, 0), 0);
      vec2 lightPos = d0.xy;
      float radius = d0.z;
      float dist = length(lightPos - worldPos);
      if (dist > radius) continue;
      float atten = 1.0 - (dist / radius);
      atten *= atten;
      if (atten > 0.05) {
        float shadow = rayMarchShadow(worldPos, lightPos);
        shadowFactor = min(shadowFactor, mix(1.0, shadow, atten));
      }
    }
    finalColor.rgb *= cpuLight * shadowFactor * finalLightMapMultiplier;
  } else if (lightMapEnabled && finalLightMapMultiplier > 0.0)
    finalColor.rgb *= sampleLight(fragmentLightMapCoordinate, 1.0 / lightMapSize) * finalLightMapMultiplier;
  outColor = finalColor;
}

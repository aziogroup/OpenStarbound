#version 140

uniform sampler2D mainBuffer;
uniform float weatherIntensity;
uniform float windStrength;
uniform float gameTime;

in vec2 fragmentTextureCoordinate;
out vec4 outColor;

float hash(vec2 p) {
  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Procedural raindrop on screen
float raindrop(vec2 uv, float seed) {
  vec2 grid = floor(uv * vec2(12.0, 6.0) + seed);
  vec2 f = fract(uv * vec2(12.0, 6.0) + seed);

  float h = hash(grid);
  if (h > weatherIntensity * 0.8) return 0.0;

  // Drop center with random offset
  vec2 center = vec2(hash(grid + 0.1), hash(grid + 0.2));
  // Animate: slide down
  float slide = fract(gameTime * (0.5 + h * 0.5) + h);
  center.y = mod(center.y + slide, 1.0);

  float dist = length((f - center) * vec2(1.0, 2.0));
  float drop = smoothstep(0.15, 0.0, dist);

  // Trail
  float trail = smoothstep(0.3, 0.0, abs(f.x - center.x) * 4.0)
              * smoothstep(0.0, 0.3, f.y - center.y)
              * smoothstep(0.6, 0.3, f.y - center.y);

  return (drop + trail * 0.3) * (1.0 - slide);
}

void main() {
  vec3 scene = texture(mainBuffer, fragmentTextureCoordinate).rgb;

  if (weatherIntensity < 0.01) {
    outColor = vec4(scene, 1.0);
    return;
  }

  // Multiple layers for depth
  float drops = 0.0;
  drops += raindrop(fragmentTextureCoordinate, 0.0) * 0.6;
  drops += raindrop(fragmentTextureCoordinate * 1.5 + 0.3, 7.0) * 0.3;
  drops += raindrop(fragmentTextureCoordinate * 2.0 + 0.7, 13.0) * 0.15;

  drops *= weatherIntensity;

  // Distort scene behind droplets
  vec2 distortedUV = fragmentTextureCoordinate + vec2(drops * 0.01 * windStrength, -drops * 0.005);
  vec3 distorted = texture(mainBuffer, clamp(distortedUV, 0.0, 1.0)).rgb;

  // Droplet highlight
  vec3 dropColor = distorted + vec3(0.1, 0.12, 0.15) * drops;

  vec3 result = mix(scene, dropColor, clamp(drops, 0.0, 1.0));
  outColor = vec4(result, 1.0);
}

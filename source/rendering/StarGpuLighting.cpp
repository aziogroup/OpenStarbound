#include "StarGpuLighting.hpp"
#include "StarWorldTiles.hpp"
#include "StarRoot.hpp"
#include "StarMaterialDatabase.hpp"

namespace Star {

GpuLighting::GpuLighting() {
  m_occlusionSize = {0, 0};
  m_lightCount = 0;
}

void GpuLighting::update(WorldRenderData const& renderData,
                          List<LightSource> const& lights,
                          Vec2F cameraCenter, float pixelRatio,
                          Vec2U screenSize) {
  // Build occlusion map from tile data
  // The tile array covers the visible area + border
  auto tileSize = renderData.tiles.size();
  if (tileSize[0] == 0 || tileSize[1] == 0) {
    m_occlusionSize = {1, 1};
    m_occlusionMap = Image::filled(m_occlusionSize, Vec4B(0, 0, 0, 255), PixelFormat::RGBA32);
    m_occlusionOffset = Vec2F();
    m_lightCount = 0;
    return;
  }

  m_occlusionSize = Vec2U(tileSize);
  m_occlusionOffset = Vec2F(renderData.tileMinPosition);

  m_occlusionMap = Image(m_occlusionSize, PixelFormat::RGBA32);

  for (size_t y = 0; y < m_occlusionSize[1]; ++y) {
    for (size_t x = 0; x < m_occlusionSize[0]; ++x) {
      auto const& tile = renderData.tiles(x, y);
      // Only solid block tiles cast shadows (not platforms, objects, empty)
      bool solid = false;
      if (tile.foreground != EmptyMaterialId && tile.foreground != NullMaterialId) {
        auto collisionKind = Root::singleton().materialDatabase()->materialCollisionKind(tile.foreground);
        solid = (collisionKind == CollisionKind::Block || collisionKind == CollisionKind::Slippery);
      }
      uint8_t occ = solid ? 255 : 0;
      m_occlusionMap.set(x, y, Vec4B(occ, occ, occ, 255));
    }
  }

  // Pack light data into a texture
  // Each light takes one column (x = lightIndex), two rows:
  // Row 0: (posX, posY, radius, colorR)
  // Row 1: (colorG, colorB, beam, beamAngle)
  m_lightCount = min((unsigned)lights.size(), MaxLights);

  unsigned texWidth = max(m_lightCount, 1u);
  m_lightDataTexture = Image(Vec2U(texWidth, 2), PixelFormat::RGBA_F);

  for (unsigned i = 0; i < m_lightCount; ++i) {
    auto const& light = lights[i];
    float radius = max(light.color[0], max(light.color[1], light.color[2]));
    // Scale radius based on light intensity - brighter lights reach further
    radius = radius * 16.0f; // approximate tile radius

    // Row 0: position, radius, color.r
    float* row0 = (float*)m_lightDataTexture.data() + i * 4;
    row0[0] = light.position[0];
    row0[1] = light.position[1];
    row0[2] = radius;
    row0[3] = light.color[0];

    // Row 1: color.g, color.b, beam, beamAngle
    float* row1 = (float*)m_lightDataTexture.data() + (texWidth * 4) + i * 4;
    row1[0] = light.color[1];
    row1[1] = light.color[2];
    row1[2] = light.pointBeam;
    row1[3] = light.beamAngle;
  }
}

void GpuLighting::uploadToRenderer(RendererPtr const& renderer) {
  renderer->setEffectTexture("occlusionMap", m_occlusionMap);
  if (m_lightCount > 0)
    renderer->setEffectTexture("lightData", m_lightDataTexture);
}

unsigned GpuLighting::lightCount() const {
  return m_lightCount;
}

Vec2F GpuLighting::occlusionMapSize() const {
  return Vec2F(m_occlusionSize);
}

Vec2F GpuLighting::occlusionOffset() const {
  return m_occlusionOffset;
}

}

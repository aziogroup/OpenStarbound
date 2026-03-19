#pragma once

#include "StarWorldRenderData.hpp"
#include "StarRenderer.hpp"
#include "StarLightSource.hpp"

namespace Star {

STAR_CLASS(GpuLighting);

class GpuLighting {
public:
  GpuLighting();

  // Build occlusion map from tile data, pack light source data into texture
  void update(WorldRenderData const& renderData,
              List<LightSource> const& lights,
              Vec2F cameraCenter, float pixelRatio,
              Vec2U screenSize);

  // Upload computed textures as effect textures via renderer
  void uploadToRenderer(RendererPtr const& renderer);

  unsigned lightCount() const;
  Vec2F occlusionMapSize() const;
  Vec2F occlusionOffset() const;

private:
  static constexpr unsigned MaxLights = 128;

  Image m_occlusionMap;
  Image m_lightDataTexture;
  unsigned m_lightCount = 0;
  Vec2F m_occlusionOffset;
  Vec2U m_occlusionSize;
};

}

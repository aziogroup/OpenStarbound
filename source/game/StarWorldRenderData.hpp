#pragma once

#include "StarImage.hpp"
#include "StarWorldTiles.hpp"
#include "StarEntityRenderingTypes.hpp"
#include "StarSkyRenderData.hpp"
#include "StarParallax.hpp"
#include "StarParticle.hpp"
#include "StarWeatherTypes.hpp"
#include "StarEntity.hpp"
#include "StarThread.hpp"
#include "StarCellularLighting.hpp"
#include "StarLightSource.hpp"

namespace Star {

struct EntityDrawables {
  EntityHighlightEffect highlightEffect;
  Map<EntityRenderLayer, List<Drawable>> layers;
};


struct WorldRenderData {
  void clear();

  WorldGeometry geometry;

  Vec2I tileMinPosition;
  RenderTileArray tiles;
  Vec2I lightMinPosition;
  Lightmap lightMap;

  List<EntityDrawables> entityDrawables;
  List<Particle> const* particles;

  List<OverheadBar> overheadBars;
  List<Drawable> nametags;

  List<Drawable> backgroundOverlays;
  List<Drawable> foregroundOverlays;

  List<ParallaxLayer> parallaxLayers;

  SkyRenderData skyRenderData;

  bool isFullbright = false;
  float dimLevel = 0.0f;
  Vec3B dimColor;

  // GPU lighting data
  List<LightSource> lightSources;

  // Weather/atmosphere data for shader pipeline
  float weatherIntensity = 0.0f;
  float windStrength = 0.0f;
  float undergroundLevel = 0.0f;
  float gameTime = 0.0f;
  float dayLevel = 1.0f;
  Vec3F environmentLight = Vec3F(1.0f, 1.0f, 1.0f);
  Vec2F sunScreenPosition;
};

inline void WorldRenderData::clear() {
  tiles.resize({0, 0}); // keep reserved

  entityDrawables.clear();
  particles = nullptr;
  overheadBars.clear();
  nametags.clear();
  backgroundOverlays.clear();
  foregroundOverlays.clear();
  parallaxLayers.clear();
  lightSources.clear();
}

}

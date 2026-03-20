#include "StarTranslationDatabase.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarLogging.hpp"
#include "StarText.hpp"

namespace Star {

TranslationDatabase::TranslationDatabase() {
  auto assets = Root::singleton().assets();

  auto translationFiles = assets->scanExtension("translation");
  if (translationFiles.empty()) {
    Logger::info("TranslationDatabase: No .translation files found");
    return;
  }

  for (auto const& path : translationFiles) {
    try {
      auto json = assets->json(path);
      if (json.isType(Json::Type::Object)) {
        for (auto const& pair : json.iterateObject()) {
          if (pair.second.isType(Json::Type::String))
            m_translations[pair.first] = pair.second.toString();
        }
      }
      Logger::info("TranslationDatabase: Loaded {} from {}", json.size(), path);
    } catch (std::exception const& e) {
      Logger::error("TranslationDatabase: Failed to load {}: {}", path, e.what());
    }
  }

  // Build stripped escape code index for fallback matching
  for (auto const& pair : m_translations) {
    String stripped = Text::stripEscapeCodes(pair.first);
    if (stripped != pair.first)
      m_strippedIndex[stripped] = pair.second;
  }

  Logger::info("TranslationDatabase: Total {} translations loaded ({} stripped fallbacks)",
    m_translations.size(), m_strippedIndex.size());
}

Maybe<String> TranslationDatabase::translate(StringView original) const {
  if (m_translations.empty())
    return {};

  String key(original);

  // 1. Exact match
  auto it = m_translations.find(key);
  if (it != m_translations.end())
    return it->second;

  // 2. Stripped escape codes match
  String stripped = Text::stripEscapeCodes(key);
  if (stripped.size() != key.size()) {
    it = m_strippedIndex.find(stripped);
    if (it != m_strippedIndex.end())
      return it->second;
    it = m_translations.find(stripped);
    if (it != m_translations.end())
      return it->second;
  }

  return {};
}

size_t TranslationDatabase::size() const {
  return m_translations.size();
}

}

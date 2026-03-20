#pragma once

#include "StarString.hpp"
#include "StarMap.hpp"
#include "StarMaybe.hpp"
#include "StarAssets.hpp"

namespace Star {

STAR_CLASS(TranslationDatabase);

// Simple translation lookup table that maps original text to translated text.
// Loaded from JSON files in assets. Used by TextPainter to translate display
// text at render time without modifying the underlying game data.
class TranslationDatabase {
public:
  TranslationDatabase();

  // Look up a translation for the given text.
  // Tries: 1) exact match, 2) format-codes-stripped match,
  // 3) prefix match (for typewriter effects - returns proportionally truncated translation)
  Maybe<String> translate(StringView original) const;

  size_t size() const;

private:
  HashMap<String, String> m_translations;
  HashMap<String, String> m_strippedIndex;
};

}

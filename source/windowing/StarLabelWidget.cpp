#include "StarLabelWidget.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarTranslationDatabase.hpp"
#include "StarText.hpp"

namespace Star {

LabelWidget::LabelWidget(String text,
    Color const& color,
    HorizontalAnchor const& hAnchor,
    VerticalAnchor const& vAnchor,
    Maybe<unsigned> wrapWidth,
    Maybe<float> lineSpacing)
  : m_hAnchor(hAnchor),
    m_vAnchor(vAnchor),
    m_wrapWidth(std::move(wrapWidth)) {
  auto assets = Root::singleton().assets();
  m_style = assets->json("/interface.config:labelTextStyle");
  m_style.color = color.toRgba();
  if (lineSpacing)
    m_style.lineSpacing = *lineSpacing;
  setText(std::move(text));
}

String const& LabelWidget::text() const {
  return m_text;
}

Maybe<unsigned> LabelWidget::getTextCharLimit() const {
  return m_textCharLimit;
}

void LabelWidget::setText(String newText) {
  m_text = std::move(newText);
  // Translate text for display
  m_translatedText = {};
  if (auto* root = Root::singletonPtr()) {
    if (auto db = root->translationDatabase()) {
      if (auto trans = db->translate(m_text))
        m_translatedText = std::move(*trans);
    }
  }
  updateTextRegion();
}

void LabelWidget::setFontSize(int fontSize) {
  m_style.fontSize = fontSize;
  updateTextRegion();
}

void LabelWidget::setFontMode(FontMode fontMode) {
  m_style.shadow = fontModeToColor(fontMode).toRgba();
}

void LabelWidget::setColor(Color newColor) {
  m_style.color = newColor.toRgba();
}

void LabelWidget::setAnchor(HorizontalAnchor hAnchor, VerticalAnchor vAnchor) {
  m_hAnchor = hAnchor;
  m_vAnchor = vAnchor;
  updateTextRegion();
}

void LabelWidget::setWrapWidth(Maybe<unsigned> wrapWidth) {
  m_wrapWidth = std::move(wrapWidth);
  updateTextRegion();
}

void LabelWidget::setLineSpacing(Maybe<float> lineSpacing) {
  m_style.lineSpacing = lineSpacing.value(DefaultLineSpacing);
  updateTextRegion();
}

void LabelWidget::setDirectives(String const& directives) {
  m_style.directives = directives;
  updateTextRegion();
}

void LabelWidget::setTextCharLimit(Maybe<unsigned> charLimit) {
  m_textCharLimit = charLimit;
  updateTextRegion();
}

void LabelWidget::setTextStyle(TextStyle const& textStyle) {
  m_style = textStyle;
  updateTextRegion();
}


RectI LabelWidget::relativeBoundRect() const {
  return RectI(m_textRegion).translated(relativePosition());
}

RectI LabelWidget::getScissorRect() const {
  return noScissor();
}

void LabelWidget::renderImpl() {
  context()->setTextStyle(m_style);
  String const& displayText = m_translatedText ? *m_translatedText : m_text;
  context()->renderInterfaceText(displayText, {Vec2F(screenPosition()), m_hAnchor, m_vAnchor, m_wrapWidth, m_textCharLimit});
}

void LabelWidget::updateTextRegion() {
  context()->setTextStyle(m_style);
  String const& displayText = m_translatedText ? *m_translatedText : m_text;
  m_textRegion = RectI(context()->determineInterfaceTextSize(displayText, {Vec2F(), m_hAnchor, m_vAnchor, m_wrapWidth, m_textCharLimit}));
  setSize(m_textRegion.size());
}

}

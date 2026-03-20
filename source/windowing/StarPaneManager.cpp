#include "StarPaneManager.hpp"
#include "StarGameTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

namespace {

struct InterfaceScaleOverride {
  InterfaceScaleOverride(GuiContext* context, float interfaceScale)
    : m_context(context) {
    m_context->setInterfaceScaleOverride(interfaceScale);
  }

  ~InterfaceScaleOverride() {
    m_context->setInterfaceScaleOverride({});
  }

  GuiContext* m_context;
};

}

EnumMap<PaneLayer> const PaneLayerNames{
  {PaneLayer::Tooltip, "Tooltip"},
  {PaneLayer::ModalWindow, "ModalWindow"},
  {PaneLayer::Window, "Window"},
  {PaneLayer::Hud, "Hud"},
  {PaneLayer::World, "World"}
};

PaneManager::PaneManager()
  : m_context(GuiContext::singletonPtr()) {
  auto assets = Root::singleton().assets();
  m_tooltipMouseoverRadius = assets->json("/panes.config:tooltipMouseoverRadius").toFloat();
  m_tooltipMouseOffset = jsonToVec2I(assets->json("/panes.config:tooltipMouseoverOffset"));
  m_tooltipShowTimer = GameTimer(assets->json("/panes.config:tooltipMouseoverTime").toFloat());

  for (auto const& paneLayer : PaneLayerNames)
    m_prevInterfaceScale.set(paneLayer.first, interfaceScale(paneLayer.first));
}

void PaneManager::displayPane(PaneLayer paneLayer, PanePtr const& pane, DismissCallback onDismiss) {
  if (!m_displayedPanes[paneLayer].insertFront(pane, std::move(onDismiss)).second)
    throw GuiException("Pane displayed twice in PaneManager::displayPane");

  if (!pane->hasDisplayed() && pane->anchor() == PaneAnchor::None)
    pane->setPosition(Vec2I((windowSize(paneLayer) - pane->size()) / 2) + pane->centerOffset()); // center it

  pane->displayed();
}

bool PaneManager::isDisplayed(PanePtr const& pane) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (layerPair.second.contains(pane))
      return true;
  }

  return false;
}

void PaneManager::dismissPane(PanePtr const& pane) {
  if (!dismiss(pane))
    throw GuiException("No such pane in PaneManager::dismissPane");
}

void PaneManager::dismissAllPanes(Set<PaneLayer> const& paneLayers) {
  for (auto const& paneLayer : paneLayers) {
    for (auto const& panePair : copy(m_displayedPanes[paneLayer]))
      dismiss(panePair.first);
  }
}

void PaneManager::dismissAllPanes() {
  for (auto layerPair : copy(m_displayedPanes)) {
    for (auto const& panePair : layerPair.second)
      dismiss(panePair.first);
  }
}

PanePtr PaneManager::topPane(Set<PaneLayer> const& paneLayers) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (paneLayers.contains(layerPair.first) && !layerPair.second.empty())
      return layerPair.second.firstKey();
  }
  return {};
}

PanePtr PaneManager::topPane() const {
  for (auto const& layerPair : m_displayedPanes) {
    if (!layerPair.second.empty())
      return layerPair.second.firstKey();
  }
  return {};
}

void PaneManager::bringToTop(PanePtr const& pane) {
  for (auto& layerPair : m_displayedPanes) {
    if (layerPair.second.contains(pane)) {
      layerPair.second.toFront(pane);
      return;
    }
  }

  throw GuiException("Pane was not displayed in PaneManager::bringToTop");
}

void PaneManager::bringPaneAdjacent(PanePtr const& anchor, PanePtr const& adjacent, int gap) {
  Vec2I centerAdjacent = anchor->position() + (anchor->size() / 2) - (adjacent->size() / 2);
  auto adjacentLayer = paneLayer(adjacent).value(PaneLayer::Window);
  centerAdjacent = centerAdjacent.piecewiseClamp(Vec2I(), windowSize(adjacentLayer) - adjacent->size()); // keeps pane inside window

  if (anchor->position()[0] + anchor->size()[0] + gap + adjacent->size()[0] <= windowSize(adjacentLayer)[0])
    adjacent->setPosition(Vec2I(anchor->position()[0] + anchor->size()[0] + gap, centerAdjacent[1])); // place to the right
  else if (anchor->position()[0] - gap - adjacent->size()[0] >= 0)
    adjacent->setPosition(Vec2I(anchor->position()[0] - gap - adjacent->size()[0], centerAdjacent[1])); // place to the left
  else if (anchor->position()[1] + anchor->size()[1] + gap + adjacent->size()[1] <= windowSize(adjacentLayer)[1])
    adjacent->setPosition(Vec2I(centerAdjacent[0], anchor->position()[1] + anchor->size()[1] + gap)); // place above
  else if (anchor->position()[1] - gap - adjacent->size()[1] >= 0)
    adjacent->setPosition(Vec2I(centerAdjacent[0], anchor->position()[1] - gap - adjacent->size()[1])); // place below
  else
    adjacent->setPosition(centerAdjacent);

  bringToTop(adjacent);
}

PanePtr PaneManager::getPaneAt(Set<PaneLayer> const& paneLayers, Vec2I const& position) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (!paneLayers.contains(layerPair.first))
      continue;

    for (auto const& panePair : layerPair.second) {
      if (panePair.first->inWindow(panePosition(layerPair.first, position)) && panePair.first->active())
        return panePair.first;
    }
  }

  return {};
}

PanePtr PaneManager::getPaneAt(Vec2I const& position) const {
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (panePair.first != m_activeTooltip
        && panePair.first->inWindow(panePosition(layerPair.first, position))
        && panePair.first->active())
        return panePair.first;
    }
  }

  return {};
}

List<PanePtr> PaneManager::getAllPanes() {
  List<PanePtr> list;
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (panePair.first != m_activeTooltip && panePair.first->active())
        list.append(panePair.first);
    }
  }
  return list;
}

void PaneManager::setBackgroundWidget(WidgetPtr bg) {
  m_backgroundWidget = bg;
}

void PaneManager::dismissWhere(function<bool(PanePtr const&)> func) {
  if (!func)
    return;

  for (auto& layerPair : m_displayedPanes) {
    eraseWhere(layerPair.second, [&](auto& panePair) {
      if (func(panePair.first)) {
        panePair.first->dismissed();
        if (panePair.second)
          panePair.second(panePair.first);
        return true;
      }
      return false;
    });
  }
}

PanePtr PaneManager::keyboardCapturedPane() const {
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (panePair.first->keyboardCapturer())
        return panePair.first;
    }
  }

  return {};
}

WidgetPtr PaneManager::keyboardCapturedWidget() const {
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (auto capturer = panePair.first->keyboardCapturer())
        return capturer;
    }
  }

  return {};
}

Maybe<pair<RectI, int>> PaneManager::keyboardCaptureArea() const {
  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : layerPair.second) {
      if (auto capturer = panePair.first->keyboardCapturer()) {
        InterfaceScaleOverride interfaceScaleOverride(m_context, interfaceScale(layerPair.first));
        return capturer->keyboardCaptureArea();
      }
    }
  }

  return {};
}

bool PaneManager::keyboardCapturedForTextInput() const {
  if (auto widget = keyboardCapturedWidget())
    return widget->keyboardCaptureMode() == KeyboardCaptureMode::TextInput;
  return false;
}

Vec2I PaneManager::panePosition(PanePtr const& pane, Vec2I const& screenPosition) const {
  if (auto currentLayer = paneLayer(pane))
    return panePosition(*currentLayer, screenPosition);

  return panePosition(PaneLayer::Window, screenPosition);
}

bool PaneManager::sendInputEvent(InputEvent const& event) {
  auto mouseScreenPosition = m_context->mousePosition(event, 1.0f);

  if (event.is<MouseMoveEvent>()) {
    m_tooltipLastMouseScreenPos = *mouseScreenPosition;

    for (auto const& layerPair : m_displayedPanes) {
      for (auto const& panePair : layerPair.second) {
        if (panePair.first->dragActive()) {
          panePair.first->drag(panePosition(layerPair.first, *mouseScreenPosition));
          return true;
        }
      }
    }
  }

  if (event.is<MouseButtonDownEvent>()) {
    m_tooltipShowTimer.reset();
    if (m_activeTooltip) {
      dismiss(m_activeTooltip);
      m_activeTooltip.reset();
      m_tooltipParentPane.reset();
      m_tooltipShowTimer.reset();
    }
  }

  if (event.is<MouseButtonUpEvent>()) {
    for (auto const& layerPair : m_displayedPanes) {
      for (auto const& panePair : layerPair.second) {
        if (panePair.first->dragActive()) {
          panePair.first->setDragActive(false, {});
          return true;
        }
      }
    }
  }

  // The gui close event can only be intercepted by a pane that has captured
  // the keyboard otherwise it will always be used to close first before being
  // a normal event. This is so a window can control its own closing if it
  // really needs to (like the keybindings window).
  if (event.is<KeyDownEvent>() && m_context->actions(event).contains(InterfaceAction::GuiClose)) {
    if (auto top = topPane({PaneLayer::ModalWindow, PaneLayer::Window})) {
      dismiss(top);
      return true;
    }
  }

  // If there is a pane that has captured the keyboard, keyboard events will
  // ONLY be sent to it.
  auto keyCapturePane = keyboardCapturedPane();
  if (keyCapturePane && (event.is<KeyDownEvent>() || event.is<KeyUpEvent>() || event.is<TextInputEvent>())) {
    if (auto currentLayer = paneLayer(keyCapturePane)) {
      InterfaceScaleOverride interfaceScaleOverride(m_context, interfaceScale(*currentLayer));
      return keyCapturePane->sendEvent(event);
    }
    return keyCapturePane->sendEvent(event);
  }

  bool foundModal = false;
  for (auto& layerPair : m_displayedPanes) {
    for (auto const& panePair : copy(layerPair.second)) {
      InterfaceScaleOverride interfaceScaleOverride(m_context, interfaceScale(layerPair.first));
      if (panePair.first->sendEvent(event)) {
        if (event.is<MouseButtonDownEvent>())
          layerPair.second.toFront(panePair.first);
        return true;
      }
      // If any modal windows are shown, Only the first modal window should
      // have a chance to consume the input event and all other panes below it
      // including different layers should ignore it.
      if (layerPair.first == PaneLayer::ModalWindow) {
        foundModal = true;
        break;
      }
    }
    if (foundModal)
      break;
  }

  return false;
}

void PaneManager::render() {
  if (m_backgroundWidget) {
    InterfaceScaleOverride interfaceScaleOverride(m_context, interfaceScale(PaneLayer::Window));
    auto size = m_backgroundWidget->size();
    auto backgroundWindowSize = windowSize(PaneLayer::Window);
    m_backgroundWidget->setPosition(Vec2I((backgroundWindowSize[0] - size[0]) / 2, (backgroundWindowSize[1] - size[1]) / 2));
    m_backgroundWidget->render(RectI(Vec2I(), backgroundWindowSize));
  }

  for (auto const& layerPair : reverseIterate(m_displayedPanes)) {
    float layerInterfaceScale = interfaceScale(layerPair.first);
    float previousInterfaceScale = m_prevInterfaceScale.get(layerPair.first);
    for (auto const& panePair : reverseIterate(layerPair.second)) {
      if (panePair.first->active()) {
        if (previousInterfaceScale != layerInterfaceScale)
          panePair.first->setPosition(
              calculateNewInterfacePosition(layerPair.first, panePair.first, layerInterfaceScale / previousInterfaceScale));

        panePair.first->setDrawingOffset(calculatePaneOffset(layerPair.first, panePair.first));
        InterfaceScaleOverride interfaceScaleOverride(m_context, layerInterfaceScale);
        panePair.first->render(RectI(Vec2I(), windowSize(layerPair.first)));
      }
    }
    m_prevInterfaceScale.set(layerPair.first, layerInterfaceScale);
  }

  m_context->resetInterfaceScissorRect();
}

void PaneManager::update(float dt) {
  auto newTooltipParentPane = getPaneAt(m_tooltipLastMouseScreenPos);

  bool updateTooltip = m_tooltipShowTimer.tick(dt) || (m_activeTooltip && (
    vmag(m_tooltipInitialScreenPos - m_tooltipLastMouseScreenPos) > m_tooltipMouseoverRadius
    || m_tooltipParentPane != newTooltipParentPane
    || !m_tooltipParentPane->inWindow(panePosition(m_tooltipParentPane, m_tooltipLastMouseScreenPos)))); 

  if (updateTooltip) {
    if (m_activeTooltip) {
      dismiss(m_activeTooltip);
      m_activeTooltip.reset();
      m_tooltipParentPane.reset();
    }

    m_tooltipShowTimer.reset();
    if (newTooltipParentPane) {
      if (auto tooltip = newTooltipParentPane->createTooltip(panePosition(newTooltipParentPane, m_tooltipLastMouseScreenPos))) {
        m_activeTooltip = std::move(tooltip);
        m_tooltipParentPane = std::move(newTooltipParentPane);
        m_tooltipInitialScreenPos = m_tooltipLastMouseScreenPos;
        displayPane(PaneLayer::Tooltip, m_activeTooltip);
      }
    }
  }

  if (m_activeTooltip) {
    auto tooltipMousePosition = panePosition(PaneLayer::Tooltip, m_tooltipLastMouseScreenPos);
    Vec2I offsetDirection = Vec2I::filled(1);
    Vec2I offsetAdjust = Vec2I();

    if (tooltipMousePosition[0] + m_tooltipMouseOffset[0] + m_activeTooltip->size()[0] > windowSize(PaneLayer::Tooltip)[0]) {
      offsetDirection[0] = -1;
      offsetAdjust[0] = -m_activeTooltip->size()[0];
    }

    if (tooltipMousePosition[1] + m_tooltipMouseOffset[1] - m_activeTooltip->size()[1] < 0)
      offsetDirection[1] = -1;
    else
      offsetAdjust[1] = -m_activeTooltip->size()[1];

    m_activeTooltip->setPosition(tooltipMousePosition + (offsetAdjust + m_tooltipMouseOffset.piecewiseMultiply(offsetDirection)));
  }

  for (auto const& layerPair : m_displayedPanes) {
    for (auto const& panePair : copy(layerPair.second)) {
      if (panePair.first->isDismissed())
        dismiss(panePair.first);
    }
  }

  for (auto const& layerPair : reverseIterate(m_displayedPanes)) {
    for (auto const& panePair : reverseIterate(layerPair.second)) {
      InterfaceScaleOverride interfaceScaleOverride(m_context, interfaceScale(layerPair.first));
      panePair.first->tick(dt);
      if (panePair.first->active())
        panePair.first->update(dt);
    }
  }
}

float PaneManager::hudInterfaceScale() const {
  auto configuration = Root::singleton().configuration();
  if (auto scale = configuration->get("hudInterfaceScale").optFloat().value(0.0f); scale != 0)
    return scale;

  if (auto legacyScale = configuration->get("windowInterfaceScale").optFloat().value(0.0f); legacyScale != 0)
    return legacyScale;

  return m_context->baseInterfaceScale();
}

float PaneManager::interfaceScale(PaneLayer paneLayer) const {
  float scale = paneLayer == PaneLayer::Hud ? hudInterfaceScale() : m_context->baseInterfaceScale();
  return m_context->effectiveInterfaceScale(scale);
}

Maybe<PaneLayer> PaneManager::paneLayer(PanePtr const& pane) const {
  for (auto const& layerPair : m_displayedPanes) {
    if (layerPair.second.contains(pane))
      return layerPair.first;
  }

  return {};
}

Vec2I PaneManager::windowSize(PaneLayer paneLayer) const {
  return Vec2I::ceil(Vec2F(m_context->windowSize()) / interfaceScale(paneLayer));
}

Vec2I PaneManager::panePosition(PaneLayer paneLayer, Vec2I const& screenPosition) const {
  return Vec2I(Vec2F(screenPosition) / interfaceScale(paneLayer));
}

Vec2I PaneManager::calculatePaneOffset(PaneLayer paneLayer, PanePtr const& pane) const {
  Vec2I size = pane->size();
  auto currentWindowSize = windowSize(paneLayer);
  switch (pane->anchor()) {
    case PaneAnchor::None:
      return pane->anchorOffset();
    case PaneAnchor::BottomLeft:
      return pane->anchorOffset();
    case PaneAnchor::BottomRight:
      return pane->anchorOffset() + Vec2I{currentWindowSize[0] - size[0], 0};
    case PaneAnchor::TopLeft:
      return pane->anchorOffset() + Vec2I{0, currentWindowSize[1] - size[1]};
    case PaneAnchor::TopRight:
      return pane->anchorOffset() + (currentWindowSize - size);
    case PaneAnchor::CenterTop:
      return pane->anchorOffset() + Vec2I{(currentWindowSize[0] - size[0]) / 2, currentWindowSize[1] - size[1]};
    case PaneAnchor::CenterBottom:
      return pane->anchorOffset() + Vec2I{(currentWindowSize[0] - size[0]) / 2, 0};
    case PaneAnchor::CenterLeft:
      return pane->anchorOffset() + Vec2I{0, (currentWindowSize[1] - size[1]) / 2};
    case PaneAnchor::CenterRight:
      return pane->anchorOffset() + Vec2I{currentWindowSize[0] - size[0], (currentWindowSize[1] - size[1]) / 2};
    case PaneAnchor::Center:
      return pane->anchorOffset() + ((currentWindowSize - size) / 2);
    default:
      return pane->anchorOffset();
  }
}

Vec2I PaneManager::calculateNewInterfacePosition(PaneLayer paneLayer, PanePtr const& pane, float interfaceScaleRatio) const {
  Vec2F position(pane->relativePosition());
  Vec2F size(pane->size());
  Vec2F currentWindowSize(windowSize(paneLayer));
  Mat3F scale;
  switch (pane->anchor()) {
    case PaneAnchor::None:
      scale = Mat3F::scaling(interfaceScaleRatio, currentWindowSize / 2);
      break;
    case PaneAnchor::BottomLeft:
      scale = Mat3F::scaling(interfaceScaleRatio);
      break;
    case PaneAnchor::BottomRight:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0], 0});
      break;
    case PaneAnchor::TopLeft:
      scale = Mat3F::scaling(interfaceScaleRatio, {0, size[1]});
      break;
    case PaneAnchor::TopRight:
      scale = Mat3F::scaling(interfaceScaleRatio, size);
      break;
    case PaneAnchor::CenterTop:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0] / 2, size[1]});
      break;
    case PaneAnchor::CenterBottom:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0] / 2, 0});
      break;
    case PaneAnchor::CenterLeft:
      scale = Mat3F::scaling(interfaceScaleRatio, {0, size[1] / 2});
      break;
    case PaneAnchor::CenterRight:
      scale = Mat3F::scaling(interfaceScaleRatio, {size[0], size[1] / 2});
      break;
    case PaneAnchor::Center:
      scale = Mat3F::scaling(interfaceScaleRatio, size / 2);
      break;
    default:
      scale = Mat3F::scaling(interfaceScaleRatio, currentWindowSize / 2);
  }
  return Vec2I::round((scale * Vec3F(position, 0)).vec2());
}

bool PaneManager::dismiss(PanePtr const& pane) {
  bool dismissed = false;
  for (auto& layerPair : m_displayedPanes) {
    if (auto panePair = layerPair.second.maybeTake(pane)) {
      dismissed = true;
      panePair->first->dismissed();
      if (panePair->second)
        panePair->second(pane);
    }
  }

  return dismissed;
}

}

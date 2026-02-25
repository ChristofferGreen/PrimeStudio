#include "PrimeStage/StudioUi.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PrimeStage {
namespace {

struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

bool is_utf8_continuation(uint8_t value) {
  return (value & 0xC0u) == 0x80u;
}

void apply_rect(PrimeFrame::Node& node, Rect const& rect) {
  node.localX = rect.x;
  node.localY = rect.y;
  if (rect.width > 0.0f) {
    node.sizeHint.width.preferred = rect.width;
  } else {
    node.sizeHint.width.preferred.reset();
  }
  if (rect.height > 0.0f) {
    node.sizeHint.height.preferred = rect.height;
  } else {
    node.sizeHint.height.preferred.reset();
  }
}

void apply_size_spec(PrimeFrame::Node& node, SizeSpec const& size) {
  node.sizeHint.width.min = size.minWidth;
  node.sizeHint.width.max = size.maxWidth;
  if (!node.sizeHint.width.preferred.has_value() && size.preferredWidth.has_value()) {
    node.sizeHint.width.preferred = size.preferredWidth;
  }
  node.sizeHint.width.stretch = size.stretchX;

  node.sizeHint.height.min = size.minHeight;
  node.sizeHint.height.max = size.maxHeight;
  if (!node.sizeHint.height.preferred.has_value() && size.preferredHeight.has_value()) {
    node.sizeHint.height.preferred = size.preferredHeight;
  }
  node.sizeHint.height.stretch = size.stretchY;
}

Rect resolve_rect(SizeSpec const& size) {
  Rect resolved;
  if (size.preferredWidth.has_value()) {
    resolved.width = *size.preferredWidth;
  }
  if (size.preferredHeight.has_value()) {
    resolved.height = *size.preferredHeight;
  }
  return resolved;
}

float slider_value_from_event(PrimeFrame::Event const& event, bool vertical, float thumbSize) {
  float width = std::max(0.0f, event.targetW);
  float height = std::max(0.0f, event.targetH);
  float thumb = std::max(0.0f, thumbSize);
  float clampedThumb = std::min(thumb, std::min(width, height));
  if (vertical) {
    float range = std::max(0.0f, height - clampedThumb);
    if (range <= 0.0f) {
      return 0.0f;
    }
    float pos = std::clamp(event.localY - clampedThumb * 0.5f, 0.0f, range);
    return std::clamp(1.0f - (pos / range), 0.0f, 1.0f);
  }
  float range = std::max(0.0f, width - clampedThumb);
  if (range <= 0.0f) {
    return 0.0f;
  }
  float pos = std::clamp(event.localX - clampedThumb * 0.5f, 0.0f, range);
  return std::clamp(pos / range, 0.0f, 1.0f);
}

PrimeFrame::NodeId create_node(PrimeFrame::Frame& frame,
                               PrimeFrame::NodeId parent,
                               Rect const& rect,
                               SizeSpec const* size,
                               PrimeFrame::LayoutType layout,
                               PrimeFrame::Insets const& padding,
                               float gap,
                               bool clipChildren,
                               bool visible) {
  PrimeFrame::NodeId id = frame.createNode();
  PrimeFrame::Node* node = frame.getNode(id);
  if (!node) {
    return id;
  }
  apply_rect(*node, rect);
  if (size) {
    apply_size_spec(*node, *size);
  }
  node->layout = layout;
  node->padding = padding;
  node->gap = gap;
  node->clipChildren = clipChildren;
  node->visible = visible;
  if (parent.isValid()) {
    frame.addChild(parent, id);
  } else {
    frame.addRoot(id);
  }
  return id;
}

void add_rect_primitive(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        PrimeFrame::RectStyleToken token,
                        PrimeFrame::RectStyleOverride const& overrideStyle) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Rect;
  prim.rect.token = token;
  prim.rect.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
}

PrimeFrame::PrimitiveId add_rect_primitive_with_rect(PrimeFrame::Frame& frame,
                                                     PrimeFrame::NodeId nodeId,
                                                     Rect const& rect,
                                                     PrimeFrame::RectStyleToken token,
                                                     PrimeFrame::RectStyleOverride const& overrideStyle) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Rect;
  prim.offsetX = rect.x;
  prim.offsetY = rect.y;
  prim.width = rect.width;
  prim.height = rect.height;
  prim.rect.token = token;
  prim.rect.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
  return pid;
}

void add_text_primitive(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        std::string_view text,
                        PrimeFrame::TextStyleToken textStyle,
                        PrimeFrame::TextStyleOverride const& overrideStyle,
                        PrimeFrame::TextAlign align,
                        PrimeFrame::WrapMode wrap,
                        float maxWidth,
                        float width,
                        float height) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Text;
  prim.width = width;
  prim.height = height;
  prim.textBlock.text = std::string(text);
  prim.textBlock.align = align;
  prim.textBlock.wrap = wrap;
  prim.textBlock.maxWidth = maxWidth;
  prim.textStyle.token = textStyle;
  prim.textStyle.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
}

PrimeFrame::NodeId create_rect_node(PrimeFrame::Frame& frame,
                                    PrimeFrame::NodeId parent,
                                    Rect const& rect,
                                    PrimeFrame::RectStyleToken token,
                                    PrimeFrame::RectStyleOverride const& overrideStyle,
                                    bool clipChildren,
                                    bool visible) {
  PrimeFrame::NodeId id = create_node(frame,
                                      parent,
                                      rect,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      clipChildren,
                                      visible);
  PrimeFrame::Node* node = frame.getNode(id);
  if (node) {
    node->hitTestVisible = false;
  }
  add_rect_primitive(frame, id, token, overrideStyle);
  return id;
}

PrimeFrame::NodeId create_text_node(PrimeFrame::Frame& frame,
                                    PrimeFrame::NodeId parent,
                                    Rect const& rect,
                                    std::string_view text,
                                    PrimeFrame::TextStyleToken textStyle,
                                    PrimeFrame::TextStyleOverride const& overrideStyle,
                                    PrimeFrame::TextAlign align,
                                    PrimeFrame::WrapMode wrap,
                                    float maxWidth,
                                    bool visible) {
  PrimeFrame::NodeId id = create_node(frame,
                                      parent,
                                      rect,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      visible);
  PrimeFrame::Node* node = frame.getNode(id);
  if (node) {
    node->hitTestVisible = false;
  }
  add_text_primitive(frame,
                     id,
                     text,
                     textStyle,
                     overrideStyle,
                     align,
                     wrap,
                     maxWidth,
                     rect.width,
                     rect.height);
  return id;
}

float resolve_line_height(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
  float lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : resolved.size * 1.2f;
  return lineHeight;
}

float estimate_text_width(PrimeFrame::Frame& frame,
                          PrimeFrame::TextStyleToken token,
                          std::string_view text) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
  float advance = resolved.size * 0.6f + resolved.tracking;
  float lineWidth = 0.0f;
  float maxWidth = 0.0f;
  for (char ch : text) {
    if (ch == '\n') {
      maxWidth = std::max(maxWidth, lineWidth);
      lineWidth = 0.0f;
      continue;
    }
    lineWidth += advance;
  }
  maxWidth = std::max(maxWidth, lineWidth);
  return maxWidth;
}

int estimate_wrapped_lines(PrimeFrame::Frame& frame,
                           PrimeFrame::TextStyleToken token,
                           std::string_view text,
                           float maxWidth,
                           PrimeFrame::WrapMode wrap) {
  if (text.empty()) {
    return 0;
  }

  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    int lines = 1;
    for (char ch : text) {
      if (ch == '\n') {
        ++lines;
      }
    }
    return lines;
  }

  float spaceWidth = measureTextWidth(frame, token, " ");
  float lineWidth = 0.0f;
  int lines = 1;
  std::string word;

  auto flush_word = [&]() {
    if (word.empty()) {
      return;
    }
    float wordWidth = measureTextWidth(frame, token, word);
    if (lineWidth > 0.0f && lineWidth + spaceWidth + wordWidth > maxWidth) {
      ++lines;
      lineWidth = wordWidth;
    } else {
      if (lineWidth > 0.0f) {
        lineWidth += spaceWidth;
      }
      lineWidth += wordWidth;
    }
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flush_word();
      ++lines;
      lineWidth = 0.0f;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      flush_word();
      continue;
    }
    word.push_back(ch);
  }
  flush_word();

  return lines;
}

std::vector<std::string> wrap_text_lines(PrimeFrame::Frame& frame,
                                         PrimeFrame::TextStyleToken token,
                                         std::string_view text,
                                         float maxWidth,
                                         PrimeFrame::WrapMode wrap) {
  std::vector<std::string> lines;
  if (text.empty()) {
    return lines;
  }

  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    std::string current;
    for (char ch : text) {
      if (ch == '\n') {
        lines.push_back(current);
        current.clear();
        continue;
      }
      current.push_back(ch);
    }
    if (!current.empty() || (!text.empty() && text.back() == '\n')) {
      lines.push_back(current);
    }
    return lines;
  }

  float spaceWidth = estimate_text_width(frame, token, " ");
  float lineWidth = 0.0f;
  std::string current;
  std::string word;
  bool wrapByChar = wrap == PrimeFrame::WrapMode::Character;

  auto flush_word = [&]() {
    if (word.empty()) {
      return;
    }
    float wordWidth = estimate_text_width(frame, token, word);
    if (!current.empty() && lineWidth + spaceWidth + wordWidth > maxWidth) {
      lines.push_back(current);
      current.clear();
      lineWidth = 0.0f;
    }
    if (!current.empty()) {
      current.push_back(' ');
      lineWidth += spaceWidth;
    }
    current += word;
    lineWidth += wordWidth;
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flush_word();
      lines.push_back(current);
      current.clear();
      lineWidth = 0.0f;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      flush_word();
      continue;
    }
    word.push_back(ch);
    if (wrapByChar) {
      flush_word();
    }
  }
  flush_word();
  if (!current.empty()) {
    lines.push_back(current);
  }

  return lines;
}

struct FlatTreeRow {
  std::string_view label;
  int depth = 0;
  int parentIndex = -1;
  bool hasChildren = false;
  bool expanded = true;
  bool selected = false;
  std::vector<int> ancestors;
  std::vector<uint32_t> path;
};

void flatten_tree(std::vector<TreeNode> const& nodes,
                  int depth,
                  std::vector<int>& depthStack,
                  std::vector<uint32_t>& pathStack,
                  std::vector<FlatTreeRow>& out) {
  for (size_t i = 0; i < nodes.size(); ++i) {
    TreeNode const& node = nodes[i];
    int parentIndex = depth > 0 && depth - 1 < static_cast<int>(depthStack.size())
                          ? depthStack[static_cast<size_t>(depth - 1)]
                          : -1;
    FlatTreeRow row;
    row.label = node.label;
    row.depth = depth;
    row.parentIndex = parentIndex;
    row.hasChildren = !node.children.empty();
    row.expanded = node.expanded;
    row.selected = node.selected;
    if (depth > 0 && depth <= static_cast<int>(depthStack.size())) {
      row.ancestors.assign(depthStack.begin(), depthStack.begin() + depth);
    }
    pathStack.push_back(static_cast<uint32_t>(i));
    row.path = pathStack;
    int index = static_cast<int>(out.size());
    out.push_back(std::move(row));

    if (depth >= static_cast<int>(depthStack.size())) {
      depthStack.resize(static_cast<size_t>(depth) + 1, -1);
    }
    depthStack[static_cast<size_t>(depth)] = index;

    if (node.expanded && !node.children.empty()) {
      flatten_tree(node.children, depth + 1, depthStack, pathStack, out);
    }
    pathStack.pop_back();
  }
}

void add_divider_rect(PrimeFrame::Frame& frame,
                      PrimeFrame::NodeId nodeId,
                      Rect const& bounds,
                      PrimeFrame::RectStyleToken token) {
  PrimeFrame::NodeId id = create_node(frame, nodeId, bounds,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      true);
  add_rect_primitive(frame, id, token, {});
}

} // namespace

namespace Studio {

PrimeFrame::RectStyleToken rectToken(RectRole role) {
  return static_cast<PrimeFrame::RectStyleToken>(role);
}

PrimeFrame::TextStyleToken textToken(TextRole role) {
  return static_cast<PrimeFrame::TextStyleToken>(role);
}

void applyStudioTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }
  theme->name = "Studio";
  theme->palette = {
      PrimeFrame::Color{0.07f, 0.09f, 0.13f, 1.0f}, // Background
      PrimeFrame::Color{0.12f, 0.15f, 0.22f, 1.0f}, // Topbar
      PrimeFrame::Color{0.10f, 0.12f, 0.18f, 1.0f}, // Statusbar
      PrimeFrame::Color{0.11f, 0.13f, 0.19f, 1.0f}, // Sidebar/Inspector
      PrimeFrame::Color{0.13f, 0.16f, 0.22f, 1.0f}, // Content
      PrimeFrame::Color{0.16f, 0.19f, 0.26f, 1.0f}, // Panel
      PrimeFrame::Color{0.18f, 0.21f, 0.30f, 1.0f}, // Panel Alt
      PrimeFrame::Color{0.22f, 0.26f, 0.36f, 1.0f}, // Panel Strong
      PrimeFrame::Color{0.35f, 0.64f, 1.0f, 1.0f},  // Accent
      PrimeFrame::Color{0.25f, 0.38f, 0.60f, 1.0f}, // Selection
      PrimeFrame::Color{0.10f, 0.12f, 0.16f, 1.0f}, // Scroll Track
      PrimeFrame::Color{0.24f, 0.28f, 0.39f, 1.0f}, // Scroll Thumb
      PrimeFrame::Color{0.08f, 0.10f, 0.15f, 1.0f}, // Divider
      PrimeFrame::Color{0.925f, 0.937f, 0.957f, 1.0f}, // Text Bright
      PrimeFrame::Color{0.588f, 0.620f, 0.667f, 1.0f}, // Text Muted
      PrimeFrame::Color{0.20f, 0.24f, 0.34f, 1.0f}, // Button Base
      PrimeFrame::Color{0.24f, 0.29f, 0.41f, 1.0f}, // Button Hover
      PrimeFrame::Color{0.16f, 0.20f, 0.30f, 1.0f}, // Button Pressed
      PrimeFrame::Color{0.35f, 0.64f, 1.0f, 1.0f},  // Button Primary
      PrimeFrame::Color{0.42f, 0.72f, 1.0f, 1.0f},  // Button Primary Hover
      PrimeFrame::Color{0.28f, 0.55f, 0.92f, 1.0f}  // Button Primary Pressed
  };
  theme->rectStyles = {
      PrimeFrame::RectStyle{0, 1.0f},
      PrimeFrame::RectStyle{1, 1.0f},
      PrimeFrame::RectStyle{2, 1.0f},
      PrimeFrame::RectStyle{3, 1.0f},
      PrimeFrame::RectStyle{4, 1.0f},
      PrimeFrame::RectStyle{3, 1.0f},
      PrimeFrame::RectStyle{5, 1.0f},
      PrimeFrame::RectStyle{6, 1.0f},
      PrimeFrame::RectStyle{7, 1.0f},
      PrimeFrame::RectStyle{8, 1.0f},
      PrimeFrame::RectStyle{9, 0.55f},
      PrimeFrame::RectStyle{10, 1.0f},
      PrimeFrame::RectStyle{11, 1.0f},
      PrimeFrame::RectStyle{12, 1.0f},
      PrimeFrame::RectStyle{15, 1.0f},
      PrimeFrame::RectStyle{16, 1.0f},
      PrimeFrame::RectStyle{17, 1.0f},
      PrimeFrame::RectStyle{18, 1.0f},
      PrimeFrame::RectStyle{19, 1.0f},
      PrimeFrame::RectStyle{20, 1.0f}
  };

  PrimeFrame::TextStyle titleText;
  titleText.size = 18.0f;
  titleText.weight = 600.0f;
  titleText.lineHeight = titleText.size * 1.2f;
  titleText.color = 13;
  PrimeFrame::TextStyle bodyText;
  bodyText.size = 14.0f;
  bodyText.weight = 500.0f;
  bodyText.lineHeight = bodyText.size * 1.4f;
  bodyText.color = 13;
  PrimeFrame::TextStyle bodyMuted = bodyText;
  bodyMuted.color = 14;
  PrimeFrame::TextStyle smallText;
  smallText.size = 12.0f;
  smallText.weight = 500.0f;
  smallText.lineHeight = smallText.size * 1.3f;
  smallText.color = 13;
  PrimeFrame::TextStyle smallMuted = smallText;
  smallMuted.color = 14;

  theme->textStyles = {titleText, bodyText, bodyMuted, smallText, smallMuted};
}

bool is_default_theme(PrimeFrame::Theme const& theme) {
  if (theme.name != "Default") {
    return false;
  }
  if (theme.palette.size() != 1 || theme.rectStyles.size() != 1 || theme.textStyles.size() != 1) {
    return false;
  }
  PrimeFrame::Color const& color = theme.palette[0];
  if (color.r != 0.0f || color.g != 0.0f || color.b != 0.0f || color.a != 1.0f) {
    return false;
  }
  PrimeFrame::RectStyle const& rect = theme.rectStyles[0];
  if (rect.fill != 0 || rect.opacity != 1.0f) {
    return false;
  }
  PrimeFrame::TextStyle const& text = theme.textStyles[0];
  if (text.font != 0 || text.size != 14.0f || text.weight != 400.0f || text.slant != 0.0f ||
      text.lineHeight != 0.0f || text.tracking != 0.0f || text.color != 0) {
    return false;
  }
  return true;
}

static void ensure_studio_theme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }
  if (theme->palette.empty() || theme->rectStyles.empty() || theme->textStyles.empty() ||
      is_default_theme(*theme)) {
    applyStudioTheme(frame);
  }
}

} // namespace Studio

namespace Studio {

UiNode createPanel(UiNode& parent, RectRole role, SizeSpec const& size) {
  return parent.createPanel(rectToken(role), size);
}

} // namespace Studio

namespace Studio {

UiNode createLabel(UiNode& parent, std::string_view text, TextRole role, SizeSpec const& size) {
  return parent.createLabel(text, textToken(role), size);
}

} // namespace Studio

namespace Studio {

UiNode createParagraph(UiNode& parent,
                       std::string_view text,
                       TextRole role,
                       SizeSpec const& size) {
  return parent.createParagraph(text, textToken(role), size);
}

} // namespace Studio

namespace Studio {

UiNode createTextLine(UiNode& parent,
                      std::string_view text,
                      TextRole role,
                      SizeSpec const& size,
                      PrimeFrame::TextAlign align) {
  return parent.createTextLine(text, textToken(role), size, align);
}

} // namespace Studio

namespace Studio {

UiNode createTable(UiNode& parent, TableSpec const& spec) {
  PrimeStage::TableSpec tableSpec = spec.base;
  tableSpec.headerStyle = rectToken(spec.headerRole);
  tableSpec.rowStyle = rectToken(spec.rowRole);
  tableSpec.rowAltStyle = rectToken(spec.rowAltRole);
  tableSpec.dividerStyle = rectToken(spec.dividerRole);

  if (!spec.columns.empty()) {
    tableSpec.columns.clear();
    tableSpec.columns.reserve(spec.columns.size());
    for (TableColumn const& col : spec.columns) {
      PrimeStage::TableColumn core;
      core.label = col.label;
      core.width = col.width;
      core.headerStyle = textToken(col.headerRole);
      core.cellStyle = textToken(col.cellRole);
      tableSpec.columns.push_back(core);
    }
  }

  return parent.createTable(tableSpec);
}

UiNode createTreeView(UiNode& parent, TreeViewSpec const& spec) {
  PrimeStage::TreeViewSpec treeSpec = spec.base;
  treeSpec.rowStyle = rectToken(spec.rowRole);
  treeSpec.rowAltStyle = rectToken(spec.rowAltRole);
  treeSpec.hoverStyle = rectToken(spec.hoverRole);
  treeSpec.selectionStyle = rectToken(spec.selectionRole);
  treeSpec.selectionAccentStyle = rectToken(spec.selectionAccentRole);
  treeSpec.caretBackgroundStyle = rectToken(spec.caretBackgroundRole);
  treeSpec.caretLineStyle = rectToken(spec.caretLineRole);
  treeSpec.connectorStyle = rectToken(spec.connectorRole);
  treeSpec.focusStyle = rectToken(spec.focusRole);
  if (!treeSpec.focusStyleOverride.opacity.has_value()) {
    treeSpec.focusStyleOverride.opacity = 0.18f;
  }
  treeSpec.textStyle = textToken(spec.textRole);
  treeSpec.selectedTextStyle = textToken(spec.selectedTextRole);

  if (treeSpec.scrollBar.trackStyle == 0) {
    treeSpec.scrollBar.trackStyle = rectToken(RectRole::ScrollTrack);
  }
  if (treeSpec.scrollBar.thumbStyle == 0) {
    treeSpec.scrollBar.thumbStyle = rectToken(RectRole::ScrollThumb);
  }

  return parent.createTreeView(treeSpec);
}

} // namespace Studio

namespace Studio {

UiNode createSectionHeader(UiNode& parent, SectionHeaderSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = StudioDefaults::SectionHeaderHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.title.empty()) {
    float textWidth = estimate_text_width(frame(), textToken(spec.textRole), spec.title);
    bounds.width = std::max(bounds.width, spec.textInsetX + textWidth);
  }
  SizeSpec headerSize = spec.size;
  if (!headerSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    headerSize.preferredWidth = bounds.width;
  }
  if (!headerSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    headerSize.preferredHeight = bounds.height;
  }

  PanelSpec panel;
  panel.size = headerSize;
  panel.rectStyle = rectToken(spec.backgroundRole);
  panel.layout = PrimeFrame::LayoutType::HorizontalStack;
  panel.gap = 0.0f;
  UiNode header = parent.createPanel(panel);

  if (spec.accentWidth > 0.0f) {
    PanelSpec accentPanel;
    accentPanel.rectStyle = rectToken(spec.accentRole);
    accentPanel.size.preferredWidth = spec.accentWidth;
    accentPanel.size.stretchY = 1.0f;
    header.createPanel(accentPanel);
  }

  if (spec.textInsetX > 0.0f) {
    SizeSpec spacer;
    spacer.preferredWidth = spec.textInsetX;
    spacer.preferredHeight = bounds.height;
    header.createSpacer(spacer);
  }

  if (!spec.title.empty()) {
    SizeSpec textSize;
    textSize.stretchX = 1.0f;
    textSize.preferredHeight = bounds.height;
    header.createTextLine(spec.title, textToken(spec.textRole), textSize, PrimeFrame::TextAlign::Start);
  }

  if (spec.textInsetX > 0.0f) {
    SizeSpec spacer;
    spacer.preferredWidth = spec.textInsetX;
    spacer.preferredHeight = bounds.height;
    header.createSpacer(spacer);
  }

  if (spec.addDivider) {
    if (spec.dividerOffsetY > 0.0f) {
      SizeSpec spacer;
      spacer.preferredHeight = spec.dividerOffsetY;
      parent.createSpacer(spacer);
    }
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.size.stretchX = 1.0f;
    divider.size.preferredHeight = 1.0f;
    parent.createDivider(divider);
  }

  return UiNode(frame(), header.nodeId(), allowAbsolute_);
}

UiNode createSectionHeader(UiNode& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role) {
  SectionHeaderSpec spec;
  spec.size = size;
  spec.title = title;
  spec.textRole = role;
  return createSectionHeader(parent, spec);
}

UiNode createSectionHeader(UiNode& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role,
                           bool addDivider,
                           float dividerOffsetY) {
  SectionHeaderSpec spec;
  spec.size = size;
  spec.title = title;
  spec.textRole = role;
  spec.addDivider = addDivider;
  spec.dividerOffsetY = dividerOffsetY;
  return createSectionHeader(parent, spec);
}

SectionPanel createSectionPanel(UiNode& parent, SectionPanelSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.title.empty()) {
    float textWidth = estimate_text_width(frame(), textToken(spec.textRole), spec.title);
    bounds.width = spec.headerInsetX + spec.headerInsetRight + spec.headerPaddingX * 2.0f + textWidth;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = spec.headerInsetY + spec.headerHeight + spec.contentInsetY + spec.contentInsetBottom;
  }
  SizeSpec panelSize = spec.size;
  if (!panelSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    panelSize.preferredWidth = bounds.width;
  }
  if (!panelSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    panelSize.preferredHeight = bounds.height;
  }

  PanelSpec panel;
  panel.size = panelSize;
  panel.rectStyle = rectToken(spec.panelRole);
  panel.layout = PrimeFrame::LayoutType::VerticalStack;
  panel.gap = 0.0f;
  UiNode panelNode = parent.createPanel(panel);
  SectionPanel out{UiNode(frame(), panelNode.nodeId(), allowAbsolute_),
                   UiNode(frame(), PrimeFrame::NodeId{}, allowAbsolute_)};

  float headerWidth = std::max(0.0f, bounds.width - spec.headerInsetX - spec.headerInsetRight);

  if (spec.headerInsetY > 0.0f) {
    SizeSpec spacer;
    spacer.preferredHeight = spec.headerInsetY;
    panelNode.createSpacer(spacer);
  }

  if (spec.headerHeight > 0.0f) {
    StackSpec headerInset;
    headerInset.size.stretchX = 1.0f;
    headerInset.size.preferredHeight = spec.headerHeight;
    headerInset.clipChildren = false;
    headerInset.padding.left = spec.headerInsetX;
    headerInset.padding.right = spec.headerInsetRight;
    UiNode headerInsetNode = panelNode.createOverlay(headerInset);

    PanelSpec headerPanel;
    headerPanel.rectStyle = rectToken(spec.headerRole);
    headerPanel.layout = PrimeFrame::LayoutType::HorizontalStack;
    headerPanel.size.stretchX = 1.0f;
    headerPanel.size.stretchY = 1.0f;
    if (headerWidth > 0.0f) {
      headerPanel.size.preferredWidth = headerWidth;
    }
    headerPanel.gap = 0.0f;
    headerPanel.clipChildren = false;
    UiNode headerRow = headerInsetNode.createPanel(headerPanel);

    if (spec.showAccent && spec.accentWidth > 0.0f) {
      PanelSpec accentPanel;
      accentPanel.rectStyle = rectToken(spec.accentRole);
      accentPanel.size.preferredWidth = spec.accentWidth;
      accentPanel.size.stretchY = 1.0f;
      headerRow.createPanel(accentPanel);
    }

    if (spec.headerPaddingX > 0.0f) {
      SizeSpec spacer;
      spacer.preferredWidth = spec.headerPaddingX;
      spacer.preferredHeight = spec.headerHeight;
      headerRow.createSpacer(spacer);
    }

    if (!spec.title.empty()) {
      SizeSpec titleSize;
      titleSize.stretchX = 1.0f;
      titleSize.preferredHeight = spec.headerHeight;
      headerRow.createTextLine(spec.title, textToken(spec.textRole), titleSize, PrimeFrame::TextAlign::Start);
    }

    if (spec.headerPaddingX > 0.0f) {
      SizeSpec spacer;
      spacer.preferredWidth = spec.headerPaddingX;
      spacer.preferredHeight = spec.headerHeight;
      headerRow.createSpacer(spacer);
    }
  }

  if (spec.contentInsetY > 0.0f) {
    SizeSpec spacer;
    spacer.preferredHeight = spec.contentInsetY;
    panelNode.createSpacer(spacer);
  }

  float contentY = spec.headerInsetY + spec.headerHeight + spec.contentInsetY;
  float contentW = std::max(0.0f, bounds.width - spec.contentInsetX - spec.contentInsetRight);
  float contentH = std::max(0.0f, bounds.height - (contentY + spec.contentInsetBottom));

  StackSpec contentInset;
  contentInset.size.stretchX = 1.0f;
  contentInset.size.stretchY = panelSize.stretchY > 0.0f ? 1.0f : 0.0f;
  if (contentH > 0.0f) {
    contentInset.size.preferredHeight = contentH;
  }
  contentInset.clipChildren = false;
  contentInset.padding.left = spec.contentInsetX;
  contentInset.padding.right = spec.contentInsetRight;
  UiNode contentInsetNode = panelNode.createOverlay(contentInset);

  SizeSpec contentSize;
  contentSize.stretchX = 1.0f;
  contentSize.stretchY = contentInset.size.stretchY;
  if (contentW > 0.0f) {
    contentSize.preferredWidth = contentW;
  }
  if (contentH > 0.0f) {
    contentSize.preferredHeight = contentH;
  }
  PrimeFrame::NodeId contentId = create_node(frame(), contentInsetNode.nodeId(), Rect{},
                                             &contentSize,
                                             PrimeFrame::LayoutType::None,
                                             PrimeFrame::Insets{},
                                             0.0f,
                                             false,
                                             true);
  out.content = UiNode(frame(), contentId, allowAbsolute_);

  if (spec.contentInsetBottom > 0.0f) {
    SizeSpec spacer;
    spacer.preferredHeight = spec.contentInsetBottom;
    panelNode.createSpacer(spacer);
  }

  return out;
}

SectionPanel createSectionPanel(UiNode& parent, SizeSpec const& size, std::string_view title) {
  SectionPanelSpec spec;
  spec.size = size;
  spec.title = title;
  return createSectionPanel(parent, spec);
}

UiNode createPropertyList(UiNode const& parent, PropertyListSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float maxWidth = 0.0f;
    float labelInset = spec.labelInsetX;
    float valueInset = spec.valueInsetX;
    float valuePadding = spec.valuePaddingRight;
    for (PropertyRow const& row : spec.rows) {
      float labelWidth = estimate_text_width(frame(), textToken(spec.labelRole), row.label);
      float valueWidth = estimate_text_width(frame(), textToken(spec.valueRole), row.value);
      if (spec.valueAlignRight) {
        maxWidth = std::max(maxWidth, labelInset + labelWidth + valuePadding + valueWidth);
      } else {
        maxWidth = std::max(maxWidth, std::max(labelInset + labelWidth, valueInset + valueWidth));
      }
    }
    bounds.width = maxWidth;
  }
  if (bounds.height <= 0.0f &&
      !spec.rows.empty() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = static_cast<float>(spec.rows.size()) * spec.rowHeight +
                    static_cast<float>(spec.rows.size() - 1) * spec.rowGap;
  }
  SizeSpec listSize = spec.size;
  if (!listSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    listSize.preferredWidth = bounds.width;
  }
  if (!listSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    listSize.preferredHeight = bounds.height;
  }

  StackSpec listSpec;
  listSpec.size = listSize;
  listSpec.gap = spec.rowGap;
  listSpec.clipChildren = false;
  UiNode parentNode(frame(), parent.nodeId(), allowAbsolute_);
  UiNode listNode = parentNode.createVerticalStack(listSpec);

  float maxLabelWidth = 0.0f;
  float maxValueWidth = 0.0f;
  for (PropertyRow const& row : spec.rows) {
    maxLabelWidth = std::max(maxLabelWidth,
                             estimate_text_width(frame(), textToken(spec.labelRole), row.label));
    maxValueWidth = std::max(maxValueWidth,
                             estimate_text_width(frame(), textToken(spec.valueRole), row.value));
  }

  float rowWidth = bounds.width > 0.0f ? bounds.width : listSize.preferredWidth.value_or(0.0f);
  float labelCellWidth = std::max(0.0f, spec.labelInsetX + maxLabelWidth);
  float valueCellWidth = std::max(0.0f, spec.valueInsetX + maxValueWidth + spec.valuePaddingRight);
  if (rowWidth > 0.0f && spec.valueAlignRight) {
    valueCellWidth = std::max(valueCellWidth, rowWidth - labelCellWidth);
  }

  auto create_cell = [&](UiNode const& rowNode,
                         float width,
                         float height,
                         float paddingLeft,
                         float paddingRight,
                         std::string_view text,
                         TextRole role,
                         PrimeFrame::TextAlign align) {
    SizeSpec cellSize;
    if (width > 0.0f) {
      cellSize.preferredWidth = width;
    }
    if (height > 0.0f) {
      cellSize.preferredHeight = height;
    }
    PrimeFrame::Insets padding{};
    padding.left = paddingLeft;
    padding.right = paddingRight;
    PrimeFrame::NodeId cellId = create_node(frame(), rowNode.nodeId(), Rect{},
                                            &cellSize,
                                            PrimeFrame::LayoutType::Overlay,
                                            padding,
                                            0.0f,
                                            false,
                                            true);
    if (PrimeFrame::Node* cellNode = frame().getNode(cellId)) {
      cellNode->hitTestVisible = false;
    }
    UiNode cell(frame(), cellId, rowNode.allowAbsolute());
    SizeSpec textSize;
    float innerWidth = width - paddingLeft - paddingRight;
    if (innerWidth > 0.0f) {
      textSize.preferredWidth = innerWidth;
    }
    if (height > 0.0f) {
      textSize.preferredHeight = height;
    }
    cell.createTextLine(text, textToken(role), textSize, align);
  };

  for (PropertyRow const& row : spec.rows) {
    StackSpec rowSpec;
    rowSpec.size.preferredHeight = spec.rowHeight;
    rowSpec.size.stretchX = 1.0f;
    rowSpec.gap = 0.0f;
    rowSpec.clipChildren = false;
    UiNode rowNode = listNode.createHorizontalStack(rowSpec);

    create_cell(rowNode,
                labelCellWidth,
                spec.rowHeight,
                spec.labelInsetX,
                0.0f,
                row.label,
                spec.labelRole,
                PrimeFrame::TextAlign::Start);

    float valueWidth = estimate_text_width(frame(), textToken(spec.valueRole), row.value);
    SizeSpec valueSize;
    valueSize.preferredWidth = valueCellWidth;
    valueSize.preferredHeight = spec.rowHeight;
    if (spec.valueAlignRight) {
      valueSize.stretchX = 1.0f;
    }

    PrimeFrame::Insets valuePadding{};
    valuePadding.left = spec.valueInsetX;
    valuePadding.right = spec.valuePaddingRight;
    PrimeFrame::NodeId valueId = create_node(frame(), rowNode.nodeId(), Rect{},
                                             &valueSize,
                                             PrimeFrame::LayoutType::HorizontalStack,
                                             valuePadding,
                                             0.0f,
                                             false,
                                             true);
    if (PrimeFrame::Node* valueNode = frame().getNode(valueId)) {
      valueNode->hitTestVisible = false;
    }
    UiNode valueCell(frame(), valueId, rowNode.allowAbsolute());

    if (spec.valueAlignRight) {
      SizeSpec spacer;
      spacer.stretchX = 1.0f;
      valueCell.createSpacer(spacer);
    }

    SizeSpec textSize;
    textSize.preferredWidth = valueWidth;
    textSize.preferredHeight = spec.rowHeight;
    valueCell.createTextLine(row.value, textToken(spec.valueRole), textSize, PrimeFrame::TextAlign::Start);
  }

  return UiNode(frame(), listNode.nodeId(), allowAbsolute_);
}

UiNode createPropertyList(UiNode const& parent, SizeSpec const& size, std::vector<PropertyRow> rows) {
  PropertyListSpec spec;
  spec.size = size;
  spec.rows = std::move(rows);
  return createPropertyList(parent, spec);
}

UiNode createPropertyRow(UiNode const& parent,
                         SizeSpec const& size,
                         std::string_view label,
                         std::string_view value,
                         TextRole role) {
  PropertyListSpec spec;
  spec.size = size;
  spec.labelRole = role;
  spec.valueRole = role;
  spec.rows = {{label, value}};
  return createPropertyList(parent, spec);
}

UiNode createProgressBar(UiNode& parent, ProgressBarSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  Rect bounds = resolve_rect(spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    if (bounds.width <= 0.0f) {
      bounds.width = StudioDefaults::ControlWidthL;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = StudioDefaults::OpacityBarHeight;
    }
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = rectToken(spec.trackRole);
  UiNode bar = parent.createPanel(panel);
  float clamped = std::clamp(spec.value, 0.0f, 1.0f);
  float fillWidth = bounds.width * clamped;
  if (spec.minFillWidth > 0.0f && clamped > 0.0f) {
    fillWidth = std::max(fillWidth, spec.minFillWidth);
  }
  if (fillWidth > 0.0f) {
    create_rect_node(frame(),
                     bar.nodeId(),
                     Rect{0.0f, 0.0f, fillWidth, bounds.height},
                     rectToken(spec.fillRole),
                     {},
                     false,
                     true);
  }
  return bar;
}

UiNode createProgressBar(UiNode& parent, SizeSpec const& size, float value) {
  ProgressBarSpec spec;
  spec.size = size;
  spec.value = value;
  return createProgressBar(parent, spec);
}

UiNode createCardGrid(UiNode& parent, CardGridSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f &&
      !spec.cards.empty()) {
    if (bounds.width <= 0.0f) {
      bounds.width = spec.cardWidth;
    }
    if (bounds.height <= 0.0f) {
      size_t rows = spec.cards.size();
      if (rows > 1 && spec.gapY > 0.0f) {
        bounds.height = static_cast<float>(rows) * spec.cardHeight +
                        static_cast<float>(rows - 1) * spec.gapY;
      } else {
        bounds.height = static_cast<float>(rows) * spec.cardHeight;
      }
    }
  }
  SizeSpec gridSize = spec.size;
  if (!gridSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    gridSize.preferredWidth = bounds.width;
  }
  if (!gridSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    gridSize.preferredHeight = bounds.height;
  }

  StackSpec gridSpec;
  gridSpec.size = gridSize;
  gridSpec.gap = spec.gapY;
  gridSpec.clipChildren = true;
  UiNode gridNode = parent.createVerticalStack(gridSpec);

  if (spec.cards.empty()) {
    return UiNode(frame(), gridNode.nodeId(), allowAbsolute_);
  }

  size_t columns = 1;
  float gridWidth = bounds.width > 0.0f ? bounds.width : gridSize.preferredWidth.value_or(0.0f);
  if (spec.cardWidth + spec.gapX > 0.0f && gridWidth > 0.0f) {
    float stride = spec.cardWidth + spec.gapX;
    float ratio = (gridWidth + spec.gapX) / stride;
    constexpr float kColumnEpsilon = 1e-3f;
    columns = std::max<size_t>(1, static_cast<size_t>(std::floor(ratio + kColumnEpsilon)));
  }

  float titleLine = resolve_line_height(frame(), textToken(spec.titleRole));
  float subtitleLine = resolve_line_height(frame(), textToken(spec.subtitleRole));

  size_t rowCount = (spec.cards.size() + columns - 1) / columns;
  for (size_t row = 0; row < rowCount; ++row) {
    StackSpec rowSpec;
    rowSpec.size.preferredHeight = spec.cardHeight;
    rowSpec.size.stretchX = 1.0f;
    rowSpec.gap = spec.gapX;
    rowSpec.clipChildren = false;
    UiNode rowNode = gridNode.createHorizontalStack(rowSpec);

    for (size_t col = 0; col < columns; ++col) {
      size_t index = row * columns + col;
      if (index >= spec.cards.size()) {
        break;
      }
      PanelSpec cardPanel;
      cardPanel.rectStyle = rectToken(spec.cardRole);
      cardPanel.size.preferredWidth = spec.cardWidth;
      cardPanel.size.preferredHeight = spec.cardHeight;
      UiNode cardNode = rowNode.createPanel(cardPanel);
      UiNode cardInternal(frame(), cardNode.nodeId(), true);

      CardSpec const& card = spec.cards[index];
      if (!card.title.empty()) {
        Rect titleRect;
        titleRect.x = spec.paddingX;
        titleRect.y = spec.titleOffsetY;
        titleRect.width = std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f);
        titleRect.height = titleLine;
        create_text_node(frame(),
                         cardInternal.nodeId(),
                         titleRect,
                         card.title,
                         textToken(spec.titleRole),
                         {},
                         PrimeFrame::TextAlign::Start,
                         PrimeFrame::WrapMode::None,
                         titleRect.width,
                         true);
      }
      if (!card.subtitle.empty()) {
        Rect subtitleRect;
        subtitleRect.x = spec.paddingX;
        subtitleRect.y = spec.subtitleOffsetY;
        subtitleRect.width = std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f);
        subtitleRect.height = subtitleLine;
        create_text_node(frame(),
                         cardInternal.nodeId(),
                         subtitleRect,
                         card.subtitle,
                         textToken(spec.subtitleRole),
                         {},
                         PrimeFrame::TextAlign::Start,
                         PrimeFrame::WrapMode::None,
                         subtitleRect.width,
                         true);
      }
    }
  }

  return UiNode(frame(), gridNode.nodeId(), allowAbsolute_);
}

} // namespace Studio

namespace Studio {

UiNode createButton(UiNode& parent,
                    std::string_view label,
                    ButtonVariant variant,
                    SizeSpec const& size,
                    ButtonCallbacks const& callbacks) {
  ButtonSpec spec;
  spec.label = label;
  spec.size = size;
  if (!spec.size.preferredHeight.has_value() && spec.size.stretchY <= 0.0f) {
    spec.size.preferredHeight = StudioDefaults::ControlHeight;
  }
  if (spec.label.empty() &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    spec.size.preferredWidth = StudioDefaults::ControlWidthM;
  }
  if (variant == ButtonVariant::Primary) {
    spec.backgroundStyle = rectToken(RectRole::ButtonPrimary);
    spec.hoverStyle = rectToken(RectRole::ButtonPrimaryHover);
    spec.pressedStyle = rectToken(RectRole::ButtonPrimaryPressed);
    spec.baseOpacity = 1.0f;
    spec.hoverOpacity = 1.0f;
    spec.pressedOpacity = 1.0f;
    spec.textStyle = textToken(TextRole::BodyBright);
  } else {
    spec.backgroundStyle = rectToken(RectRole::ButtonBase);
    spec.hoverStyle = rectToken(RectRole::ButtonHover);
    spec.pressedStyle = rectToken(RectRole::ButtonPressed);
    spec.baseOpacity = 1.0f;
    spec.hoverOpacity = 1.0f;
    spec.pressedOpacity = 1.0f;
    spec.textStyle = textToken(TextRole::BodyBright);
  }
  spec.focusStyle = rectToken(RectRole::Accent);
  spec.focusStyleOverride.opacity = 0.25f;
  spec.callbacks = callbacks;
  return parent.createButton(spec);
}

} // namespace Studio

namespace Studio {

UiNode createTextField(UiNode& parent, std::string_view placeholder, SizeSpec const& size) {
  TextFieldSpec spec;
  spec.placeholder = placeholder;
  spec.size = size;
  if (!spec.size.preferredHeight.has_value() && spec.size.stretchY <= 0.0f) {
    spec.size.preferredHeight = StudioDefaults::ControlHeight;
  }
  if (!spec.size.minWidth.has_value() && spec.size.stretchX <= 0.0f) {
    spec.size.minWidth = StudioDefaults::FieldWidthL;
  }
  spec.backgroundStyle = rectToken(RectRole::Panel);
  spec.focusStyle = rectToken(RectRole::Accent);
  spec.focusStyleOverride.opacity = 0.22f;
  spec.textStyle = textToken(TextRole::BodyBright);
  spec.placeholderStyle = textToken(TextRole::BodyMuted);
  return parent.createTextField(spec);
}

UiNode createStatusBar(UiNode& parent, StatusBarSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  PrimeFrame::NodeId id_ = parent.nodeId();
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = StudioDefaults::StatusHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float leftWidth = spec.leftText.empty()
                          ? 0.0f
                          : estimate_text_width(frame(), textToken(spec.leftRole), spec.leftText);
    float rightWidth = spec.rightText.empty()
                           ? 0.0f
                           : estimate_text_width(frame(), textToken(spec.rightRole), spec.rightText);
    float spacing = spec.rightText.empty() ? 0.0f : spec.paddingX;
    bounds.width = spec.paddingX * 2.0f + leftWidth + spacing + rightWidth;
  }
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  PanelSpec panelSpec;
  panelSpec.rectStyle = rectToken(spec.backgroundRole);
  panelSpec.size = spec.size;
  if (!panelSpec.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panelSpec.size.preferredWidth = bounds.width;
  }
  if (!panelSpec.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panelSpec.size.preferredHeight = bounds.height;
  }
  UiNode bar = parent.createPanel(panelSpec);

  float leftLineHeight = resolve_line_height(frame(), textToken(spec.leftRole));
  float rightLineHeight = resolve_line_height(frame(), textToken(spec.rightRole));

  float leftY = (bounds.height - leftLineHeight) * 0.5f;
  float rightY = (bounds.height - rightLineHeight) * 0.5f;

  float rightWidth = 0.0f;
  if (!spec.rightText.empty()) {
    rightWidth = estimate_text_width(frame(), textToken(spec.rightRole), spec.rightText);
    float maxRightWidth = std::max(0.0f, bounds.width - spec.paddingX);
    rightWidth = std::min(rightWidth, maxRightWidth);
  }

  float rightX = bounds.width - spec.paddingX - rightWidth;
  if (rightX < 0.0f) {
    rightX = 0.0f;
  }

  float leftMaxWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);
  if (!spec.rightText.empty()) {
    leftMaxWidth = std::min(leftMaxWidth, std::max(0.0f, rightX - spec.paddingX));
  }

  if (!spec.leftText.empty()) {
    Rect leftRect{spec.paddingX, leftY, leftMaxWidth, leftLineHeight};
    create_text_node(frame(),
                     bar.nodeId(),
                     leftRect,
                     spec.leftText,
                     textToken(spec.leftRole),
                     {},
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     leftMaxWidth,
                     true);
  }

  if (!spec.rightText.empty()) {
    Rect rightRect{rightX, rightY, rightWidth, rightLineHeight};
    create_text_node(frame(),
                     bar.nodeId(),
                     rightRect,
                     spec.rightText,
                     textToken(spec.rightRole),
                     {},
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     rightWidth,
                     true);
  }

  return UiNode(frame(), bar.nodeId(), allowAbsolute_);
}

UiNode createStatusBar(UiNode& parent,
                       SizeSpec const& size,
                       std::string_view leftText,
                       std::string_view rightText) {
  StatusBarSpec spec;
  spec.size = size;
  spec.leftText = leftText;
  spec.rightText = rightText;
  return createStatusBar(parent, spec);
}

} // namespace Studio

namespace Studio {

UiNode createScrollHints(UiNode& parent, ScrollHintsSpec const& spec) {
  ScrollViewSpec view;
  view.size = spec.size;
  if (!view.size.preferredWidth.has_value() &&
      !view.size.preferredHeight.has_value() &&
      view.size.stretchX <= 0.0f &&
      view.size.stretchY <= 0.0f) {
    view.size.preferredWidth = StudioDefaults::FieldWidthL;
    view.size.preferredHeight = StudioDefaults::PanelHeightM;
  }
  view.showVertical = spec.showVertical;
  view.showHorizontal = spec.showHorizontal;
  view.vertical.trackStyle = rectToken(RectRole::ScrollTrack);
  view.vertical.thumbStyle = rectToken(RectRole::ScrollThumb);
  view.vertical.thumbLength = spec.verticalThumbLength;
  view.vertical.thumbOffset = spec.verticalThumbOffset;
  view.horizontal.trackStyle = rectToken(RectRole::ScrollTrack);
  view.horizontal.thumbStyle = rectToken(RectRole::ScrollThumb);
  view.horizontal.thumbLength = spec.horizontalThumbLength;
  view.horizontal.thumbOffset = spec.horizontalThumbOffset;
  view.horizontal.startPadding = spec.horizontalStartPadding;
  view.horizontal.endPadding = spec.horizontalEndPadding;
  return parent.createScrollView(view).root;
}

} // namespace Studio

namespace Studio {

UiNode createRoot(PrimeFrame::Frame& frame, SizeSpec const& size) {
  Rect rootBounds = resolve_rect(size);
  PrimeFrame::NodeId id = create_node(frame, PrimeFrame::NodeId{}, rootBounds,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      true);
  return UiNode(frame, id);
}

UiNode createStudioRoot(PrimeFrame::Frame& frame, SizeSpec const& size) {
  ensure_studio_theme(frame);
  return createRoot(frame, size);
}

ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec) {
  ensure_studio_theme(frame);
  Rect shellBounds = resolve_rect(spec.size);
  float width = shellBounds.width;
  float height = shellBounds.height;
  float contentH = std::max(0.0f, height - spec.topbarHeight - spec.statusHeight);
  float contentW = std::max(0.0f, width - spec.sidebarWidth - spec.inspectorWidth);
  SizeSpec shellSize = spec.size;
  if (!shellSize.preferredWidth.has_value() && width > 0.0f) {
    shellSize.preferredWidth = width;
  }
  if (!shellSize.preferredHeight.has_value() && height > 0.0f) {
    shellSize.preferredHeight = height;
  }
  UiNode root = createRoot(frame, shellSize);
  StackSpec overlaySpec;
  overlaySpec.size.preferredWidth = width;
  overlaySpec.size.preferredHeight = height;
  overlaySpec.clipChildren = false;
  UiNode overlay = root.createOverlay(overlaySpec);

  SizeSpec fullSize;
  fullSize.preferredWidth = width;
  fullSize.preferredHeight = height;

  UiNode background = overlay.createPanel(rectToken(spec.backgroundRole), fullSize);

  StackSpec shellSpec;
  shellSpec.size = fullSize;
  shellSpec.gap = 0.0f;
  shellSpec.clipChildren = false;
  UiNode shellStack = overlay.createVerticalStack(shellSpec);

  PanelSpec topbarSpec;
  topbarSpec.rectStyle = rectToken(spec.topbarRole);
  topbarSpec.size.preferredHeight = spec.topbarHeight;
  UiNode topbar = shellStack.createPanel(topbarSpec);

  StackSpec bodySpec;
  bodySpec.size.stretchY = 1.0f;
  bodySpec.size.stretchX = 1.0f;
  bodySpec.gap = 0.0f;
  bodySpec.clipChildren = false;
  UiNode bodyRow = shellStack.createHorizontalStack(bodySpec);

  PanelSpec sidebarSpec;
  sidebarSpec.rectStyle = rectToken(spec.sidebarRole);
  sidebarSpec.size.preferredWidth = spec.sidebarWidth;
  UiNode sidebar = bodyRow.createPanel(sidebarSpec);

  PanelSpec contentSpec;
  contentSpec.rectStyle = rectToken(spec.contentRole);
  contentSpec.size.stretchX = 1.0f;
  UiNode content = bodyRow.createPanel(contentSpec);

  PanelSpec inspectorSpec;
  inspectorSpec.rectStyle = rectToken(spec.inspectorRole);
  inspectorSpec.size.preferredWidth = spec.inspectorWidth;
  UiNode inspector = bodyRow.createPanel(inspectorSpec);

  PanelSpec statusSpec;
  statusSpec.rectStyle = rectToken(spec.statusRole);
  statusSpec.size.preferredHeight = spec.statusHeight;
  UiNode status = shellStack.createPanel(statusSpec);

  if (spec.drawDividers) {
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{0.0f, spec.topbarHeight, width, 1.0f},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{spec.sidebarWidth - 1.0f, spec.topbarHeight, 1.0f, contentH},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{width - spec.inspectorWidth, spec.topbarHeight, 1.0f, contentH},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{0.0f, height - spec.statusHeight, width, 1.0f},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
  }

  return ShellLayout{
      root,
      background,
      topbar,
      status,
      sidebar,
      content,
      inspector
  };
}

ShellSpec makeShellSpec(SizeSpec const& size) {
  ShellSpec spec;
  spec.size = size;
  return spec;
}

} // namespace Studio

} // namespace PrimeStage

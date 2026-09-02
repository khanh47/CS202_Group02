#include "Button/SettingsPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <utility>

#include "Audio/MusicManager.h"
#include "Audio/SoundManager.h"
#include "Commands/FunctionalCommand.h"
#include "Commands/ToggleDebugCommands.h"
#include "Game/GameSettings.h"
#include "ResourceManager.h"
#include "UI/UIHelpers.h"

namespace UI {
namespace {
struct KeybindDescriptor {
    const char* label;
    ActionType action;
};

constexpr std::array<KeybindDescriptor, 6> keybinds{{
    {"Move Left", ActionType::MoveLeft},
    {"Move Right", ActionType::MoveRight},
    {"Jump", ActionType::MoveUp},
    {"Pipe Down", ActionType::MoveDown},
    {"Shoot", ActionType::Attack},
    {"Button", ActionType::Interact}
}};

constexpr float kKbBaseY = 330.f;
constexpr float kKbDeltaY = 20.f;
constexpr float kKbVpY = kKbBaseY + kKbDeltaY;
constexpr float kKbVpH = 460.f;
constexpr float kKbGapP2 = 50.f;
constexpr float kPanelX = 445.f, kPanelY = 230.f, kPanelW = 1030.f, kPanelH = 620.f;
constexpr float kTopX = 515.f, kTopW = 890.f;
constexpr float kHeaderH = 34.f, kSpacing = 65.f, kRowH = 54.f;
inline float kbContentH() { return kHeaderH + 6*kSpacing + kKbGapP2 + kHeaderH + 6*kSpacing; }
inline float kbMaxScroll() { return std::max(0.f, kbContentH() - kKbVpH + 20.f); }
inline float kbRowOffset(int idx) {
    return (idx < 6) ? kHeaderH + 8.f + idx*kSpacing
                     : kHeaderH + 8.f + 6*kSpacing + kKbGapP2 + kHeaderH + (idx-6)*kSpacing;
}
}

SettingsPanel::SettingsPanel(BackCallback onBack)
    : _onBack(std::move(onBack)) {
    buildTopMenu();
    setActiveTab(SettingsTab::Music);
}

void SettingsPanel::setOnBack(BackCallback onBack) {
    _onBack = std::move(onBack);
}

void SettingsPanel::refresh() {
    _rebindingAction.reset();
    _activeRebindingButton.reset();
    setActiveTab(_activeTab);
}

void SettingsPanel::buildTopMenu() {
    _topMenu.clear();
    _topMenu.setLayoutProperties(
        {kTopX, 265.f}, {200.f, 60.f}, 230.f, true,
        sf::Color(70, 130, 180), 26
    );
    _topMenu.addButtonAuto("Audio", std::make_unique<FunctionalCommand>(
        "TabMusic", [this]() { setActiveTab(SettingsTab::Music); }
    ));
    _topMenu.addButtonAuto("Keybind", std::make_unique<FunctionalCommand>(
        "TabKeybind", [this]() { setActiveTab(SettingsTab::Keybind); }
    ));
    _topMenu.addButtonAuto("Dev Mode", std::make_unique<FunctionalCommand>(
        "TabDevMode", [this]() { setActiveTab(SettingsTab::DevMode); }
    ));
    _topMenu.addButtonAuto(
        "Back", 26,
        std::make_unique<FunctionalCommand>("Back", [this]() { goBack(); }),
        sf::Color(180, 80, 80)
    );
}

void SettingsPanel::buildMusicPanel() {
    const sf::Vector2f start{kTopX, 400.f};
    const sf::Vector2f size{kTopW, 60.f};
    constexpr float spacing = 82.f;
    const sf::Color color(70, 130, 180);
    _subMenu.setLayoutProperties(start, size, spacing, false, color, 24);

    auto& settings = GameSettings::getInstance();
    auto makeToggle = [&](sf::Vector2f pos, const char* label, bool state, auto cb) {
        auto t = std::make_shared<ToggleButton>(pos, size, color, label, 24, state, 20.f);
        t->setToggleCallback(cb);
        _subMenu.addButton(t);
        return t;
    };
    auto makeVolSlider = [&](sf::Vector2f pos, const char* label, float vol, auto cb) {
        auto s = std::make_shared<BarSlider>(pos, size, color, label, 24, vol, 0.f, 100.f, false, 20.f);
        s->setValueCallback(cb);
        _subMenu.addButton(s);
        return s;
    };
    makeToggle(start, "Music Enabled", settings.musicEnabled, [](bool v){
        auto& c = GameSettings::getInstance(); c.musicEnabled=v;
        Audio::MusicManager::getInstance().setEnabled(v); c.save();
    });
    _musicVolume = makeVolSlider({start.x, start.y + spacing}, "Music Volume", settings.musicVolume, [this](float v){
        auto& c = GameSettings::getInstance(); c.musicVolume = std::clamp(v,0.f,100.f);
        Audio::MusicManager::getInstance().setVolume(c.musicVolume); c.save(); updateVolumeLabels();
    });
    makeToggle({start.x, start.y + spacing*2.f}, "Sound Enabled", settings.soundEnabled, [](bool v){
        auto& c = GameSettings::getInstance(); c.soundEnabled=v;
        Audio::SoundManager::getInstance().setEnabled(v); c.save();
    });
    _soundVolume = makeVolSlider({start.x, start.y + spacing*3.f}, "Sound Volume", settings.soundVolume, [this](float v){
        auto& c = GameSettings::getInstance(); c.soundVolume = std::clamp(v,0.f,100.f);
        Audio::SoundManager::getInstance().setGlobalVolume(c.soundVolume); c.save(); updateVolumeLabels();
    });
    updateVolumeLabels();
}

void SettingsPanel::buildKeybindPanel() {
    const sf::Vector2f size{kTopW, kRowH};
    const sf::Color color(70, 130, 180);
    _subMenu.clear();
    _subMenu.setLayoutProperties({kTopX, 370.f}, size, kSpacing, false, color, 22);

    _keybindButtons.clear();
    _keybindPlayer.clear();
    _keybindAction.clear();
    _keybindButtons.reserve(12);
    _keybindPlayer.reserve(12);
    _keybindAction.reserve(12);

    auto addRow = [&](ActionType action, int player) {
        auto button = std::make_shared<Button>(
            sf::Vector2f(kTopX, 370.f), size, color, "", 22, 18.f
        );
        const std::weak_ptr<Button> weakButton = button;
        button->setCommand(std::make_unique<FunctionalCommand>(
            std::string("Rebind") + std::to_string(static_cast<int>(action)) + "P" + std::to_string(player),
            [this, action, player, weakButton]() {
                startRebinding(action, player, weakButton.lock());
            }
        ));
        _keybindButtons.push_back(button);
        _keybindPlayer.push_back(player);
        _keybindAction.push_back(action);
        _subMenu.addButton(button);
    };

    for (std::size_t i = 0; i < keybinds.size(); ++i) addRow(keybinds[i].action, 1);
    for (std::size_t i = 0; i < keybinds.size(); ++i) addRow(keybinds[i].action, 2);

    _keybindScroll = 0.f;
    _subMenu.setFocusedIndex(0);
    _focus = FocusTarget::SubMenu;
    updateKeybindButtonTexts();
    applyKeybindScroll();
}

void SettingsPanel::buildDevModePanel() {
    _subMenu.setLayoutProperties(
        {kTopX, 400.f}, {kTopW, 60.f}, 82.f, false,
        sf::Color(100, 149, 237), 24
    );
    const auto& settings = GameSettings::getInstance();
    _subMenu.addToggleButtonAuto(
        "Grid", settings.debugDrawGrid, std::make_unique<ToggleGridCommand>()
    );
    _subMenu.addToggleButtonAuto(
        "Coordinates", settings.debugDrawCoordinates,
        std::make_unique<ToggleCoordinatesCommand>()
    );
    _subMenu.addToggleButtonAuto(
        "Hitbox", settings.debugDrawHitbox, std::make_unique<ToggleHitboxCommand>()
    );
    _subMenu.addToggleButtonAuto(
        "Camera Move", settings.freeCameraMove,
        std::make_unique<ToggleFreeCameraCommand>()
    );
}

void SettingsPanel::setActiveTab(SettingsTab tab) {
    _activeTab = tab;
    _subMenu.clear();
    _keybindButtons.clear();
    _keybindPlayer.clear();
    _keybindAction.clear();
    _musicVolume.reset();
    _soundVolume.reset();
    if (tab != SettingsTab::Keybind) _keybindScroll = 0.f;
    switch (_activeTab) {
        case SettingsTab::Music: buildMusicPanel(); break;
        case SettingsTab::Keybind: buildKeybindPanel(); break;
        case SettingsTab::DevMode: buildDevModePanel(); break;
    }
    if (tab != SettingsTab::Keybind) {
        _subMenu.setFocusedIndex(0);
        _focus = FocusTarget::SubMenu;
    }
}

void SettingsPanel::startRebinding(
    ActionType action,
    int playerIndex,
    const std::shared_ptr<Button>& button
) {
    _rebindingAction = action;
    _rebindingPlayer = playerIndex;
    _activeRebindingButton = button;
    if (button) button->setText("< Press Key >");
}

void SettingsPanel::handleKeybindScroll(float delta) {
    if (_activeTab != SettingsTab::Keybind) return;
    _keybindScroll = std::clamp(_keybindScroll + delta, 0.f, kbMaxScroll());
    applyKeybindScroll();
}

void SettingsPanel::ensureKeybindVisible() {
    if (_activeTab != SettingsTab::Keybind || _subMenu.size() == 0) return;
    int idx = _subMenu.getFocusedIndex();
    if (idx < 0 || idx >= static_cast<int>(_subMenu.size())) return;
    float btnTop = kKbVpY + kbRowOffset(idx) - _keybindScroll;
    float btnBottom = btnTop + kRowH;
    const float vpTop = kKbVpY, vpBottom = kKbVpY + kKbVpH;
    if (btnTop < vpTop + 6.f) {
        handleKeybindScroll(btnTop - vpTop - 6.f);
    } else if (btnBottom > vpBottom - 6.f) {
        handleKeybindScroll(btnBottom - vpBottom + 6.f);
    }
}

void SettingsPanel::applyKeybindScroll() {
    if (_activeTab != SettingsTab::Keybind) return;
    for (std::size_t i = 0; i < _subMenu.size(); ++i) {
        float rowOffset = kbRowOffset(static_cast<int>(i));
        sf::Vector2f pos{kTopX, kKbVpY + rowOffset - _keybindScroll};
        if (auto b = _subMenu.getButton(i)) {
            b->setPosition(pos);
            b->setSize({kTopW, kRowH});
        }
    }
}

void SettingsPanel::updateKeybindButtonTexts() {
    if (_keybindButtons.size() != 12 || _keybindPlayer.size() != 12) return;
    const auto& settings = GameSettings::getInstance();
    for (std::size_t i = 0; i < 12; ++i) {
        const ActionType action = _keybindAction[i];
        const int player = _keybindPlayer[i];
        const sf::Keyboard::Key key = (player == 2)
            ? settings.getKeyForAction2(action)
            : settings.getKeyForAction(action);
        // label is same for both players (6 descriptors repeat)
        const char* baseLabel = keybinds[i % 6].label;
        std::string prefix = (player == 1 ? "P1 " : "P2 ");
        _keybindButtons[i]->setText(prefix + baseLabel + ": " + GameSettings::keyToString(key));
    }
}

void SettingsPanel::updateVolumeLabels() {
    const auto& settings = GameSettings::getInstance();
    if (_musicVolume) {
        _musicVolume->setText(
            "Music Volume: " + std::to_string(static_cast<int>(settings.musicVolume)) + "%"
        );
    }
    if (_soundVolume) {
        _soundVolume->setText(
            "Sound Volume: " + std::to_string(static_cast<int>(settings.soundVolume)) + "%"
        );
    }
}

void SettingsPanel::navigateMenu(ButtonMenu& menu, int delta) {
    const int count = static_cast<int>(menu.size());
    if (count > 0) {
        menu.setFocusedIndex((menu.getFocusedIndex() + delta + count) % count);
    }
}

void SettingsPanel::goBack() {
    _rebindingAction.reset();
    _activeRebindingButton.reset();
    if (_onBack) _onBack();
}

void SettingsPanel::handleInput(const sf::Event& event) {
    // Keybind scroll: mouse wheel
    if (_activeTab == SettingsTab::Keybind) {
        if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (wheel->wheel == sf::Mouse::Wheel::Vertical) {
                // Clamp scroll to viewport; header area also scrolls
                handleKeybindScroll(-wheel->delta * 40.f);
                return;
            }
        }
    }

    if (_rebindingAction) {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) {
                updateKeybindButtonTexts();
                _rebindingAction.reset();
                _activeRebindingButton.reset();
            } else if (key->code != sf::Keyboard::Key::Unknown) {
                auto& settings = GameSettings::getInstance();
                if (_rebindingPlayer == 2) {
                    settings.setKeyForAction2(*_rebindingAction, key->code);
                } else {
                    settings.setKeyForAction(*_rebindingAction, key->code);
                }
                settings.save();
                updateKeybindButtonTexts();
                _rebindingAction.reset();
                _activeRebindingButton.reset();
            }
        }
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (_focus == FocusTarget::SubMenu && _activeTab == SettingsTab::Music) {
            auto focused = _subMenu.getButton(
                static_cast<std::size_t>(_subMenu.getFocusedIndex())
            );
            if (auto slider = std::dynamic_pointer_cast<BarSlider>(focused);
                slider && (slider->isFocused() || slider->isSelected())) {
                if (key->code == sf::Keyboard::Key::Left
                    || key->code == sf::Keyboard::Key::A) {
                    slider->adjust(-5.f);
                    return;
                }
                if (key->code == sf::Keyboard::Key::Right
                    || key->code == sf::Keyboard::Key::D) {
                    slider->adjust(5.f);
                    return;
                }
            }
        }

        switch (key->code) {
            case sf::Keyboard::Key::Escape: goBack(); return;
            case sf::Keyboard::Key::Left:
            case sf::Keyboard::Key::A:
                _focus = FocusTarget::TopMenu;
                navigateMenu(_topMenu, -1);
                return;
            case sf::Keyboard::Key::Right:
            case sf::Keyboard::Key::D:
                _focus = FocusTarget::TopMenu;
                navigateMenu(_topMenu, 1);
                return;
            case sf::Keyboard::Key::Up:
            case sf::Keyboard::Key::W:
                _focus = FocusTarget::SubMenu;
                navigateMenu(_subMenu, -1);
                if (_activeTab == SettingsTab::Keybind) ensureKeybindVisible();
                return;
            case sf::Keyboard::Key::Down:
            case sf::Keyboard::Key::S:
                _focus = FocusTarget::SubMenu;
                navigateMenu(_subMenu, 1);
                if (_activeTab == SettingsTab::Keybind) ensureKeybindVisible();
                return;
            case sf::Keyboard::Key::Enter:
            case sf::Keyboard::Key::Space: {
                ButtonMenu& menu = _focus == FocusTarget::TopMenu
                    ? _topMenu : _subMenu;
                if (auto button = menu.getButton(
                        static_cast<std::size_t>(menu.getFocusedIndex()))) {
                    button->execute();
                }
                return;
            }
            default: return;
        }
    }

    _topMenu.processEvent(event);
    _subMenu.processEvent(event);
    if (event.is<sf::Event::MouseMoved>()) {
        for (std::size_t index = 0; index < _topMenu.size(); ++index) {
            if (auto button = _topMenu.getButton(index); button && button->isFocused()) {
                _focus = FocusTarget::TopMenu;
                break;
            }
        }
        for (std::size_t index = 0; index < _subMenu.size(); ++index) {
            if (auto button = _subMenu.getButton(index); button && button->isFocused()) {
                _focus = FocusTarget::SubMenu;
                break;
            }
        }
    }
}

void SettingsPanel::updateVisuals(float deltaTime) {
    _topMenu.updateVisuals(deltaTime);
    _subMenu.updateVisuals(deltaTime);
}

void SettingsPanel::render(sf::RenderTarget& target) {
    const sf::Vector2f viewSize = target.getView().getSize();
    const sf::Vector2f topLeft = target.getView().getCenter() - viewSize * 0.5f;
    sf::RectangleShape backdrop(viewSize);
    backdrop.setPosition(topLeft);
    backdrop.setFillColor(sf::Color(0, 0, 0, 170));
    target.draw(backdrop);

    const sf::Font& font = ResourceManager::getInstance().getFont("SuperMario");
    sf::Text title(font, "SETTINGS", 48);
    title.setOutlineThickness(5.f);
    title.setOutlineColor(sf::Color::Black);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition({960.f - title.getLocalBounds().size.x * 0.5f, 120.f});
    target.draw(title);

    {
        sf::ConvexShape panel = UI::Helper::makeRoundedRect({kPanelX, kPanelY}, {kPanelW, kPanelH}, 28.f, 10);
        panel.setFillColor(sf::Color(14, 20, 32, 155));
        panel.setOutlineThickness(1.8f);
        panel.setOutlineColor(sf::Color(165, 190, 220, 95));
        target.draw(panel);
    }

    _topMenu.render(target);

    if (_activeTab == SettingsTab::Keybind) {
        // Clipped viewport: effective Y = 295+delta (keep 295 symbolic)
        constexpr float vpX = 445.f, vpY = kKbVpY, vpW = 1030.f, vpH = kKbVpH;
        sf::View prevView = target.getView();
        sf::View clipView;
        clipView.setSize({vpW, vpH});
        clipView.setCenter({vpX + vpW*0.5f, vpY + vpH*0.5f});
        clipView.setViewport(sf::FloatRect({vpX / 1920.f, vpY / 1080.f}, {vpW / 1920.f, vpH / 1080.f}));
        target.setView(clipView);

        // Headers separated by text (not buttons)
        {
            const sf::Font& hdrFont = ResourceManager::getInstance().getFont("moon_get");
            auto drawHeader = [&](float y, const char* text) {
                sf::Text t(hdrFont, text, 20);
                t.setFillColor(sf::Color(165, 190, 220));
                t.setOutlineColor(sf::Color(10, 18, 30));
                t.setOutlineThickness(1.2f);
                sf::FloatRect b = t.getLocalBounds();
                t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
                t.setPosition({960.f, y});
                target.draw(t);
            };
            // header Y positions match applyKeybindScroll math: kKbVpY + ...
            constexpr float headerH = 34.f;
            float h1y = kKbVpY + headerH*0.5f - _keybindScroll;
            float h2y = kKbVpY + headerH - 8.f + 6*65.f + kKbGapP2 + headerH*0.5f - _keybindScroll;
            // Only draw if inside viewport
            if (h1y >= vpY - 20.f && h1y <= vpY + vpH + 20.f) drawHeader(h1y, "--  PLAYER 1  --");
            if (h2y >= vpY - 20.f && h2y <= vpY + vpH + 20.f) drawHeader(h2y, "--  PLAYER 2  --");
        }

        _subMenu.render(target);

        // Thin scrollbar on right edge of viewport when scrollable
        {
            constexpr float contentH = 34.f + 6*65.f + kKbGapP2 + 34.f + 6*65.f;
            constexpr float viewportH = kKbVpH;
            const float maxScroll = std::max(0.f, contentH - viewportH + 20.f);
            if (maxScroll > 1.f) {
                float frac = _keybindScroll / maxScroll;
                float barH = std::max(28.f, viewportH * (viewportH / contentH));
                float barY = vpY + 6.f + frac * (viewportH - barH - 12.f);
                sf::RectangleShape track({6.f, viewportH - 12.f});
                track.setPosition({vpX + vpW - 14.f, vpY + 6.f});
                track.setFillColor(sf::Color(255,255,255,22));
                target.draw(track);
                sf::RectangleShape thumb({6.f, barH});
                thumb.setPosition({vpX + vpW - 14.f, barY});
                thumb.setFillColor(sf::Color(165,190,220,165));
                target.draw(thumb);
            }
        }

        target.setView(prevView);
    } else {
        _subMenu.render(target);
    }

    const std::string hintText = _rebindingAction
        ? "PRESS ANY KEY TO REBIND (ESC TO CANCEL)"
        : "Left/Right switch tab | Up/Down navigate | Enter select | ESC back";
    sf::Text hint(font, hintText, 22);
    hint.setFillColor(_rebindingAction
        ? sf::Color(255, 180, 80) : sf::Color(230, 230, 230));
    hint.setPosition({960.f - hint.getLocalBounds().size.x * 0.5f, 965.f});
    target.draw(hint);
}

} // namespace UI

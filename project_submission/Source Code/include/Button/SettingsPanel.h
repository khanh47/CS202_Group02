#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Button/BarSlider.h"
#include "Button/ButtonMenu.h"
#include "Button/ToggleButton.h"
#include "Game/UserInput/Action.h"

namespace UI {

class SettingsPanel {
public:
    using BackCallback = std::function<void()>;

    explicit SettingsPanel(BackCallback onBack = {});

    void setOnBack(BackCallback onBack);
    void refresh();
    void handleInput(const sf::Event& event);
    void updateVisuals(float deltaTime);
    void render(sf::RenderTarget& target);

private:
    enum class SettingsTab { Music, Keybind, DevMode };
    enum class FocusTarget { TopMenu, SubMenu };

    void buildTopMenu();
    void buildMusicPanel();
    void buildKeybindPanel();
    void buildDevModePanel();
    void setActiveTab(SettingsTab tab);
    void startRebinding(ActionType action, int playerIndex, const std::shared_ptr<Button>& button);
    void updateKeybindButtonTexts();
    void updateVolumeLabels();
    void navigateMenu(ButtonMenu& menu, int delta);
    void goBack();

    // Keybind scroll helpers
    void handleKeybindScroll(float delta);
    void ensureKeybindVisible();
    void applyKeybindScroll();

    ButtonMenu _topMenu;
    ButtonMenu _subMenu;
    SettingsTab _activeTab = SettingsTab::Music;
    FocusTarget _focus = FocusTarget::SubMenu;
    std::vector<std::shared_ptr<Button>> _keybindButtons; // 12: 0-5 P1, 6-11 P2
    std::vector<int> _keybindPlayer; // parallel to _keybindButtons
    std::vector<ActionType> _keybindAction;
    std::shared_ptr<BarSlider> _musicVolume;
    std::shared_ptr<BarSlider> _soundVolume;
    float _keybindScroll = 0.f;
    int _rebindingPlayer = 1;
    std::optional<ActionType> _rebindingAction;
    std::shared_ptr<Button> _activeRebindingButton;
    BackCallback _onBack;
};

} // namespace UI

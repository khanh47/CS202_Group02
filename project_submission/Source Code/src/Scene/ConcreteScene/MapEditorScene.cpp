#include "Scene/ConcreteScene/MapEditorScene.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "Animation/Animation.h"
#include "Animation/AnimationLibrary.h"
#include "Button/CheckBox.h"
#include "Button/Dropdown.h"
#include "Button/TextInput.h"
#include "Commands/FunctionalCommand.h"
#include "Game/GameSettings.h"
#include "Game/Objects/Enemy/ConcreteEnemy/Koopa.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Pipe/Pipe.h"
#include "Game/World/LevelDataLoader.h"
#include "Game/World/ThemeAssets.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/InGameScene.h"
#include "Scene/SceneManager.h"

namespace {
using json = nlohmann::json;

const sf::Color categoryColor(MapEditorScene::Category category) {
    switch (category) {
        case MapEditorScene::Category::Blocks:
            return sf::Color(180, 105, 45);
        case MapEditorScene::Category::Items:
            return sf::Color(190, 145, 35);
        case MapEditorScene::Category::Enemies:
            return sf::Color(170, 70, 70);
        case MapEditorScene::Category::Players:
            return sf::Color(65, 105, 180);
    }
    return sf::Color(100, 100, 100);
}

}

MapEditorScene::MapEditorScene()
    : Scene("MapEditorScene"),
      MapWidth(DefaultMapWidth),
      MapHeight(DefaultMapHeight),
      _cells(
          static_cast<std::size_t>(MapWidth * MapHeight),
          '.'
      ),
      _cellPlacements(
          static_cast<std::size_t>(MapWidth * MapHeight),
          std::nullopt
      ),
      _mapView(
          {MapViewportWidth * 0.5f, MapViewportHeight * 0.5f},
          {MapViewportWidth, MapViewportHeight}
      ),
      _screenBackdrop({1920.0f, 1080.0f}),
      _gridBackdrop({MapWidth * CellSize, MapHeight * CellSize}),
      _paletteBackdrop({320.0f, 980.0f}),
      _titleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "MAP BUILDER",
          46
      ),
      _paletteTitleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "PALETTE",
          28
      ),
      _selectedText(
          ResourceManager::getInstance().getFont("moon_get"),
          "Selected: Brick (#)",
          19
      ),
      _statusText(
          ResourceManager::getInstance().getFont("moon_get"),
          "",
          16
      ),
      _instructionsBackdrop({LogicalScreenWidth, LogicalScreenHeight}),
      _instructionsPanel({1300.0f, 760.0f}),
      _instructionsTitle(
          ResourceManager::getInstance().getFont("SuperMario"),
          "MAP EDITOR INSTRUCTIONS",
          36
      ),
      _instructionsBody(
          ResourceManager::getInstance().getFont("moon_get"),
          "Mouse controls\n\n"
          "Wheel: zoom in/out\n"
          "Middle mouse + drag: move the camera\n"
          "Hold left mouse: place continuously\n"
          "Hold right mouse: erase continuously\n"
          "Shift + left mouse drag: fill a rectangle\n"
          "Shift + right mouse drag: erase a rectangle\n\n"
          "Keyboard controls\n\n"
          "Ctrl + Z: undo       Ctrl + Y: redo\n"
          "S: save map          P: save and play\n"
           "T: change theme       C: clear map\n"
           "Map Size: resize from the general map menu\n"
          "1-4: choose a category\n"
          "Esc: close this screen / go back",
          22
      ),
      _configBackdrop({LogicalScreenWidth, LogicalScreenHeight}),
      _configPanel({1100.0f, 850.0f}),
      _configTitle(
          ResourceManager::getInstance().getFont("SuperMario"),
          "PLACEMENT OPTIONS",
          36
      ),
      _configBody(
          ResourceManager::getInstance().getFont("moon_get"),
          "Configure this object before placing it on the map.",
          20
      ),
      _paletteScrollTrack({10.0f, PaletteViewportHeight}),
      _paletteScrollThumb({10.0f, 70.0f}),
      _mapSizeDropdownBackdrop({285.0f, MapSizeDropdownHeight}),
      _configScrollTrack({10.0f, ConfigViewportHeight}),
      _configScrollThumb({10.0f, 70.0f}) {
    _paletteEntries = {
        {Category::Blocks, "Solid Brick", "brick", 'b', sf::Color(183, 111, 46)},
        {Category::Blocks, "Breakable Brick", "breakable_brick", '#', sf::Color(215, 130, 55)},
        {Category::Blocks, "Ground", "terrain_grassland", 'A', sf::Color(70, 160, 86)},
        {Category::Blocks, "Coin Block", "block_coin", 'B', sf::Color(205, 157, 45)},
        {Category::Blocks, "Lucky Block", "block_lucky", '?', sf::Color(220, 175, 45)},
        {Category::Blocks, "Pipe", "pipe_basic", 'V', sf::Color(60, 160, 80)},
        {Category::Items, "Coin", "item_coin", 'c', sf::Color(244, 190, 35)},
        {Category::Items, "Fire Flower", "item_fire_flower", 'f', sf::Color(230, 75, 55)},
        {Category::Items, "Super Mushroom", "item_super_mushroom", 'u', sf::Color(210, 55, 55)},
        {Category::Items, "1-Up Mushroom", "item_one_up_mushroom", 'i', sf::Color(70, 185, 90)},
        {Category::Items, "Mega Mushroom", "item_mega_mushroom", 'G', sf::Color(215, 100, 50)},
        {Category::Items, "Super Star", "item_super_star", 's', sf::Color(250, 220, 75)},
        {Category::Items, "Mega Coin", "item_mega_coin", 'o', sf::Color(255, 185, 35)},
        {Category::Items, "Goal Flag", "item_flagpole", 'D', sf::Color(90, 190, 110)},
        {Category::Items, "Checkpoint Flag", "item_checkpoint_flag", 'q', sf::Color(90, 180, 145)},
        {Category::Enemies, "Goomba", "enemy_goomba", 'e', sf::Color(160, 90, 55)},
        {Category::Enemies, "Koopa", "enemy_koopa", 'k', sf::Color(75, 165, 75)},
        {Category::Enemies, "Piranha Plant", "enemy_piranha_plant", 'p', sf::Color(200, 70, 70)},
        {Category::Players, "Mario", "player_mario", 'M', sf::Color(65, 105, 210)},
        {Category::Players, "Luigi", "player_luigi", 'L', sf::Color(55, 165, 85)}
    };

    _themeOptions = {
        {"sky", "Sky", "parallax_sky", "ground_theme"},
        {"underground", "Underground", "parallax_underground", "underground_theme"}
    };

    _mapView.setViewport({
        {0.0f, 0.0f},
        {MapViewportWidth / LogicalScreenWidth, 1.0f}
    });
    clampMapView();

    _screenBackdrop.setPosition({0.0f, 0.0f});
    _screenBackdrop.setFillColor(sf::Color(10, 20, 38));

    _gridBackdrop.setPosition({0.0f, 0.0f});
    _gridBackdrop.setFillColor(sf::Color(19, 43, 71));
    _gridBackdrop.setOutlineThickness(3.0f);
    _gridBackdrop.setOutlineColor(sf::Color(108, 163, 210));

    _paletteBackdrop.setPosition({1584.0f, 70.0f});
    _paletteBackdrop.setFillColor(sf::Color(24, 45, 72, 245));
    _paletteBackdrop.setOutlineThickness(2.0f);
    _paletteBackdrop.setOutlineColor(sf::Color(95, 142, 190));

    _titleText.setPosition({24.0f, 24.0f});
    _titleText.setFillColor(sf::Color(255, 226, 120));
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setOutlineThickness(4.0f);

    _paletteTitleText.setPosition({PaletteLeft, 88.0f});
    _paletteTitleText.setFillColor(sf::Color(255, 226, 120));
    _paletteTitleText.setOutlineColor(sf::Color::Black);
    _paletteTitleText.setOutlineThickness(2.0f);

    _selectedText.setPosition({500.0f, 40.0f});
    _selectedText.setFillColor(sf::Color::White);

    _statusText.setPosition({500.0f, 78.0f});
    _statusText.setFillColor(sf::Color(180, 220, 255));
    _statusText.setOutlineColor(sf::Color::Black);
    _statusText.setOutlineThickness(1.0f);

    _instructionsBackdrop.setPosition({0.0f, 0.0f});
    _instructionsBackdrop.setFillColor(sf::Color(0, 0, 0, 205));

    _instructionsPanel.setPosition({310.0f, 110.0f});
    _instructionsPanel.setFillColor(sf::Color(24, 45, 72, 250));
    _instructionsPanel.setOutlineThickness(4.0f);
    _instructionsPanel.setOutlineColor(sf::Color(120, 180, 235));

    _instructionsTitle.setPosition({650.0f, 145.0f});
    _instructionsTitle.setFillColor(sf::Color(255, 226, 120));
    _instructionsTitle.setOutlineColor(sf::Color::Black);
    _instructionsTitle.setOutlineThickness(3.0f);

    _instructionsBody.setPosition({390.0f, 225.0f});
    _instructionsBody.setFillColor(sf::Color(235, 245, 255));

    _configBackdrop.setPosition({0.0f, 0.0f});
    _configBackdrop.setFillColor(sf::Color(0, 0, 0, 210));

    _configPanel.setPosition({410.0f, 90.0f});
    _configPanel.setFillColor(sf::Color(24, 45, 72, 252));
    _configPanel.setOutlineThickness(4.0f);
    _configPanel.setOutlineColor(sf::Color(120, 180, 235));

    _configTitle.setPosition({720.0f, 125.0f});
    _configTitle.setFillColor(sf::Color(255, 226, 120));
    _configTitle.setOutlineColor(sf::Color::Black);
    _configTitle.setOutlineThickness(3.0f);

    _configBody.setPosition({480.0f, 180.0f});
    _configBody.setFillColor(sf::Color(235, 245, 255));

    _paletteScrollTrack.setPosition({PaletteScrollX, PaletteViewportTop});
    _paletteScrollTrack.setFillColor(sf::Color(9, 22, 38, 230));
    _paletteScrollThumb.setFillColor(sf::Color(115, 175, 225, 235));

    _mapSizeDropdownBackdrop.setPosition({1605.0f, MapSizeDropdownTop});
    _mapSizeDropdownBackdrop.setFillColor(sf::Color(24, 45, 72, 252));
    _mapSizeDropdownBackdrop.setOutlineThickness(2.0f);
    _mapSizeDropdownBackdrop.setOutlineColor(sf::Color(120, 180, 235));

    _configScrollTrack.setPosition({ConfigScrollX, ConfigViewportTop});
    _configScrollTrack.setFillColor(sf::Color(9, 22, 38, 230));
    _configScrollThumb.setFillColor(sf::Color(115, 175, 225, 235));
}

void MapEditorScene::init() {
    setupMenus();
    selectSymbol('#');
    if (!loadSavedMap()) {
        setStatus(
            "New map: " + std::to_string(MapWidth)
            + " x " + std::to_string(MapHeight) + " cells"
        );
    }
    refreshThemeButton();
}

void MapEditorScene::onEnter() {
    Scene::onEnter();
}

void MapEditorScene::onExit() {
    Scene::onExit();
}

void MapEditorScene::updateSimulation(const float& fixedDt) {
    (void)fixedDt;
}

void MapEditorScene::updateVisuals(float deltaTime) {
    _categoryMenu.updateVisuals(deltaTime);
    _paletteMenu.updateVisuals(deltaTime);
    _generalMapMenu.updateVisuals(deltaTime);
    _actionMenu.updateVisuals(deltaTime);
    _mapSizeMenu.updateVisuals(deltaTime);
    _instructionsMenu.updateVisuals(deltaTime);
    _configMenu.updateVisuals(deltaTime);
}

void MapEditorScene::handleInput(const sf::Event& event) {
    if (_showInstructions) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->code == sf::Keyboard::Key::Escape) {
                _showInstructions = false;
                return;
            }
        }
        _instructionsMenu.processEvent(event);
        return;
    }

    if (_configMode != ConfigMode::None) {
        if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (mouseWheel->position.x >= 410
                && mouseWheel->position.x < 1510
                && mouseWheel->position.y >= ConfigViewportTop
                && mouseWheel->position.y < ConfigViewportTop + ConfigViewportHeight) {
                scrollConfig(mouseWheel->delta);
                return;
            }
        }
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->code == sf::Keyboard::Key::Escape) {
                cancelConfig();
                return;
            }
        }
        _configMenu.processEvent(event);
        return;
    }

    if (_mapSizeExpanded) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->code == sf::Keyboard::Key::Escape) {
                closeMapSizeDropdown();
                return;
            }
            _mapSizeMenu.processEvent(event);
            tryAutoApplyMapSize();
            return;
        }

        if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
            (void)mouseMove;
            _mapSizeMenu.processEvent(event);
            return;
        }

        if (const auto* mousePress = event.getIf<sf::Event::MouseButtonPressed>()) {
            const sf::Vector2f mousePosition = {
                static_cast<float>(mousePress->position.x),
                static_cast<float>(mousePress->position.y)
            };
            const sf::FloatRect mapSizeButtonBounds{
                {1605.0f, MapSizeButtonTop},
                {285.0f, 40.0f}
            };
            const sf::FloatRect dropdownBounds{
                {1605.0f, MapSizeDropdownTop},
                {285.0f, MapSizeDropdownHeight}
            };

            _mapSizeMenu.processEvent(event);
            if (mapSizeButtonBounds.contains(mousePosition)) {
                tryAutoApplyMapSize();
                if (_mapSizeExpanded) {
                    closeMapSizeDropdown();
                }
                return;
            }
            if (!dropdownBounds.contains(mousePosition)) {
                tryAutoApplyMapSize();
                if (_mapSizeExpanded) {
                    closeMapSizeDropdown();
                }
            }
            return;
        }

        if (const auto* mouseRelease = event.getIf<sf::Event::MouseButtonReleased>()) {
            handleMouseReleased(*mouseRelease);
            return;
        }

        return;
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->control && keyEvent->code == sf::Keyboard::Key::Z) {
            undoLastEdit();
            return;
        }
        if (keyEvent->control && keyEvent->code == sf::Keyboard::Key::Y) {
            redoLastEdit();
            return;
        }

        switch (keyEvent->code) {
            case sf::Keyboard::Key::Escape:
                if (auto* manager = getSceneManager()) {
                    manager->requestPopScene();
                }
                return;
            case sf::Keyboard::Key::S:
                saveMap();
                return;
            case sf::Keyboard::Key::P:
                saveAndPlay();
                return;
            case sf::Keyboard::Key::U:
                undoLastEdit();
                return;
            case sf::Keyboard::Key::T:
                cycleTheme();
                return;
            case sf::Keyboard::Key::C:
                clearMap();
                return;
            case sf::Keyboard::Key::Num1:
                selectCategory(Category::Blocks);
                return;
            case sf::Keyboard::Key::Num2:
                selectCategory(Category::Items);
                return;
            case sf::Keyboard::Key::Num3:
                selectCategory(Category::Enemies);
                return;
            case sf::Keyboard::Key::Num4:
                selectCategory(Category::Players);
                return;
            default:
                return;
        }
    }

    if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (mouseWheel->position.x >= 1584
            && mouseWheel->position.x < 1920
            && mouseWheel->position.y >= PaletteViewportTop
            && mouseWheel->position.y < PaletteViewportTop + PaletteViewportHeight) {
            scrollPalette(mouseWheel->delta);
            return;
        }
        zoomMap(
            mouseWheel->delta,
            mouseWheel->position
        );
        return;
    }

    if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
        handleMouseMoved(*mouseMove);
        if (_themeDropdown && _themeDropdown->isOpen()) {
            _themeDropdown->processEvent(event);
            return;
        }
        _categoryMenu.processEvent(event);
        _paletteMenu.processEvent(event);
        _generalMapMenu.processEvent(event);
        _actionMenu.processEvent(event);
        _configMenu.processEvent(event);
        return;
    }

    if (const auto* mousePress = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (_themeDropdown && _themeDropdown->isOpen()) {
            _themeDropdown->processEvent(event);
            return;
        }
        _categoryMenu.processEvent(event);
        _paletteMenu.processEvent(event);
        _generalMapMenu.processEvent(event);
        _actionMenu.processEvent(event);
        _configMenu.processEvent(event);
        handleMousePressed(*mousePress);
        return;
    }

    if (const auto* mouseRelease = event.getIf<sf::Event::MouseButtonReleased>()) {
        handleMouseReleased(*mouseRelease);
        return;
    }

    if (event.is<sf::Event::FocusLost>() || event.is<sf::Event::MouseLeft>()) {
        _leftMouseHeld = false;
        _rightMouseHeld = false;
        _middleMouseHeld = false;
        _paintActive = false;
        _rectangleDrag = false;
        _strokeUndoCaptured = false;
        _hoverColumn = -1;
        _hoverRow = -1;
    }
}

void MapEditorScene::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());
    target.draw(_screenBackdrop);

    drawMap(target);

    target.setView(target.getDefaultView());
    target.draw(_paletteBackdrop);
    target.draw(_titleText);
    target.draw(_paletteTitleText);
    target.draw(_selectedText);
    target.draw(_statusText);

    _categoryMenu.render(target);
    sf::View paletteClipView(
        {PaletteLeft + 142.5f, PaletteViewportTop + PaletteViewportHeight * 0.5f},
        {285.0f, PaletteViewportHeight}
    );
    paletteClipView.setViewport({
        {PaletteLeft / LogicalScreenWidth, PaletteViewportTop / LogicalScreenHeight},
        {285.0f / LogicalScreenWidth, PaletteViewportHeight / LogicalScreenHeight}
    });
    target.setView(paletteClipView);
    _paletteMenu.render(target);
    target.setView(target.getDefaultView());
    updateScrollVisuals();
    const float paletteContentHeight = _paletteMenu.size() == 0
        ? 0.0f
        : static_cast<float>(_paletteMenu.size() - 1) * PaletteButtonSpacing
            + PaletteButtonHeight;
    if (paletteContentHeight > PaletteViewportHeight) {
        target.draw(_paletteScrollTrack);
        target.draw(_paletteScrollThumb);
    }
    _generalMapMenu.render(target);
    _actionMenu.render(target);
    sf::ConvexShape mapSizeArrow;
    mapSizeArrow.setPointCount(3);
    mapSizeArrow.setPoint(
        0,
        {1865.0f, MapSizeButtonTop + (_mapSizeExpanded ? 18.0f : 17.0f)}
    );
    mapSizeArrow.setPoint(
        1,
        {1881.0f, MapSizeButtonTop + (_mapSizeExpanded ? 18.0f : 17.0f)}
    );
    mapSizeArrow.setPoint(
        2,
        {1873.0f, MapSizeButtonTop + (_mapSizeExpanded ? 8.0f : 27.0f)}
    );
    mapSizeArrow.setFillColor(sf::Color::White);
    target.draw(mapSizeArrow);
    if (_mapSizeExpanded) {
        target.draw(_mapSizeDropdownBackdrop);
        _mapSizeMenu.render(target);
    }
    if (_themeDropdown) {
        _themeDropdown->renderPopup(target);
    }

    if (_showInstructions) {
        target.draw(_instructionsBackdrop);
        target.draw(_instructionsPanel);
        target.draw(_instructionsTitle);
        target.draw(_instructionsBody);
        _instructionsMenu.render(target);
    }

    if (_configMode != ConfigMode::None) {
        target.draw(_configBackdrop);
        target.draw(_configPanel);
        target.draw(_configTitle);
        target.draw(_configBody);
        sf::View configClipView(
            {960.0f, ConfigViewportTop + ConfigViewportHeight * 0.5f},
            {680.0f, ConfigViewportHeight}
        );
        configClipView.setViewport({
            {620.0f / LogicalScreenWidth, ConfigViewportTop / LogicalScreenHeight},
            {680.0f / LogicalScreenWidth, ConfigViewportHeight / LogicalScreenHeight}
        });
        target.setView(configClipView);
        _configMenu.render(target);
        if (_coinCapacityDropdown) {
            _coinCapacityDropdown->renderPopup(target);
        }
        if (_luckyTextureDropdown) {
            _luckyTextureDropdown->renderPopup(target);
        }
        if (_luckyCapacityDropdown) {
            _luckyCapacityDropdown->renderPopup(target);
        }
        if (_pipeOrientationDropdown) {
            _pipeOrientationDropdown->renderPopup(target);
        }
        if (_pipeEndSideDropdown) {
            _pipeEndSideDropdown->renderPopup(target);
        }
        target.setView(target.getDefaultView());
        const float configContentHeight = _configMenu.size() == 0
            ? 0.0f
            : static_cast<float>(_configMenu.size() - 1) * ConfigButtonSpacing
                + ConfigButtonHeight;
        if (configContentHeight > ConfigViewportHeight) {
            target.draw(_configScrollTrack);
            target.draw(_configScrollThumb);
        }
    }
}

void MapEditorScene::setupMenus() {
    setupCategoryMenu();
    setupPaletteMenu();
    setupGeneralMapMenu();
    setupActionMenu();
    setupInstructionsMenu();
    setupConfigMenu();

    _categoryMenu.setMouseEnabled(true); _categoryMenu.setKeyboardEnabled(false);
    _paletteMenu.setMouseEnabled(true); _paletteMenu.setKeyboardEnabled(false);
    _generalMapMenu.setMouseEnabled(true); _generalMapMenu.setKeyboardEnabled(false);
    _actionMenu.setMouseEnabled(true); _actionMenu.setKeyboardEnabled(false);
    _mapSizeMenu.setMouseEnabled(true); _mapSizeMenu.setKeyboardEnabled(false);
    _instructionsMenu.setMouseEnabled(true); _instructionsMenu.setKeyboardEnabled(false);
    _configMenu.setMouseEnabled(true); _configMenu.setKeyboardEnabled(false);
}

void MapEditorScene::setupCategoryMenu() {
    _categoryMenu.clear();
    _categoryMenu.setLayoutProperties(
        {1600.0f, 132.0f},
        {72.0f, 42.0f},
        76.0f,
        true,
        sf::Color(62, 105, 157),
        15
    );

    const std::vector<std::pair<Category, std::string>> categories = {
        {Category::Blocks, "Blocks"},
        {Category::Items, "Items"},
        {Category::Enemies, "Enemies"},
        {Category::Players, "Players"}
    };
    for (const auto& [category, label] : categories) {
        _categoryMenu.addButtonAuto(
            label,
            15,
            std::make_unique<FunctionalCommand>(
                label,
                [this, category]() { selectCategory(category); }
            ),
            categoryColor(category)
        );
    }
}

void MapEditorScene::setupPaletteMenu() {
    _paletteMenu.clear();
    _paletteMenu.setLayoutProperties(
        {PaletteLeft, PaletteViewportTop - _paletteScrollOffset},
        {285.0f, PaletteButtonHeight},
        PaletteButtonSpacing,
        false,
        categoryColor(_activeCategory),
        18
    );

    for (const PaletteEntry& entry : _paletteEntries) {
        if (entry.category != _activeCategory) {
            continue;
        }

        std::string label = entry.label;
        if (entry.prefabId == "terrain_grassland"
            && !_themeOptions.empty()) {
            label += ": " + _themeOptions[_themeIndex].label;
        }
        _paletteMenu.addButtonAuto(
            label,
            18,
            std::make_unique<FunctionalCommand>(
                label,
                [this, symbol = entry.symbol]() { selectSymbol(symbol); }
            ),
            entry.previewColor
        );
    }
    const float contentHeight = _paletteMenu.size() == 0
        ? 0.0f
        : static_cast<float>(_paletteMenu.size() - 1) * PaletteButtonSpacing
            + PaletteButtonHeight;
    const float maximumScroll = std::max(
        0.0f,
        contentHeight - PaletteViewportHeight
    );
    _paletteScrollOffset = std::clamp(
        _paletteScrollOffset,
        0.0f,
        maximumScroll
    );
    _paletteMenu.setLayoutProperties(
        {PaletteLeft, PaletteViewportTop - _paletteScrollOffset},
        {285.0f, PaletteButtonHeight},
        PaletteButtonSpacing,
        false,
        categoryColor(_activeCategory),
        18
    );
    updateScrollVisuals();
}

void MapEditorScene::setupGeneralMapMenu() {
    _generalMapMenu.clear();
    _generalMapMenu.setLayoutProperties(
        {1605.0f, GeneralMapTop},
        {285.0f, 40.0f},
        GeneralMapSpacing,
        false,
        sf::Color(70, 110, 150),
        16
    );

    std::vector<std::string> themeLabels;
    themeLabels.reserve(_themeOptions.size());
    for (const ThemeChoice& theme : _themeOptions) {
        themeLabels.push_back(theme.label);
    }
    _themeDropdown = std::make_shared<UI::Dropdown>(
        sf::Vector2f{1605.0f, GeneralMapTop},
        sf::Vector2f{285.0f, 40.0f},
        sf::Color(70, 110, 150),
        "Theme",
        16,
        std::move(themeLabels),
        _themeIndex,
        20.0f
    );
    _themeDropdown->setSelectionCallback(
        [this](std::size_t index) { setThemeIndex(index, true); }
    );
    _generalMapMenu.addButton(_themeDropdown);
    _generalMapMenu.addButtonAuto(
        "Map Size",
        16,
        std::make_unique<FunctionalCommand>(
            "Map Size", [this]() { openMapSizeConfig(); }
        ),
        sf::Color(70, 110, 150)
    );
}

void MapEditorScene::setupInstructionsMenu() {
    _instructionsMenu.clear();
    _instructionsMenu.setLayoutProperties(
        {825.0f, 805.0f},
        {270.0f, 55.0f},
        60.0f,
        false,
        sf::Color(62, 105, 157),
        20
    );
    _instructionsMenu.addButtonAuto(
        "Close",
        20,
        std::make_unique<FunctionalCommand>(
            "Close", [this]() { _showInstructions = false; }
        )
    );
}

void MapEditorScene::setupConfigMenu() {
    _configMenu.clear();
    _configMenu.setLayoutProperties(
        {620.0f, ConfigViewportTop - _configScrollOffset},
        {680.0f, ConfigButtonHeight},
        ConfigButtonSpacing,
        false,
        sf::Color(53, 91, 130),
        18
    );
    updateScrollVisuals();
}

void MapEditorScene::refreshConfigMenu() {
    _configMenu.clear();
    _configMenu.setLayoutProperties(
        {620.0f, ConfigViewportTop - _configScrollOffset},
        {680.0f, ConfigButtonHeight},
        ConfigButtonSpacing,
        false,
        sf::Color(53, 91, 130),
        18
    );

    const sf::Color buttonColor(53, 91, 130);
    _coinCapacityDropdown.reset();
    _luckyTextureDropdown.reset();
    _luckyCapacityDropdown.reset();
    _luckyOptionChecks.clear();
    _pipeOrientationDropdown.reset();
    _pipeEndSideDropdown.reset();
    _pipeLengthInput.reset();
    _pipeWarpCheck.reset();
    _pipeWarpIDInput.reset();
    _pipeWarpTargetInput.reset();
    _pipePiranhaCheck.reset();
    _pipeContentsStaticCheck.reset();
    _mapWidthInput.reset();
    _mapHeightInput.reset();

    const auto controlPosition = [this]() {
        return sf::Vector2f(
            620.0f,
            ConfigViewportTop - _configScrollOffset
                + static_cast<float>(_configMenu.size()) * ConfigButtonSpacing
        );
    };
    const auto addControl = [this](const std::shared_ptr<UI::Button>& control) {
        _configMenu.addButton(control);
    };
    const auto addButton = [this, &buttonColor](
        const std::string& label,
        std::unique_ptr<ICommand> command
    ) {
        _configMenu.addButtonAuto(
            label,
            18,
            std::move(command),
            buttonColor
        );
    };

    if (_configMode == ConfigMode::MapSize) {
        _mapWidthInput = std::make_shared<UI::TextInput>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Map width (10-500)",
            18,
            std::to_string(_draftMapWidth)
        );
        _mapWidthInput->setNumericOnly(true);
        _mapWidthInput->setMaxLength(3);
        _mapWidthInput->setValueCallback([this](const std::string& value) {
            if (!value.empty()) {
                _draftMapWidth = std::clamp(
                    std::stoi(value),
                    MinimumMapWidth,
                    MaximumMapWidth
                );
            }
        });
        addControl(_mapWidthInput);

        _mapHeightInput = std::make_shared<UI::TextInput>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Map height (8-60)",
            18,
            std::to_string(_draftMapHeight)
        );
        _mapHeightInput->setNumericOnly(true);
        _mapHeightInput->setMaxLength(2);
        _mapHeightInput->setValueCallback([this](const std::string& value) {
            if (!value.empty()) {
                _draftMapHeight = std::clamp(
                    std::stoi(value),
                    MinimumMapHeight,
                    MaximumMapHeight
                );
            }
        });
        addControl(_mapHeightInput);

        addButton(
            "Apply map size",
            std::make_unique<FunctionalCommand>(
                "Apply map size", [this]() { applyMapSize(); }
            )
        );
        addButton(
            "Cancel",
            std::make_unique<FunctionalCommand>(
                "Cancel", [this]() { cancelConfig(); }
            )
        );
        _configBody.setString(
            "Set the map dimensions in cells. Existing content is preserved\n"
            "where it still fits; shrinking removes content outside the map."
        );
    } else if (_configMode == ConfigMode::CoinBlock) {
        _coinCapacityDropdown = std::make_shared<UI::Dropdown>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Maximum coins",
            18,
            std::vector<std::string>{
                "1 coin",
                "2 coins",
                "3 coins",
                "4 coins",
                "5 coins",
                "6 coins",
                "7 coins",
                "8 coins",
                "9 coins",
                "10 coins"
            },
            static_cast<std::size_t>(std::clamp(
                _draftPlacement.coinCapacity - 1,
                0,
                9
            ))
        );
        _coinCapacityDropdown->setSelectionCallback(
            [this](std::size_t index) {
                _draftPlacement.coinCapacity = static_cast<int>(index) + 1;
            }
        );
        addControl(_coinCapacityDropdown);
        addButton(
            "Confirm placement",
            std::make_unique<FunctionalCommand>(
                "Confirm placement", [this]() { confirmConfig(); }
            )
        );
        addButton(
            "Cancel",
            std::make_unique<FunctionalCommand>(
                "Cancel", [this]() { cancelConfig(); }
            )
        );
        _configBody.setString(
            "Choose how many coins this coin block can release.\n"
            "The block becomes empty after the maximum is reached."
        );
    } else if (_configMode == ConfigMode::LuckyBlock) {
        const std::vector<std::string> itemKeys = {
            "Coin",
            "SuperMushroom",
            "OneUpMushroom",
            "FireFlower",
            "SuperStar",
            "MegaMushroom",
            "MegaCoin"
        };
        const std::vector<std::string> labels = {
            "Coin",
            "Super Mushroom",
            "1-Up Mushroom",
            "Fire Flower",
            "Super Star",
            "Mega Mushroom",
            "Mega Coin"
        };
        for (std::size_t index = 0; index < itemKeys.size(); ++index) {
            const bool enabled = std::any_of(
                _draftPlacement.luckyOptions.begin(),
                _draftPlacement.luckyOptions.end(),
                [&itemKey = itemKeys[index]](const LuckyOptionData& option) {
                    return option.itemTypeKey == itemKey;
                }
            );
            auto checkbox = std::make_shared<UI::CheckBox>(
                controlPosition(),
                sf::Vector2f{680.0f, ConfigButtonHeight},
                buttonColor,
                labels[index],
                18,
                enabled
            );
            checkbox->setCheckedCallback(
                [this, index](bool) { cycleLuckyOption(index); }
            );
            _luckyOptionChecks.push_back(checkbox);
            addControl(checkbox);
        }
        const std::size_t luckyTextureIndex =
            _draftPlacement.luckyTexture == "invisible"
            ? 1
            : (_draftPlacement.luckyTexture == "brick" ? 2 : 0);
        _luckyTextureDropdown = std::make_shared<UI::Dropdown>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Texture",
            18,
            std::vector<std::string>{
                "Normal lucky block",
                "Invisible lucky block",
                "Brick texture lucky block"
            },
            luckyTextureIndex
        );
        _luckyTextureDropdown->setSelectionCallback(
            [this](std::size_t index) {
                _draftPlacement.luckyTexture = index == 1
                    ? "invisible"
                    : (index == 2 ? "brick" : "default");
            }
        );
        addControl(_luckyTextureDropdown);
        _luckyCapacityDropdown = std::make_shared<UI::Dropdown>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Capacity",
            18,
            std::vector<std::string>{"1 use", "2 uses", "3 uses", "4 uses", "5 uses"},
            static_cast<std::size_t>(std::clamp(
                _draftPlacement.luckyCapacity - 1,
                0,
                4
            ))
        );
        _luckyCapacityDropdown->setSelectionCallback(
            [this](std::size_t index) {
                _draftPlacement.luckyCapacity = static_cast<int>(index) + 1;
            }
        );
        addControl(_luckyCapacityDropdown);
        addButton(
            "Confirm placement",
            std::make_unique<FunctionalCommand>(
                "Confirm placement", [this]() { confirmConfig(); }
            )
        );
        addButton(
            "Cancel",
            std::make_unique<FunctionalCommand>(
                "Cancel", [this]() { cancelConfig(); }
            )
        );
        _configBody.setString(
            "Choose which outcomes this lucky block can contain.\n"
            "Checkboxes choose outcomes; the dropdown sets the number of uses."
        );
    } else if (_configMode == ConfigMode::Pipe) {
        _pipeOrientationDropdown = std::make_shared<UI::Dropdown>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Orientation",
            18,
            std::vector<std::string>{"Vertical", "Horizontal"},
            _draftPlacement.pipeOrientation == "horizontal" ? 1 : 0
        );
        _pipeOrientationDropdown->setSelectionCallback(
            [this](std::size_t index) {
                _draftPlacement.pipeOrientation = index == 1
                    ? "horizontal"
                    : "vertical";
                if (_draftPlacement.pipeOrientation == "vertical") {
                    if (_draftPlacement.pipeEndSide != "top"
                        && _draftPlacement.pipeEndSide != "bottom") {
                        _draftPlacement.pipeEndSide = "top";
                    }
                } else if (_draftPlacement.pipeEndSide != "left"
                           && _draftPlacement.pipeEndSide != "right") {
                    _draftPlacement.pipeEndSide = "right";
                }
                if (_pipeEndSideDropdown) {
                    _pipeEndSideDropdown->setOptions(
                        _draftPlacement.pipeOrientation == "vertical"
                            ? std::vector<std::string>{"Top", "Bottom"}
                            : std::vector<std::string>{"Left", "Right"}
                    );
                    _pipeEndSideDropdown->setSelectedIndex(
                        _draftPlacement.pipeEndSide == "bottom"
                            || _draftPlacement.pipeEndSide == "right"
                            ? 1
                            : 0
                    );
                }
            }
        );
        addControl(_pipeOrientationDropdown);

        _pipeEndSideDropdown = std::make_shared<UI::Dropdown>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "End side",
            18,
            _draftPlacement.pipeOrientation == "vertical"
                ? std::vector<std::string>{"Top", "Bottom"}
                : std::vector<std::string>{"Left", "Right"},
            _draftPlacement.pipeEndSide == "bottom"
                || _draftPlacement.pipeEndSide == "right"
                ? 1
                : 0
        );
        _pipeEndSideDropdown->setSelectionCallback(
            [this](std::size_t index) {
                _draftPlacement.pipeEndSide =
                    _draftPlacement.pipeOrientation == "vertical"
                    ? (index == 1 ? "bottom" : "top")
                    : (index == 1 ? "right" : "left");
            }
        );
        addControl(_pipeEndSideDropdown);

        _pipeLengthInput = std::make_shared<UI::TextInput>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Body length (1-6)",
            18,
            std::to_string(_draftPlacement.pipeBodyLength)
        );
        _pipeLengthInput->setNumericOnly(true);
        _pipeLengthInput->setMaxLength(1);
        _pipeLengthInput->setValueCallback([this](const std::string& value) {
            if (!value.empty()) {
                _draftPlacement.pipeBodyLength = std::clamp(
                    std::stoi(value),
                    1,
                    6
                );
            }
        });
        addControl(_pipeLengthInput);

        _pipeWarpCheck = std::make_shared<UI::CheckBox>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Warp enabled",
            18,
            _draftPlacement.pipeIsWarp
        );
        _pipeWarpCheck->setCheckedCallback(
            [this](bool checked) { _draftPlacement.pipeIsWarp = checked; }
        );
        addControl(_pipeWarpCheck);

        _pipeWarpIDInput = std::make_shared<UI::TextInput>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Warp ID (1-20)",
            18,
            std::to_string(_draftPlacement.warpID)
        );
        _pipeWarpIDInput->setNumericOnly(true);
        _pipeWarpIDInput->setMaxLength(2);
        _pipeWarpIDInput->setValueCallback([this](const std::string& value) {
            if (!value.empty()) {
                _draftPlacement.warpID = std::clamp(std::stoi(value), 1, 20);
            }
        });
        addControl(_pipeWarpIDInput);

        _pipeWarpTargetInput = std::make_shared<UI::TextInput>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Warp target (1-20)",
            18,
            std::to_string(_draftPlacement.warpTarget)
        );
        _pipeWarpTargetInput->setNumericOnly(true);
        _pipeWarpTargetInput->setMaxLength(2);
        _pipeWarpTargetInput->setValueCallback([this](const std::string& value) {
            if (!value.empty()) {
                _draftPlacement.warpTarget = std::clamp(std::stoi(value), 1, 20);
            }
        });
        addControl(_pipeWarpTargetInput);

        _pipePiranhaCheck = std::make_shared<UI::CheckBox>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Contains piranha plant",
            18,
            _draftPlacement.pipeContainsPiranha
        );
        _pipePiranhaCheck->setCheckedCallback(
            [this](bool checked) { _draftPlacement.pipeContainsPiranha = checked; }
        );
        addControl(_pipePiranhaCheck);

        _pipeContentsStaticCheck = std::make_shared<UI::CheckBox>(
            controlPosition(),
            sf::Vector2f{680.0f, ConfigButtonHeight},
            buttonColor,
            "Piranha plant stays extended",
            18,
            _draftPlacement.pipeContentsStatic
        );
        _pipeContentsStaticCheck->setCheckedCallback(
            [this](bool checked) { _draftPlacement.pipeContentsStatic = checked; }
        );
        addControl(_pipeContentsStaticCheck);

        addButton(
            "Confirm placement",
            std::make_unique<FunctionalCommand>(
                "Confirm placement", [this]() { confirmConfig(); }
            )
        );
        addButton(
            "Cancel",
            std::make_unique<FunctionalCommand>(
                "Cancel", [this]() { cancelConfig(); }
            )
        );
        _configBody.setString(
            "Configure the pipe before placing it.\n"
            "Use dropdowns, input bars, and checkboxes for every pipe parameter."
        );
    }

    const float contentHeight = _configMenu.size() == 0
        ? 0.0f
        : static_cast<float>(_configMenu.size() - 1) * ConfigButtonSpacing
            + ConfigButtonHeight;
    const float maximumScroll = std::max(
        0.0f,
        contentHeight - ConfigViewportHeight
    );
    _configScrollOffset = std::clamp(
        _configScrollOffset,
        0.0f,
        maximumScroll
    );
    _configMenu.setLayoutProperties(
        {620.0f, ConfigViewportTop - _configScrollOffset},
        {680.0f, ConfigButtonHeight},
        ConfigButtonSpacing,
        false,
        buttonColor,
        18
    );
    updateScrollVisuals();
}

void MapEditorScene::openLuckyBlockConfig() {
    _configMode = ConfigMode::LuckyBlock;
    _configScrollOffset = 0.0f;
    _draftPlacement = CellPlacement{};
    _draftPlacement.prefabId = "block_lucky";
    _draftPlacement.luckyOptions = {
        {"Coin", 1.0f},
        {"SuperMushroom", 1.0f},
        {"OneUpMushroom", 1.0f},
        {"FireFlower", 1.0f},
        {"SuperStar", 1.0f}
    };
    _draftPlacement.luckyTexture = "default";
    _draftPlacement.luckyCapacity = 1;
    _showInstructions = false;
    refreshConfigMenu();
    setStatus("Configure the lucky block, then confirm placement");
}

void MapEditorScene::openCoinBlockConfig() {
    _configMode = ConfigMode::CoinBlock;
    _configScrollOffset = 0.0f;
    _draftPlacement = CellPlacement{};
    _draftPlacement.prefabId = "block_coin";
    _draftPlacement.coinCapacity = 10;
    _showInstructions = false;
    refreshConfigMenu();
    setStatus("Configure the coin block, then confirm placement");
}

void MapEditorScene::openPipeConfig() {
    _configMode = ConfigMode::Pipe;
    _configScrollOffset = 0.0f;
    _draftPlacement = CellPlacement{};
    _draftPlacement.prefabId = "pipe_basic";
    _draftPlacement.pipeOrientation = "vertical";
    _draftPlacement.pipeEndSide = "top";
    _draftPlacement.pipeBodyLength = 2;
    _draftPlacement.pipeIsWarp = false;
    _draftPlacement.warpID = 1;
    _draftPlacement.warpTarget = 2;
    _draftPlacement.pipeContainsPiranha = false;
    _draftPlacement.pipeContentsStatic = false;
    _showInstructions = false;
    refreshConfigMenu();
    setStatus("Configure the pipe, then confirm placement");
}

void MapEditorScene::openMapSizeConfig() {
    if (_mapSizeExpanded) {
        closeMapSizeDropdown();
        setStatus("Map size controls closed");
        return;
    }

    _draftMapWidth = MapWidth;
    _draftMapHeight = MapHeight;
    _showInstructions = false;
    _mapSizeExpanded = true;
    refreshMapSizeDropdown();
    setStatus("Enter width and height, then press Enter or click outside");
}

void MapEditorScene::refreshMapSizeDropdown() {
    _mapSizeMenu.clear();
    _mapWidthInput.reset();
    _mapHeightInput.reset();
    _mapSizeMenu.setLayoutProperties(
        {1605.0f, MapSizeDropdownTop + 4.0f},
        {285.0f, MapSizeInputHeight},
        MapSizeInputSpacing,
        false,
        sf::Color(53, 91, 130),
        15
    );

    const sf::Color inputColor(53, 91, 130);
    _mapWidthInput = std::make_shared<UI::TextInput>(
        sf::Vector2f{1605.0f, MapSizeDropdownTop + 4.0f},
        sf::Vector2f{285.0f, MapSizeInputHeight},
        inputColor,
        "Width (10-500)",
        15,
        std::to_string(_draftMapWidth)
    );
    _mapWidthInput->setNumericOnly(true);
    _mapWidthInput->setMaxLength(3);
    _mapWidthInput->setValueCallback([this](const std::string& value) {
        if (!value.empty()) {
            _draftMapWidth = std::clamp(
                std::stoi(value),
                MinimumMapWidth,
                MaximumMapWidth
            );
        }
    });
    _mapSizeMenu.addButton(_mapWidthInput);

    _mapHeightInput = std::make_shared<UI::TextInput>(
        sf::Vector2f{
            1605.0f,
            MapSizeDropdownTop + 4.0f + MapSizeInputSpacing
        },
        sf::Vector2f{285.0f, MapSizeInputHeight},
        inputColor,
        "Height (8-60)",
        15,
        std::to_string(_draftMapHeight)
    );
    _mapHeightInput->setNumericOnly(true);
    _mapHeightInput->setMaxLength(2);
    _mapHeightInput->setValueCallback([this](const std::string& value) {
        if (!value.empty()) {
            _draftMapHeight = std::clamp(
                std::stoi(value),
                MinimumMapHeight,
                MaximumMapHeight
            );
        }
    });
    _mapSizeMenu.addButton(_mapHeightInput);
}

void MapEditorScene::tryAutoApplyMapSize() {
    if (!_mapSizeExpanded || !_mapWidthInput || !_mapHeightInput
        || _mapWidthInput->isEditing() || _mapHeightInput->isEditing()) {
        return;
    }

    const std::string widthValue = _mapWidthInput->getValue();
    const std::string heightValue = _mapHeightInput->getValue();
    if (widthValue.empty() || heightValue.empty()) {
        return;
    }

    const int width = std::stoi(widthValue);
    const int height = std::stoi(heightValue);
    if (width < MinimumMapWidth || width > MaximumMapWidth
        || height < MinimumMapHeight || height > MaximumMapHeight) {
        return;
    }

    _draftMapWidth = width;
    _draftMapHeight = height;
    applyMapSize();
}

void MapEditorScene::closeMapSizeDropdown() {
    _mapSizeExpanded = false;
    _mapSizeMenu.clear();
    _mapWidthInput.reset();
    _mapHeightInput.reset();
}

void MapEditorScene::cycleLuckyOption(std::size_t optionIndex) {
    static const std::vector<std::string> itemKeys = {
        "Coin",
        "SuperMushroom",
        "OneUpMushroom",
        "FireFlower",
        "SuperStar",
        "MegaMushroom",
        "MegaCoin"
    };
    if (optionIndex >= itemKeys.size()) {
        return;
    }

    const auto optionIt = std::find_if(
        _draftPlacement.luckyOptions.begin(),
        _draftPlacement.luckyOptions.end(),
        [&itemKey = itemKeys[optionIndex]](const LuckyOptionData& option) {
            return option.itemTypeKey == itemKey;
        }
    );
    if (optionIt == _draftPlacement.luckyOptions.end()) {
        _draftPlacement.luckyOptions.push_back({itemKeys[optionIndex], 1.0f});
    } else {
        _draftPlacement.luckyOptions.erase(optionIt);
    }
}

void MapEditorScene::cycleLuckyCapacity() {
    _draftPlacement.luckyCapacity = _draftPlacement.luckyCapacity % 5 + 1;
    refreshConfigMenu();
}

void MapEditorScene::cyclePipeOrientation() {
    _draftPlacement.pipeOrientation =
        _draftPlacement.pipeOrientation == "vertical"
        ? "horizontal"
        : "vertical";
    if (_draftPlacement.pipeOrientation == "vertical") {
        if (_draftPlacement.pipeEndSide != "top"
            && _draftPlacement.pipeEndSide != "bottom") {
            _draftPlacement.pipeEndSide = "top";
        }
    } else if (_draftPlacement.pipeEndSide != "left"
               && _draftPlacement.pipeEndSide != "right") {
        _draftPlacement.pipeEndSide = "right";
    }
    refreshConfigMenu();
}

void MapEditorScene::cyclePipeEndSide() {
    if (_draftPlacement.pipeOrientation == "horizontal") {
        _draftPlacement.pipeEndSide =
            _draftPlacement.pipeEndSide == "left" ? "right" : "left";
    } else {
        _draftPlacement.pipeEndSide =
            _draftPlacement.pipeEndSide == "top" ? "bottom" : "top";
    }
    refreshConfigMenu();
}

void MapEditorScene::cyclePipeLength() {
    _draftPlacement.pipeBodyLength =
        _draftPlacement.pipeBodyLength % 6 + 1;
    refreshConfigMenu();
}

void MapEditorScene::togglePipeWarp() {
    _draftPlacement.pipeIsWarp = !_draftPlacement.pipeIsWarp;
    refreshConfigMenu();
}

void MapEditorScene::cyclePipeWarpID() {
    _draftPlacement.warpID = _draftPlacement.warpID % 20 + 1;
    refreshConfigMenu();
}

void MapEditorScene::cyclePipeWarpTarget() {
    _draftPlacement.warpTarget = _draftPlacement.warpTarget % 20 + 1;
    refreshConfigMenu();
}

void MapEditorScene::togglePipePiranha() {
    _draftPlacement.pipeContainsPiranha =
        !_draftPlacement.pipeContainsPiranha;
    refreshConfigMenu();
}

void MapEditorScene::togglePipeContentsStatic() {
    _draftPlacement.pipeContentsStatic =
        !_draftPlacement.pipeContentsStatic;
    refreshConfigMenu();
}

void MapEditorScene::resizeMap(int width, int height, bool preserveCells) {
    width = std::clamp(width, MinimumMapWidth, MaximumMapWidth);
    height = std::clamp(height, MinimumMapHeight, MaximumMapHeight);

    const int oldWidth = MapWidth;
    const int oldHeight = MapHeight;
    std::vector<char> oldCells = std::move(_cells);
    std::vector<std::optional<CellPlacement>> oldPlacements =
        std::move(_cellPlacements);

    MapWidth = width;
    MapHeight = height;
    _cells.assign(
        static_cast<std::size_t>(MapWidth * MapHeight),
        '.'
    );
    _cellPlacements.assign(
        static_cast<std::size_t>(MapWidth * MapHeight),
        std::nullopt
    );

    if (preserveCells) {
        const int copiedWidth = std::min(oldWidth, MapWidth);
        const int copiedHeight = std::min(oldHeight, MapHeight);
        for (int row = 0; row < copiedHeight; ++row) {
            for (int column = 0; column < copiedWidth; ++column) {
                const std::size_t oldIndex = static_cast<std::size_t>(
                    row * oldWidth + column
                );
                const std::size_t newIndex = static_cast<std::size_t>(
                    row * MapWidth + column
                );
                if (oldPlacements[oldIndex].has_value()) {
                    const auto footprint = placementFootprint(
                        *oldPlacements[oldIndex],
                        column,
                        row
                    );
                    const bool fits = std::all_of(
                        footprint.begin(),
                        footprint.end(),
                        [this](const sf::Vector2i& cell) {
                            return cell.x >= 0 && cell.x < MapWidth
                                && cell.y >= 0 && cell.y < MapHeight;
                        }
                    );
                    if (!fits) {
                        continue;
                    }
                    _cellPlacements[newIndex] = oldPlacements[oldIndex];
                }
                _cells[newIndex] = oldCells[oldIndex];
            }
        }
    }

    _gridBackdrop.setSize({MapWidth * CellSize, MapHeight * CellSize});
    _gridBackdrop.setPosition({0.0f, 0.0f});
    if (_hoverColumn >= MapWidth || _hoverRow >= MapHeight) {
        _hoverColumn = -1;
        _hoverRow = -1;
    }
    if (_dragStartColumn >= MapWidth || _dragStartRow >= MapHeight) {
        _dragStartColumn = -1;
        _dragStartRow = -1;
    }
    clampMapView();
}

void MapEditorScene::applyMapSize() {
    const int width = std::clamp(
        _draftMapWidth,
        MinimumMapWidth,
        MaximumMapWidth
    );
    const int height = std::clamp(
        _draftMapHeight,
        MinimumMapHeight,
        MaximumMapHeight
    );
    if (width == MapWidth && height == MapHeight) {
        closeMapSizeDropdown();
        _configMode = ConfigMode::None;
        _configMenu.clear();
        setStatus("Map size unchanged");
        return;
    }

    rememberBeforeEdit();
    resizeMap(width, height, true);
    _dirty = true;
    closeMapSizeDropdown();
    _configMode = ConfigMode::None;
    _configMenu.clear();
    setStatus(
        "Map resized to " + std::to_string(MapWidth)
        + " x " + std::to_string(MapHeight) + " cells"
    );
}

void MapEditorScene::confirmConfig() {
    if (_configMode == ConfigMode::LuckyBlock
        && _draftPlacement.luckyOptions.empty()) {
        setStatus(
            "Select at least one lucky block outcome",
            sf::Color(255, 190, 120)
        );
        return;
    }

    if (_configMode == ConfigMode::MapSize) {
        applyMapSize();
        return;
    }

    _selectedPlacement = _draftPlacement;
    if (_configMode == ConfigMode::LuckyBlock) {
        _selectedSymbol = '?';
    } else if (_configMode == ConfigMode::CoinBlock) {
        _selectedSymbol = 'B';
    } else {
        _selectedSymbol = 'V';
    }
    const PaletteEntry* entry = findEntry(_selectedSymbol);
    if (entry != nullptr) {
        _selectedText.setString(
            "Selected: " + entry->label + " (configured)"
        );
    }
    _configMode = ConfigMode::None;
    _configMenu.clear();
    _placementBeforeConfig.reset();
    setStatus("Placement configured; click a map cell to place it");
}

void MapEditorScene::cancelConfig() {
    if (_configMode == ConfigMode::MapSize) {
        _configMode = ConfigMode::None;
        _configMenu.clear();
        setStatus("Map size change cancelled");
        return;
    }
    _configMode = ConfigMode::None;
    _configMenu.clear();
    _selectedSymbol = _selectionBeforeConfig;
    _selectedPlacement = _placementBeforeConfig;
    if (const PaletteEntry* entry = findEntry(_selectedSymbol)) {
        _selectedText.setString(
            "Selected: " + entry->label
            + " (" + std::string(1, entry->symbol) + ")"
        );
    }
    setStatus("Placement configuration cancelled");
}

void MapEditorScene::refreshThemeButton() {
    if (_themeDropdown && !_themeOptions.empty()) {
        _themeDropdown->setSelectedIndex(_themeIndex);
    }
}

void MapEditorScene::cycleTheme() {
    if (_themeOptions.empty()) {
        return;
    }

    setThemeIndex((_themeIndex + 1) % _themeOptions.size(), true);
}

void MapEditorScene::setThemeIndex(std::size_t index, bool markDirty) {
    if (_themeOptions.empty()) {
        return;
    }
    _themeIndex = std::min(index, _themeOptions.size() - 1);
    _themeKey = _themeOptions[_themeIndex].key;
    if (markDirty) {
        _dirty = true;
        setStatus("Theme: " + _themeOptions[_themeIndex].label);
    }
    refreshThemeButton();
    setupPaletteMenu();
}

void MapEditorScene::applyLoadedTheme(
    const std::string& theme,
    const std::string& background,
    const std::string& music
) {
    if (_themeOptions.empty()) {
        return;
    }

    const auto themeMatches = [&theme, &background, &music](
        const ThemeChoice& choice
    ) {
        return (!theme.empty() && choice.key == theme)
            || (theme.empty()
                && (choice.background == background || choice.music == music));
    };
    const auto themeIt = std::find_if(
        _themeOptions.begin(),
        _themeOptions.end(),
        themeMatches
    );
    _themeIndex = themeIt == _themeOptions.end()
        ? 0
        : static_cast<std::size_t>(
            std::distance(_themeOptions.begin(), themeIt)
        );
    _themeKey = _themeOptions[_themeIndex].key;
    refreshThemeButton();
    setupPaletteMenu();
}

void MapEditorScene::updateScrollVisuals() {
    const auto contentHeight = [](std::size_t count, float spacing, float height) {
        return count == 0
            ? 0.0f
            : static_cast<float>(count - 1) * spacing + height;
    };
    const float paletteContentHeight = contentHeight(
        _paletteMenu.size(),
        PaletteButtonSpacing,
        PaletteButtonHeight
    );
    const float paletteMaximumScroll = std::max(
        0.0f,
        paletteContentHeight - PaletteViewportHeight
    );
    const float paletteThumbHeight = paletteMaximumScroll <= 0.0f
        ? PaletteViewportHeight
        : std::max(
            38.0f,
            PaletteViewportHeight * PaletteViewportHeight
                / std::max(paletteContentHeight, PaletteViewportHeight)
        );
    _paletteScrollTrack.setSize({10.0f, PaletteViewportHeight});
    _paletteScrollThumb.setSize({10.0f, paletteThumbHeight});
    const float paletteTravel = PaletteViewportHeight - paletteThumbHeight;
    _paletteScrollThumb.setPosition({
        PaletteScrollX,
        PaletteViewportTop
            + (paletteMaximumScroll <= 0.0f
                ? 0.0f
                : _paletteScrollOffset / paletteMaximumScroll * paletteTravel)
    });

    const float configContentHeight = contentHeight(
        _configMenu.size(),
        ConfigButtonSpacing,
        ConfigButtonHeight
    );
    const float configMaximumScroll = std::max(
        0.0f,
        configContentHeight - ConfigViewportHeight
    );
    const float configThumbHeight = configMaximumScroll <= 0.0f
        ? ConfigViewportHeight
        : std::max(
            38.0f,
            ConfigViewportHeight * ConfigViewportHeight
                / std::max(configContentHeight, ConfigViewportHeight)
        );
    _configScrollTrack.setSize({10.0f, ConfigViewportHeight});
    _configScrollThumb.setSize({10.0f, configThumbHeight});
    const float configTravel = ConfigViewportHeight - configThumbHeight;
    _configScrollThumb.setPosition({
        ConfigScrollX,
        ConfigViewportTop
            + (configMaximumScroll <= 0.0f
                ? 0.0f
                : _configScrollOffset / configMaximumScroll * configTravel)
    });
}

void MapEditorScene::scrollPalette(float wheelDelta) {
    const float contentHeight = _paletteMenu.size() == 0
        ? 0.0f
        : static_cast<float>(_paletteMenu.size() - 1) * PaletteButtonSpacing
            + PaletteButtonHeight;
    const float maximumScroll = std::max(
        0.0f,
        contentHeight - PaletteViewportHeight
    );
    _paletteScrollOffset = std::clamp(
        _paletteScrollOffset - wheelDelta * PaletteButtonSpacing,
        0.0f,
        maximumScroll
    );
    _paletteMenu.setLayoutProperties(
        {PaletteLeft, PaletteViewportTop - _paletteScrollOffset},
        {285.0f, PaletteButtonHeight},
        PaletteButtonSpacing,
        false,
        categoryColor(_activeCategory),
        18
    );
    updateScrollVisuals();
}

void MapEditorScene::scrollConfig(float wheelDelta) {
    const float contentHeight = _configMenu.size() == 0
        ? 0.0f
        : static_cast<float>(_configMenu.size() - 1) * ConfigButtonSpacing
            + ConfigButtonHeight;
    const float maximumScroll = std::max(
        0.0f,
        contentHeight - ConfigViewportHeight
    );
    _configScrollOffset = std::clamp(
        _configScrollOffset - wheelDelta * ConfigButtonSpacing,
        0.0f,
        maximumScroll
    );
    _configMenu.setLayoutProperties(
        {620.0f, ConfigViewportTop - _configScrollOffset},
        {680.0f, ConfigButtonHeight},
        ConfigButtonSpacing,
        false,
        sf::Color(53, 91, 130),
        18
    );
    updateScrollVisuals();
}

void MapEditorScene::setupActionMenu() {
    _actionMenu.clear();
    _actionMenu.setLayoutProperties(
        {1605.0f, 752.0f},
        {285.0f, 40.0f},
        42.0f,
        false,
        sf::Color(53, 91, 130),
        16
    );

    _actionMenu.addButtonAuto(
        "Save Map",
        16,
        std::make_unique<FunctionalCommand>(
            "Save Map", [this]() { saveMap(); }
        )
    );
    _actionMenu.addButtonAuto(
        "Save & Play",
        16,
        std::make_unique<FunctionalCommand>(
            "Save & Play", [this]() { saveAndPlay(); }
        )
    );
    _actionMenu.addButtonAuto(
        "Undo",
        16,
        std::make_unique<FunctionalCommand>(
            "Undo", [this]() { undoLastEdit(); }
        )
    );
    _actionMenu.addButtonAuto(
        "Redo",
        16,
        std::make_unique<FunctionalCommand>(
            "Redo", [this]() { redoLastEdit(); }
        )
    );
    _actionMenu.addButtonAuto(
        "Clear Map",
        16,
        std::make_unique<FunctionalCommand>(
            "Clear Map", [this]() { clearMap(); }
        )
    );
    _actionMenu.addButtonAuto(
        "Instructions",
        16,
        std::make_unique<FunctionalCommand>(
            "Instructions", [this]() { _showInstructions = true; }
        )
    );
    _actionMenu.addButtonAuto(
        "Back",
        16,
        std::make_unique<FunctionalCommand>(
            "Back", [this]() {
                if (auto* manager = getSceneManager()) {
                    manager->requestPopScene();
                }
            }
        )
    );
}

void MapEditorScene::handleMouseMoved(
    const sf::Event::MouseMoved& mouseEvent
) {
    if (_middleMouseHeld) {
        panMap(mouseEvent.position);
    }

    updateHover(mouseEvent.position);
    if (_paintActive && !_middleMouseHeld) {
        continuePaint(mouseEvent.position);
    }
}

void MapEditorScene::handleMousePressed(
    const sf::Event::MouseButtonPressed& mouseEvent
) {
    switch (mouseEvent.button) {
        case sf::Mouse::Button::Left:
            _leftMouseHeld = true;
            beginPaint(mouseEvent.button, mouseEvent.position);
            break;
        case sf::Mouse::Button::Right:
            _rightMouseHeld = true;
            beginPaint(mouseEvent.button, mouseEvent.position);
            break;
        case sf::Mouse::Button::Middle:
            if (isMapCanvasPosition(mouseEvent.position)) {
                _middleMouseHeld = true;
                _lastMousePosition = mouseEvent.position;
            }
            break;
        default:
            break;
    }
}

void MapEditorScene::handleMouseReleased(
    const sf::Event::MouseButtonReleased& mouseEvent
) {
    switch (mouseEvent.button) {
        case sf::Mouse::Button::Left:
            _leftMouseHeld = false;
            break;
        case sf::Mouse::Button::Right:
            _rightMouseHeld = false;
            break;
        case sf::Mouse::Button::Middle:
            _middleMouseHeld = false;
            break;
        default:
            break;
    }

    endPaint(mouseEvent.button);
}

void MapEditorScene::updateHover(sf::Vector2i screenPosition) {
    const std::optional<sf::Vector2i> cell = mapCellAtScreen(screenPosition);
    if (!cell.has_value()) {
        _hoverColumn = -1;
        _hoverRow = -1;
        return;
    }

    _hoverColumn = cell->x;
    _hoverRow = cell->y;
}

void MapEditorScene::beginPaint(
    sf::Mouse::Button button,
    sf::Vector2i screenPosition
) {
    if (_middleMouseHeld) {
        _paintActive = false;
        return;
    }

    const std::optional<sf::Vector2i> cell = mapCellAtScreen(screenPosition);
    if (!cell.has_value()) {
        _paintActive = false;
        _rectangleDrag = false;
        return;
    }

    _paintButton = button;
    _dragStartColumn = cell->x;
    _dragStartRow = cell->y;
    _rectangleDrag = isShiftHeld();
    _strokeUndoCaptured = false;
    _paintActive = true;
    updateHover(screenPosition);
    continuePaint(screenPosition);
}

void MapEditorScene::continuePaint(sf::Vector2i screenPosition) {
    if (!_paintActive) {
        return;
    }

    const std::optional<sf::Vector2i> cell = mapCellAtScreen(screenPosition);
    if (!cell.has_value()) {
        return;
    }

    const char symbol = _paintButton == sf::Mouse::Button::Left
        ? _selectedSymbol
        : '.';
    const std::optional<CellPlacement> placement =
        _paintButton == sf::Mouse::Button::Left
        ? _selectedPlacement
        : std::nullopt;
    if (_rectangleDrag) {
        applyPaintRectangle(
            _dragStartColumn,
            _dragStartRow,
            cell->x,
            cell->y,
            symbol,
            placement
        );
    } else {
        applyPaintCell(cell->x, cell->y, symbol, placement);
    }
}

void MapEditorScene::endPaint(sf::Mouse::Button button) {
    if (button != _paintButton) {
        return;
    }

    _paintActive = false;
    _rectangleDrag = false;
    _strokeUndoCaptured = false;
    _dragStartColumn = -1;
    _dragStartRow = -1;
}

void MapEditorScene::panMap(sf::Vector2i screenPosition) {
    const sf::Vector2i delta = screenPosition - _lastMousePosition;
    const sf::Vector2f viewSize = _mapView.getSize();
    _mapView.move({
        -static_cast<float>(delta.x) * viewSize.x / MapViewportWidth,
        -static_cast<float>(delta.y) * viewSize.y / MapViewportHeight
    });
    clampMapView();
    _lastMousePosition = screenPosition;
}

void MapEditorScene::zoomMap(
    float wheelDelta,
    sf::Vector2i screenPosition
) {
    if (!isMapCanvasPosition(screenPosition) || wheelDelta == 0.0f) {
        return;
    }

    const sf::Vector2f worldBeforeZoom = mapScreenToWorld({
        static_cast<float>(screenPosition.x),
        static_cast<float>(screenPosition.y)
    });

    _zoom = std::clamp(
        _zoom * std::pow(1.15f, wheelDelta),
        MinimumZoom,
        MaximumZoom
    );
    _mapView.setSize({
        MapViewportWidth / _zoom,
        MapViewportHeight / _zoom
    });

    const sf::Vector2f worldAfterZoom = mapScreenToWorld({
        static_cast<float>(screenPosition.x),
        static_cast<float>(screenPosition.y)
    });
    _mapView.move(worldBeforeZoom - worldAfterZoom);
    clampMapView();

    setStatus(
        "Zoom: " + std::to_string(static_cast<int>(std::round(_zoom * 100.0f))) + "%"
    );
}

void MapEditorScene::clampMapView() {
    const sf::Vector2f mapSize = {
        MapWidth * CellSize,
        MapHeight * CellSize
    };
    sf::Vector2f center = _mapView.getCenter();

    // Keep a generous empty margin around the map so the editor can inspect
    // the map boundary from outside it, while still preventing the canvas
    // from being dragged indefinitely away from the level.
    center.x = std::clamp(
        center.x,
        -CameraPanMargin,
        mapSize.x + CameraPanMargin
    );
    center.y = std::clamp(
        center.y,
        -CameraPanMargin,
        mapSize.y + CameraPanMargin
    );

    _mapView.setCenter(center);
}

sf::Vector2f MapEditorScene::mapScreenToWorld(
    sf::Vector2f screenPosition
) const {
    const sf::Vector2f viewSize = _mapView.getSize();
    const sf::Vector2f viewTopLeft = _mapView.getCenter() - viewSize * 0.5f;
    return viewTopLeft + sf::Vector2f{
        screenPosition.x / MapViewportWidth * viewSize.x,
        screenPosition.y / MapViewportHeight * viewSize.y
    };
}

std::optional<sf::Vector2i> MapEditorScene::mapCellAtScreen(
    sf::Vector2i screenPosition
) const {
    if (!isMapCanvasPosition(screenPosition)) {
        return std::nullopt;
    }

    const sf::Vector2f worldPosition = mapScreenToWorld({
        static_cast<float>(screenPosition.x),
        static_cast<float>(screenPosition.y)
    });
    const int column = static_cast<int>(std::floor(worldPosition.x / CellSize));
    const int row = static_cast<int>(std::floor(worldPosition.y / CellSize));
    if (column < 0 || column >= MapWidth || row < 0 || row >= MapHeight) {
        return std::nullopt;
    }

    return sf::Vector2i{column, row};
}

bool MapEditorScene::isMapCanvasPosition(sf::Vector2i screenPosition) const {
    if (screenPosition.x < 0 || screenPosition.x >= MapViewportWidth
        || screenPosition.y < 0 || screenPosition.y >= MapViewportHeight) {
        return false;
    }
    return true;
}

bool MapEditorScene::isShiftHeld() const {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
}

void MapEditorScene::drawMap(sf::RenderTarget& target) {
    target.setView(_mapView);
    target.draw(_gridBackdrop);
    drawMapGrid(target);

    const sf::FloatRect viewBounds(
        _mapView.getCenter() - _mapView.getSize() * 0.5f,
        _mapView.getSize()
    );
    const int startColumn = std::max(
        0,
        static_cast<int>(std::floor(viewBounds.position.x / CellSize)) - 1
    );
    const int endColumn = std::min(
        MapWidth - 1,
        static_cast<int>(std::ceil(
            (viewBounds.position.x + viewBounds.size.x) / CellSize
        )) + 1
    );
    const int startRow = std::max(
        0,
        static_cast<int>(std::floor(viewBounds.position.y / CellSize)) - 1
    );
    const int endRow = std::min(
        MapHeight - 1,
        static_cast<int>(std::ceil(
            (viewBounds.position.y + viewBounds.size.y) / CellSize
        )) + 1
    );

    for (int row = startRow; row <= endRow; ++row) {
        for (int column = startColumn; column <= endColumn; ++column) {
            const std::size_t index = static_cast<std::size_t>(
                row * MapWidth + column
            );
            if (_cells[index] == '.' || _cellPlacements[index].has_value()) {
                continue;
            }

            const PaletteEntry* entry = findEntry(_cells[index]);
            if (entry != nullptr) {
                drawEntrySprite(
                    target,
                    *entry,
                    {
                        column * CellSize + CellSize * 0.5f,
                        row * CellSize + CellSize * 0.5f
                    }
                );
            }
        }
    }

    for (std::size_t index = 0; index < _cellPlacements.size(); ++index) {
        if (!_cellPlacements[index].has_value()) {
            continue;
        }
        const int column = static_cast<int>(
            index % static_cast<std::size_t>(MapWidth)
        );
        const int row = static_cast<int>(
            index / static_cast<std::size_t>(MapWidth)
        );
        const auto footprint = placementFootprint(
            *_cellPlacements[index],
            column,
            row
        );
        const bool visible = std::any_of(
            footprint.begin(),
            footprint.end(),
            [&viewBounds](const sf::Vector2i& cell) {
                const sf::FloatRect cellBounds(
                    {
                        cell.x * CellSize,
                        cell.y * CellSize
                    },
                    {CellSize, CellSize}
                );
                return cellBounds.findIntersection(viewBounds).has_value();
            }
        );
        if (!visible) {
            continue;
        }
        drawPlacement(
            target,
            *_cellPlacements[index],
            _cells[index],
            column,
            row
        );
    }

    if (_rectangleDrag && _paintActive
        && _hoverColumn >= 0 && _hoverRow >= 0) {
        const int left = std::min(_dragStartColumn, _hoverColumn);
        const int top = std::min(_dragStartRow, _hoverRow);
        const int right = std::max(_dragStartColumn, _hoverColumn);
        const int bottom = std::max(_dragStartRow, _hoverRow);
        sf::RectangleShape selection({
            (right - left + 1) * CellSize,
            (bottom - top + 1) * CellSize
        });
        selection.setPosition({left * CellSize, top * CellSize});
        selection.setFillColor(
            _paintButton == sf::Mouse::Button::Left
                ? sf::Color(255, 235, 100, 45)
                : sf::Color(255, 100, 100, 45)
        );
        selection.setOutlineThickness(3.0f);
        selection.setOutlineColor(
            _paintButton == sf::Mouse::Button::Left
                ? sf::Color(255, 235, 100)
                : sf::Color(255, 130, 130)
        );
        target.draw(selection);
    }

    if (_hoverColumn >= 0 && _hoverRow >= 0) {
        sf::RectangleShape hover({CellSize - 4.0f, CellSize - 4.0f});
        hover.setPosition({
            _hoverColumn * CellSize + 2.0f,
            _hoverRow * CellSize + 2.0f
        });
        hover.setFillColor(sf::Color(255, 255, 255, 35));
        hover.setOutlineThickness(3.0f);
        hover.setOutlineColor(sf::Color(255, 235, 100));
        target.draw(hover);
    }
}

MapEditorScene::PreviewSpec MapEditorScene::previewSpecFor(
    const PaletteEntry& entry
) const {
    PreviewSpec spec;
    spec.textureKey = "mario_and_items";
    spec.size = {CellSize, CellSize};

    if (entry.prefabId == "brick" || entry.prefabId == "breakable_brick") {
        spec.textureKey = ThemeAssets::brickTextureAlias(_themeKey);
        spec.animationId = "brick";
    } else if (entry.prefabId == "terrain_grassland") {
        if (_themeKey == "underground") {
            spec.textureKey = "at_underground";
            spec.textureRect = {{1, 18}, {16, 16}};
        } else {
            spec.textureKey = "at_grassland";
            spec.textureRect = {{52, 86}, {16, 16}};
        }
    } else if (entry.prefabId == "block_coin") {
        spec.textureKey = ThemeAssets::brickTextureAlias(_themeKey);
        spec.animationId = "coin_block";
    } else if (entry.prefabId == "block_lucky") {
        spec.textureKey = ThemeAssets::luckyBlockTextureAlias(_themeKey);
        spec.animationId = "lucky_block";
    } else if (entry.prefabId == "item_coin") {
        spec.textureKey = "coin_spritesheet";
        spec.animationId = "coin";
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_fire_flower") {
        spec.animationId = "fire_flower";
        spec.size = {54.0f, 54.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_super_mushroom") {
        spec.animationId = "super_mushroom";
        spec.size = {54.0f, 54.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_one_up_mushroom") {
        spec.animationId = "one_up_mushroom";
        spec.size = {54.0f, 54.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_mega_mushroom") {
        spec.textureKey = "mega_mushroom_spritesheet";
        spec.animationId = "mega_mushroom";
        spec.size = {384.0f, 308.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_super_star") {
        spec.animationId = "super_star";
        spec.size = {54.0f, 54.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_mega_coin") {
        spec.textureKey = "mega_coin_spritesheet";
        spec.animationId = "mega_coin";
        spec.size = {96.0f, 96.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_flagpole") {
        spec.textureKey = "goal_flag_spritesheet";
        spec.animationId = "flagpole";
        spec.size = {128.0f, 896.0f};
        spec.centerVertically = true;
    } else if (entry.prefabId == "item_checkpoint_flag") {
        spec.textureKey = "checkpoint_flag_spritesheet";
        spec.animationId = "checkpoint_flag";
        spec.size = {48.0f, 96.0f};
        spec.offset = {0.0f, -16.0f};
    } else if (entry.prefabId == "enemy_goomba") {
        spec.textureKey = "goomba_spritesheet";
        spec.animationId = "goomba";
        spec.size = {50.0f, 65.0f};
        spec.offset = {0.0f, 5.0f};
        spec.alignToCellBottom = true;
    } else if (entry.prefabId == "enemy_koopa") {
        spec.textureKey = "koopa_spritesheet";
        spec.animationId = "koopa";
        spec.size = {64.0f, 100.0f};
        spec.visualScale = {
            Koopa::defaultVisualScaleX,
            Koopa::defaultVisualScaleY
        };
        spec.alignToCellBottom = true;
    } else if (entry.prefabId == "enemy_piranha_plant") {
        spec.textureKey = "piranha_plant_spritesheet";
        spec.animationId = "piranha_plant";
        spec.size = {78.0f, 105.0f};
        spec.offset = {0.0f, 5.0f};
        spec.alignToCellBottom = true;
    } else if (entry.prefabId == "player_mario") {
        spec.textureKey = "mario_spritesheet";
        spec.animationId = "mario";
        spec.size = {36.0f, 80.0f};
        spec.visualScale = {
            Player::defaultVisualScaleX,
            Player::defaultVisualScaleY
        };
        spec.offset = {10.0f, 0.0f};
        spec.alignToCellBottom = true;
    } else if (entry.prefabId == "player_luigi") {
        spec.textureKey = "luigi_spritesheet";
        spec.animationId = "luigi";
        spec.size = {36.0f, 80.0f};
        spec.visualScale = {
            Player::defaultVisualScaleX,
            Player::defaultVisualScaleY
        };
        spec.offset = {10.0f, 0.0f};
        spec.alignToCellBottom = true;
    }
    return spec;
}

sf::IntRect MapEditorScene::firstAnimationFrame(
    const std::string& animationId
) const {
    if (animationId.empty()) {
        return {};
    }
    try {
        const AnimationSet& set = AnimationLibrary::getInstance().getAnimationSet(
            animationId
        );
        const auto clipIt = set.clips.find(set.defaultClip);
        if (clipIt != set.clips.end() && !clipIt->second.isEmpty()) {
            return clipIt->second.getFrame(0).rect;
        }
    } catch (const std::exception&) {
        return {};
    }
    return {};
}

void MapEditorScene::drawPreviewSprite(
    sf::RenderTarget& target,
    const PreviewSpec& spec,
    sf::Vector2f cellCenter
) const {
    try {
        sf::Texture& texture = ResourceManager::getInstance().getTexture(
            spec.textureKey
        );
        const bool isSynchronizedBrickAnimation =
            spec.animationId == "brick" || spec.animationId == "coin_block";
        sf::IntRect frame;
        if (isSynchronizedBrickAnimation) {
            frame = Animation::getBrickAnimationFrameRect();
        } else if (spec.animationId.empty()) {
            frame = spec.textureRect;
        } else {
            frame = firstAnimationFrame(spec.animationId);
        }
        if (frame.size.x <= 0 || frame.size.y <= 0) {
            frame = {
                {0, 0},
                {
                    static_cast<int>(texture.getSize().x),
                    static_cast<int>(texture.getSize().y)
                }
            };
        }
        if (frame.size.x <= 0 || frame.size.y <= 0) {
            return;
        }

        // Spawn sizes describe the body's dimensions. Some objects, such as
        // players, intentionally use a separate presentation scale; keep the
        // body anchor unchanged while rendering the sprite at its actual
        // in-game size.
        const sf::Vector2f renderSize{
            spec.size.x * spec.visualScale.x,
            spec.size.y * spec.visualScale.y
        };
        sf::Sprite sprite(texture, frame);
        sprite.setOrigin({
            frame.size.x * 0.5f,
            static_cast<float>(frame.size.y)
        });
        sf::Vector2f bodyCenter = cellCenter + spec.offset;
        if (spec.alignToCellBottom) {
            bodyCenter.y = cellCenter.y
                + CellSize * 0.5f
                - spec.size.y * 0.5f;
        } else if (spec.centerVertically) {
            bodyCenter.y += (CellSize - spec.size.y) * 0.5f;
        }
        sprite.setPosition({bodyCenter.x, bodyCenter.y + spec.size.y * 0.5f});
        sprite.setScale({
            renderSize.x / static_cast<float>(frame.size.x),
            renderSize.y / static_cast<float>(frame.size.y)
        });
        target.draw(sprite);
    } catch (const std::exception&) {
        // A missing optional preview asset should not make the editor unusable.
    }
}

void MapEditorScene::drawEntrySprite(
    sf::RenderTarget& target,
    const PaletteEntry& entry,
    sf::Vector2f cellCenter
) const {
    drawPreviewSprite(target, previewSpecFor(entry), cellCenter);
}

void MapEditorScene::drawInvisibleLuckyBlockMarker(
    sf::RenderTarget& target,
    int column,
    int row
) const {
    const sf::Vector2f cellTopLeft{
        column * CellSize,
        row * CellSize
    };

    sf::RectangleShape marker({CellSize - 8.0f, CellSize - 8.0f});
    marker.setPosition(cellTopLeft + sf::Vector2f{4.0f, 4.0f});
    marker.setFillColor(sf::Color(80, 180, 255, 35));
    marker.setOutlineThickness(2.0f);
    marker.setOutlineColor(sf::Color(150, 220, 255, 220));
    target.draw(marker);

    sf::Text markerLabel(
        ResourceManager::getInstance().getFont("moon_get"),
        "INV",
        14
    );
    const sf::FloatRect labelBounds = markerLabel.getLocalBounds();
    markerLabel.setOrigin({
        labelBounds.position.x + labelBounds.size.x * 0.5f,
        labelBounds.position.y + labelBounds.size.y * 0.5f
    });
    markerLabel.setPosition({
        cellTopLeft.x + CellSize * 0.5f,
        cellTopLeft.y + CellSize * 0.5f
    });
    markerLabel.setFillColor(sf::Color(225, 245, 255));
    markerLabel.setOutlineColor(sf::Color(10, 35, 60));
    markerLabel.setOutlineThickness(1.0f);
    target.draw(markerLabel);
}

void MapEditorScene::drawPlacement(
    sf::RenderTarget& target,
    const CellPlacement& placement,
    char symbol,
    int column,
    int row
) const {
    if (placement.prefabId == "pipe_basic") {
        drawPipePreview(target, placement, column, row);
        return;
    }
    const PaletteEntry* entry = findEntry(symbol);
    if (entry == nullptr) {
        return;
    }

    PreviewSpec spec = previewSpecFor(*entry);
    if (placement.prefabId == "block_lucky") {
        if (placement.luckyTexture == "invisible") {
            drawInvisibleLuckyBlockMarker(target, column, row);
            return;
        }
        if (placement.luckyTexture == "brick") {
            spec.textureKey = ThemeAssets::brickTextureAlias(_themeKey);
            spec.animationId = "brick";
        }
    }
    drawPreviewSprite(
            target,
            spec,
            {
                column * CellSize + CellSize * 0.5f,
                row * CellSize + CellSize * 0.5f
            }
    );
}

void MapEditorScene::drawPipePreview(
    sf::RenderTarget& target,
    const CellPlacement& placement,
    int column,
    int row
) const {
    try {
        sf::Texture& texture = ResourceManager::getInstance().getTexture(
            "pipes_spritesheet"
        );
        const bool vertical = placement.pipeOrientation != "horizontal";
        const int bodyLength = std::max(placement.pipeBodyLength, 0);
        const int totalTiles = 1 + bodyLength;
        const Pipe::Orientation orientation = vertical
            ? Pipe::Orientation::Vertical
            : Pipe::Orientation::Horizontal;
        const sf::Vector2f pipeSize = Pipe::computePipeSize(
            orientation,
            bodyLength,
            CellSize
        );
        const sf::Vector2f cellCenter = {
            column * CellSize + CellSize * 0.5f,
            row * CellSize + CellSize * 0.5f
        };
        sf::Vector2f pipePosition = cellCenter;
        const float halfCell = CellSize * 0.5f;
        if (vertical) {
            pipePosition.x += halfCell;
            if (placement.pipeEndSide == "bottom") {
                pipePosition.y = cellCenter.y - halfCell
                    + pipeSize.y * 0.5f;
            } else {
                pipePosition.y = cellCenter.y + halfCell
                    - pipeSize.y * 0.5f;
            }
        } else {
            pipePosition.y = cellCenter.y - halfCell;
            if (placement.pipeEndSide == "right") {
                pipePosition.x = cellCenter.x - halfCell
                    + pipeSize.x * 0.5f;
            } else {
                pipePosition.x = cellCenter.x + halfCell
                    - pipeSize.x * 0.5f;
            }
        }

        const auto blockRect = [](int gridColumn, int gridRow) {
            return sf::IntRect(
                {(gridColumn - 1) * 17 + 1, (gridRow - 1) * 17 + 1},
                {16, 16}
            );
        };
        const auto drawTile = [&target, &texture](
            const sf::IntRect& textureRect,
            sf::Vector2f position
        ) {
            sf::Sprite sprite(texture, textureRect);
            sprite.setScale({4.0f, 4.0f});
            sprite.setPosition(position);
            target.draw(sprite);
        };

        if (vertical) {
            for (int tileRow = 0; tileRow < totalTiles; ++tileRow) {
                for (int tileColumn = 0; tileColumn < 2; ++tileColumn) {
                    const bool cap = placement.pipeEndSide == "bottom"
                        ? tileRow == totalTiles - 1
                        : tileRow == 0;
                    const int textureRow = cap
                        ? (placement.pipeEndSide == "bottom" ? 3 : 1)
                        : 2;
                    drawTile(
                        blockRect(tileColumn + 1, textureRow),
                        pipePosition + sf::Vector2f{
                            -pipeSize.x * 0.5f + tileColumn * CellSize,
                            -pipeSize.y * 0.5f + tileRow * CellSize
                        }
                    );
                }
            }
        } else {
            for (int tileColumn = 0; tileColumn < totalTiles; ++tileColumn) {
                for (int tileRow = 0; tileRow < 2; ++tileRow) {
                    const bool cap = placement.pipeEndSide == "right"
                        ? tileColumn == totalTiles - 1
                        : tileColumn == 0;
                    const int textureColumn = cap
                        ? (placement.pipeEndSide == "right" ? 5 : 3)
                        : 4;
                    drawTile(
                        blockRect(textureColumn, tileRow + 1),
                        pipePosition + sf::Vector2f{
                            -pipeSize.x * 0.5f + tileColumn * CellSize,
                            -pipeSize.y * 0.5f + tileRow * CellSize
                        }
                    );
                }
            }
        }

        if (placement.pipeContainsPiranha) {
            const PaletteEntry* plantEntry = findEntry('p');
            if (plantEntry == nullptr) {
                return;
            }
            const PreviewSpec plantSpec = previewSpecFor(*plantEntry);
            const sf::IntRect frame = firstAnimationFrame(
                plantSpec.animationId
            );
            if (frame.size.x > 0 && frame.size.y > 0) {
                sf::Texture& plantTexture =
                    ResourceManager::getInstance().getTexture(
                        plantSpec.textureKey
                    );
                sf::Sprite plant(plantTexture, frame);
                plant.setOrigin({
                    frame.size.x * 0.5f,
                    static_cast<float>(frame.size.y)
                });
                sf::Vector2f plantCenter = pipePosition;
                if (vertical && placement.pipeEndSide == "top") {
                    plantCenter = {
                        pipePosition.x,
                        pipePosition.y - pipeSize.y * 0.5f
                            - plantSpec.size.y * 0.5f
                    };
                } else if (vertical && placement.pipeEndSide == "bottom") {
                    plantCenter = {
                        pipePosition.x,
                        pipePosition.y + pipeSize.y * 0.5f
                            + plantSpec.size.y * 0.5f
                    };
                }
                plant.setPosition({
                    plantCenter.x,
                    plantCenter.y + plantSpec.size.y * 0.5f
                });
                plant.setScale({
                    plantSpec.size.x / static_cast<float>(frame.size.x),
                    plantSpec.size.y / static_cast<float>(frame.size.y)
                });
                target.draw(plant);
            }
        }
    } catch (const std::exception&) {
        // See drawEntrySprite: previews are best-effort and never block editing.
    }
}

std::vector<sf::Vector2i> MapEditorScene::placementFootprint(
    const CellPlacement& placement,
    int column,
    int row
) const {
    if (placement.prefabId != "pipe_basic") {
        return {{column, row}};
    }

    const int totalTiles = 1 + std::max(placement.pipeBodyLength, 0);
    std::vector<sf::Vector2i> footprint;
    if (placement.pipeOrientation == "horizontal") {
        const int firstColumn = placement.pipeEndSide == "right"
            ? column
            : column - totalTiles + 1;
        for (int pipeColumn = firstColumn;
             pipeColumn < firstColumn + totalTiles;
             ++pipeColumn) {
            footprint.push_back({pipeColumn, row - 1});
            footprint.push_back({pipeColumn, row});
        }
    } else {
        const int firstRow = placement.pipeEndSide == "bottom"
            ? row
            : row - totalTiles + 1;
        for (int pipeRow = firstRow;
             pipeRow < firstRow + totalTiles;
             ++pipeRow) {
            footprint.push_back({column, pipeRow});
            footprint.push_back({column + 1, pipeRow});
        }
    }
    return footprint;
}

bool MapEditorScene::placementFits(
    const CellPlacement& placement,
    int column,
    int row
) const {
    const auto footprint = placementFootprint(placement, column, row);
    return std::all_of(
        footprint.begin(),
        footprint.end(),
        [this](const sf::Vector2i& cell) {
            return cell.x >= 0 && cell.x < MapWidth
                && cell.y >= 0 && cell.y < MapHeight;
        }
    );
}

void MapEditorScene::clearOverlappingPlacements(int column, int row) {
    const sf::Vector2i target{column, row};
    for (std::size_t index = 0; index < _cellPlacements.size(); ++index) {
        if (!_cellPlacements[index].has_value()) {
            continue;
        }
        const int anchorColumn = static_cast<int>(
            index % static_cast<std::size_t>(MapWidth)
        );
        const int anchorRow = static_cast<int>(
            index / static_cast<std::size_t>(MapWidth)
        );
        const auto footprint = placementFootprint(
            *_cellPlacements[index],
            anchorColumn,
            anchorRow
        );
        if (std::find(footprint.begin(), footprint.end(), target)
            == footprint.end()) {
            continue;
        }
        _cellPlacements[index].reset();
        _cells[index] = '.';
    }
}

void MapEditorScene::drawMapGrid(sf::RenderTarget& target) {
    sf::VertexArray gridLines(sf::PrimitiveType::Lines);
    for (int column = 0; column <= MapWidth; ++column) {
        const float x = column * CellSize;
        gridLines.append(sf::Vertex({x, 0.0f}, sf::Color(80, 123, 160)));
        gridLines.append(sf::Vertex({x, MapHeight * CellSize}, sf::Color(80, 123, 160)));
    }
    for (int row = 0; row <= MapHeight; ++row) {
        const float y = row * CellSize;
        gridLines.append(sf::Vertex({0.0f, y}, sf::Color(80, 123, 160)));
        gridLines.append(sf::Vertex({MapWidth * CellSize, y}, sf::Color(80, 123, 160)));
    }
    target.draw(gridLines);
}

void MapEditorScene::selectCategory(Category category) {
    _activeCategory = category;
    setupPaletteMenu();
    _paletteTitleText.setString(
        std::string("PALETTE: ") + categoryName(category)
    );
    setStatus(
        std::string("Choose a ") + categoryName(category) + " item"
    );
}

void MapEditorScene::selectSymbol(char symbol) {
    const char previousSymbol = _selectedSymbol;
    const std::optional<CellPlacement> previousPlacement = _selectedPlacement;
    _selectedSymbol = symbol;
    const PaletteEntry* entry = findEntry(symbol);
    if (entry == nullptr) {
        _selectedPlacement.reset();
        _selectedText.setString("Selected: Eraser (.)");
        return;
    }

    if (entry->prefabId == "block_coin") {
        _selectionBeforeConfig = previousSymbol;
        _placementBeforeConfig = previousPlacement;
        openCoinBlockConfig();
        return;
    }
    if (entry->prefabId == "block_lucky") {
        _selectionBeforeConfig = previousSymbol;
        _placementBeforeConfig = previousPlacement;
        openLuckyBlockConfig();
        return;
    }
    if (entry->prefabId == "pipe_basic") {
        _selectionBeforeConfig = previousSymbol;
        _placementBeforeConfig = previousPlacement;
        openPipeConfig();
        return;
    }

    _selectedPlacement.reset();

    _selectedText.setString(
        "Selected: " + entry->label + " (" + std::string(1, entry->symbol) + ")"
    );
    setStatus("Selected " + entry->label);
}

void MapEditorScene::eraseCell(int column, int row) {
    applyPaintCell(column, row, '.', std::nullopt);
}

void MapEditorScene::placeCell(int column, int row) {
    applyPaintCell(column, row, _selectedSymbol, _selectedPlacement);
}

void MapEditorScene::applyPaintCell(
    int column,
    int row,
    char symbol,
    const std::optional<CellPlacement>& placement
) {
    if (column < 0 || column >= MapWidth || row < 0 || row >= MapHeight) {
        return;
    }

    if (placement.has_value()
        && !placementFits(*placement, column, row)) {
        setStatus(
            "Pipe does not fit inside the map",
            sf::Color(255, 190, 120)
        );
        return;
    }

    const std::size_t index = static_cast<std::size_t>(row * MapWidth + column);
    bool changed = _cells[index] != symbol
        || _cellPlacements[index] != placement;
    if (!changed && symbol == '.') {
        for (std::size_t placementIndex = 0;
             placementIndex < _cellPlacements.size();
             ++placementIndex) {
            if (!_cellPlacements[placementIndex].has_value()) {
                continue;
            }
            const int anchorColumn = static_cast<int>(
                placementIndex % static_cast<std::size_t>(MapWidth)
            );
            const int anchorRow = static_cast<int>(
                placementIndex / static_cast<std::size_t>(MapWidth)
            );
            const auto footprint = placementFootprint(
                *_cellPlacements[placementIndex],
                anchorColumn,
                anchorRow
            );
            if (std::find(
                    footprint.begin(),
                    footprint.end(),
                    sf::Vector2i{column, row}
                ) != footprint.end()) {
                changed = true;
                break;
            }
        }
    }
    if (!changed) {
        return;
    }

    if (!_strokeUndoCaptured) {
        rememberBeforeEdit();
        _strokeUndoCaptured = true;
    }
    clearOverlappingPlacements(column, row);
    _cells[index] = symbol;
    _cellPlacements[index] = placement;
    _dirty = true;
}

void MapEditorScene::applyPaintRectangle(
    int startColumn,
    int startRow,
    int endColumn,
    int endRow,
    char symbol,
    const std::optional<CellPlacement>& placement
) {
    const int left = std::min(startColumn, endColumn);
    const int top = std::min(startRow, endRow);
    const int right = std::max(startColumn, endColumn);
    const int bottom = std::max(startRow, endRow);

    for (int row = top; row <= bottom; ++row) {
        for (int column = left; column <= right; ++column) {
            applyPaintCell(column, row, symbol, placement);
        }
    }
}

void MapEditorScene::clearMap() {
    const bool empty = std::all_of(
        _cells.begin(),
        _cells.end(),
        [](char symbol) { return symbol == '.'; }
    ) && std::all_of(
        _cellPlacements.begin(),
        _cellPlacements.end(),
        [](const std::optional<CellPlacement>& placement) {
            return !placement.has_value();
        }
    );
    if (empty) {
        setStatus("Map is already empty");
        return;
    }

    rememberBeforeEdit();
    std::fill(_cells.begin(), _cells.end(), '.');
    std::fill(_cellPlacements.begin(), _cellPlacements.end(), std::nullopt);
    _dirty = true;
    setStatus("Map cleared");
}

void MapEditorScene::undoLastEdit() {
    if (_undoHistory.empty()) {
        setStatus("Nothing to undo", sf::Color(255, 205, 120));
        return;
    }

    _redoHistory.push_back({_cells, _cellPlacements, MapWidth, MapHeight});
    MapWidth = _undoHistory.back().width;
    MapHeight = _undoHistory.back().height;
    _cells = std::move(_undoHistory.back().cells);
    _cellPlacements = std::move(_undoHistory.back().placements);
    _undoHistory.pop_back();
    _gridBackdrop.setSize({MapWidth * CellSize, MapHeight * CellSize});
    clampMapView();
    _strokeUndoCaptured = false;
    _dirty = true;
    setStatus("Last edit undone");
}

void MapEditorScene::redoLastEdit() {
    if (_redoHistory.empty()) {
        setStatus("Nothing to redo", sf::Color(255, 205, 120));
        return;
    }

    _undoHistory.push_back({_cells, _cellPlacements, MapWidth, MapHeight});
    MapWidth = _redoHistory.back().width;
    MapHeight = _redoHistory.back().height;
    _cells = std::move(_redoHistory.back().cells);
    _cellPlacements = std::move(_redoHistory.back().placements);
    _redoHistory.pop_back();
    _gridBackdrop.setSize({MapWidth * CellSize, MapHeight * CellSize});
    clampMapView();
    _strokeUndoCaptured = false;
    _dirty = true;
    setStatus("Last edit redone");
}

void MapEditorScene::rememberBeforeEdit() {
    _undoHistory.push_back({_cells, _cellPlacements, MapWidth, MapHeight});
    _redoHistory.clear();
}

bool MapEditorScene::canPlayMap() {
    const bool hasMario = std::find(_cells.begin(), _cells.end(), 'M')
        != _cells.end();
    const bool hasLuigi = std::find(_cells.begin(), _cells.end(), 'L')
        != _cells.end();

    if (!hasMario && !hasLuigi) {
        setStatus(
            "Add at least one player (Mario or Luigi) before playing",
            sf::Color(255, 180, 120)
        );
        return false;
    }

    return true;
}

bool MapEditorScene::loadSavedMap() {
    const std::filesystem::path path(savedMapPath());
    if (!std::filesystem::exists(path)) {
        return false;
    }

    try {
        const LevelData levelData = LevelDataLoader::load(
            path,
            MaximumMapWidth,
            MaximumMapHeight
        );
        if (levelData.layer.empty() || levelData.layer.front().empty()) {
            throw std::runtime_error("Saved map has no cells");
        }
        resizeMap(
            static_cast<int>(levelData.layer.front().size()),
            static_cast<int>(levelData.layer.size()),
            false
        );
        std::fill(_cells.begin(), _cells.end(), '.');
        std::fill(_cellPlacements.begin(), _cellPlacements.end(), std::nullopt);
        const std::size_t rowCount = std::min(
            static_cast<std::size_t>(MapHeight),
            levelData.layer.size()
        );
        for (std::size_t row = 0; row < rowCount; ++row) {
            const std::size_t copyWidth = std::min(
                static_cast<std::size_t>(MapWidth),
                levelData.layer[row].size()
            );
            std::copy_n(
                levelData.layer[row].begin(),
                copyWidth,
                _cells.begin() + row * MapWidth
            );
        }
        for (const LevelData::Placement& savedPlacement : levelData.placements) {
            if (savedPlacement.column < 0
                || savedPlacement.column >= MapWidth
                || savedPlacement.row < 0
                || savedPlacement.row >= MapHeight) {
                continue;
            }

            CellPlacement placement;
            if (savedPlacement.spec.typeKey == "CoinBlock") {
                placement.prefabId = "block_coin";
                placement.coinCapacity = savedPlacement.spec.coinCapacity;
                _cells[static_cast<std::size_t>(
                    savedPlacement.row * MapWidth + savedPlacement.column
                )] = 'B';
            } else if (savedPlacement.spec.typeKey == "LuckyBlock") {
                placement.prefabId = "block_lucky";
                placement.luckyTexture = savedPlacement.spec.luckyTexture;
                placement.luckyCapacity = savedPlacement.spec.luckyCapacity;
                for (const LuckyOptionSpec& option : savedPlacement.spec.luckyOptions) {
                    placement.luckyOptions.push_back({
                        option.itemTypeKey,
                        option.weight
                    });
                }
                _cells[static_cast<std::size_t>(
                    savedPlacement.row * MapWidth + savedPlacement.column
                )] = '?';
            } else if (savedPlacement.spec.typeKey == "Pipe") {
                placement.prefabId = "pipe_basic";
                placement.pipeOrientation = savedPlacement.spec.pipeOrientation;
                placement.pipeEndSide = savedPlacement.spec.pipeEndSide;
                placement.pipeBodyLength = savedPlacement.spec.pipeBodyLength;
                placement.pipeIsWarp = savedPlacement.spec.pipeIsWarp;
                placement.warpID = savedPlacement.spec.warpID;
                placement.warpTarget = savedPlacement.spec.warpTarget;
                placement.pipeContainsPiranha =
                    savedPlacement.spec.contents != nullptr;
                placement.pipeContentsStatic = savedPlacement.spec.contentsStatic;
                _cells[static_cast<std::size_t>(
                    savedPlacement.row * MapWidth + savedPlacement.column
                )] = 'V';
            } else {
                continue;
            }
            _cellPlacements[static_cast<std::size_t>(
                savedPlacement.row * MapWidth + savedPlacement.column
            )] = std::move(placement);
        }
        _undoHistory.clear();
        _redoHistory.clear();
        _strokeUndoCaptured = false;
        applyLoadedTheme(
            levelData.theme,
            levelData.background,
            levelData.music
        );
        _dirty = false;
        setStatus("Loaded custom-map.json", sf::Color(160, 255, 175));
    } catch (const std::exception& error) {
        setStatus(
            std::string("Could not load saved map: ") + error.what(),
            sf::Color(255, 180, 120)
        );
    }
    return true;
}

bool MapEditorScene::saveMap() {
    json document;
    json tileMapping = {
        {".", "empty"}
    };
    for (const PaletteEntry& entry : _paletteEntries) {
        const std::string prefabId = entry.prefabId == "terrain_grassland"
            && _themeKey == "underground"
            ? "terrain_underground"
            : entry.prefabId;
        tileMapping[std::string(1, entry.symbol)] = prefabId;
    }

    std::vector<std::string> layer;
    layer.reserve(MapHeight);
    for (int row = 0; row < MapHeight; ++row) {
        layer.emplace_back(
            _cells.begin() + row * MapWidth,
            _cells.begin() + (row + 1) * MapWidth
        );
    }

    document["tileMapping"] = std::move(tileMapping);
    document["layer"] = std::move(layer);
    document["theme"] = _themeKey;
    document["background"] = _themeKey == "underground" ? "parallax_underground" : "parallax_sky";
    document["music"] = _themeKey == "underground" ? "underground_theme" : "ground_theme";
    json placements = json::array();
    for (std::size_t index = 0; index < _cellPlacements.size(); ++index) {
        if (!_cellPlacements[index].has_value()) {
            continue;
        }

        const CellPlacement& placement = *_cellPlacements[index];
        const int column = static_cast<int>(
            index % static_cast<std::size_t>(MapWidth)
        );
        const int row = static_cast<int>(
            index / static_cast<std::size_t>(MapWidth)
        );
        json spec;
        if (placement.prefabId == "block_coin") {
            spec = {
                {"kind", "block"},
                {"typeKey", "CoinBlock"},
                {"texture", ThemeAssets::brickTextureAlias(_themeKey)},
                {"animationId", "coin_block"},
                {"size", {64, 64}},
                {"addSeamFilter", true},
                {"coinCapacity", placement.coinCapacity}
            };
        } else if (placement.prefabId == "block_lucky") {
            spec = {
                {"kind", "block"},
                {"typeKey", "LuckyBlock"},
                {"texture", ThemeAssets::luckyBlockTextureFor(
                    placement.luckyTexture,
                    _themeKey
                )},
                {"luckyTexture", placement.luckyTexture},
                {"size", {64, 64}},
                {"addSeamFilter", true},
                {"luckyCapacity", placement.luckyCapacity},
                {"luckyOptions", json::array()}
            };
            for (const LuckyOptionData& option : placement.luckyOptions) {
                spec["luckyOptions"].push_back({
                    {"item", option.itemTypeKey},
                    {"weight", option.weight}
                });
            }
        } else if (placement.prefabId == "pipe_basic") {
            spec = {
                {"kind", "pipe"},
                {"typeKey", "Pipe"},
                {"texture", "pipes_spritesheet"},
                {"size", {128, 192}},
                {"pipeOrientation", placement.pipeOrientation},
                {"pipeEndSide", placement.pipeEndSide},
                {"pipeBodyLength", placement.pipeBodyLength},
                {"pipeIsWarp", placement.pipeIsWarp},
                {"warpID", placement.warpID},
                {"warpTarget", placement.warpTarget},
                {"contentsStatic", placement.pipeContentsStatic},
                {"addSeamFilter", true}
            };
            if (placement.pipeContainsPiranha) {
                spec["contents"] = {
                    {"kind", "enemy"},
                    {"typeKey", "PiranhaPlant"},
                    {"texture", "piranha_plant_spritesheet"},
                    {"animationId", "piranha_plant"},
                    {"size", {78, 105}},
                    {"offset", {0, 5}}
                };
            }
        } else {
            continue;
        }
        placements.push_back({
            {"column", column},
            {"row", row},
            {"spec", std::move(spec)}
        });
    }
    document["placements"] = std::move(placements);
    document["editor"] = {
        {"width", MapWidth},
        {"height", MapHeight},
        {"description", "Map created in the in-game map builder"}
    };
    document["prefabs"] = {
        {
            "terrain_grassland",
            {
                {"texture", "at_grassland"},
                {"solid", true},
                {"autotile", "grassland_terrain"},
                {"addSeamFilter", true}
            }
        },
        {
            "terrain_underground",
            {
                {"texture", "at_underground"},
                {"solid", true},
                {"autotile", "underground_terrain"},
                {"addSeamFilter", true}
            }
        },
        {
            "pipe_basic",
            {
                {"kind", "pipe"},
                {"typeKey", "Pipe"},
                {"texture", "pipes_spritesheet"},
                {"size", {128, 192}},
                {"pipeOrientation", "vertical"},
                {"pipeEndSide", "top"},
                {"pipeBodyLength", 2},
                {"pipeIsWarp", false},
                {"addSeamFilter", true}
            }
        }
    };

    try {
        const std::filesystem::path path(savedMapPath());
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        const std::string dumpedJson = document.dump(4) + "\n";

        std::ofstream output(path);
        if (!output) {
            setStatus("Could not open custom-map.json", sf::Color(255, 130, 130));
            return false;
        }
        output << dumpedJson;
        if (!output) {
            setStatus("Could not write custom-map.json", sf::Color(255, 130, 130));
            return false;
        }

        std::vector<std::filesystem::path> mirrorPaths = {
            "build/assets/datas/levels/custom-map.json",
            "../assets/datas/levels/custom-map.json",
            "assets/datas/levels/custom-map.json"
        };
        for (const auto& mp : mirrorPaths) {
            if (mp != path && std::filesystem::exists(mp.parent_path())) {
                std::ofstream mOut(mp);
                if (mOut) {
                    mOut << dumpedJson;
                }
            }
        }
    } catch (const std::exception& error) {
        setStatus(
            std::string("Save failed: ") + error.what(),
            sf::Color(255, 130, 130)
        );
        return false;
    }

    _dirty = false;
    setStatus("Saved custom-map.json", sf::Color(160, 255, 175));
    return true;
}

void MapEditorScene::saveAndPlay() {
    if (!canPlayMap()) {
        return;
    }

    if (!saveMap()) {
        return;
    }

    const bool hasMario = std::find(_cells.begin(), _cells.end(), 'M')
        != _cells.end();
    const bool hasLuigi = std::find(_cells.begin(), _cells.end(), 'L')
        != _cells.end();

    auto& settings = GameSettings::getInstance();
    if (hasMario && hasLuigi) {
        settings.gameMode = GameMode::Coop;
    } else if (hasLuigi) {
        settings.gameMode = GameMode::Solo;
        settings.player1Character = "luigi";
    } else {
        settings.gameMode = GameMode::Solo;
        settings.player1Character = "mario";
    }

    if (auto* manager = getSceneManager()) {
        manager->pushScene(
            std::make_unique<InGameScene>(
                savedMapPath(),
                std::nullopt,
                true
            )
        );
    }
}

void MapEditorScene::setStatus(
    const std::string& status,
    const sf::Color& color
) {
    _statusText.setString(status);
    _statusText.setFillColor(color);
}

const MapEditorScene::PaletteEntry* MapEditorScene::findEntry(char symbol) const {
    const auto it = std::find_if(
        _paletteEntries.begin(),
        _paletteEntries.end(),
        [symbol](const PaletteEntry& entry) {
            return entry.symbol == symbol;
        }
    );
    return it == _paletteEntries.end() ? nullptr : &*it;
}

const char* MapEditorScene::categoryName(Category category) {
    switch (category) {
        case Category::Blocks:
            return "Blocks";
        case Category::Items:
            return "Items";
        case Category::Enemies:
            return "Enemies";
        case Category::Players:
            return "Players";
    }
    return "Palette";
}

#pragma once

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Scene/Scene.h"
#include "Button/ButtonMenu.h"

namespace UI {
class CheckBox;
class Dropdown;
class TextInput;
}

class MapEditorScene : public Scene {
public:
    MapEditorScene();
    ~MapEditorScene() override = default;

    static constexpr const char* savedMapPath() noexcept {
        return "assets/datas/levels/custom-map.json";
    }

    void init() override;
    void onEnter() override;
    void onExit() override;
    void handleInput(const sf::Event& event) override;
    void updateSimulation(const float& fixedDt) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

public:
    enum class Category {
        Blocks,
        Items,
        Enemies,
        Players
    };

private:
    struct PaletteEntry {
        Category category;
        std::string label;
        std::string prefabId;
        char symbol;
        sf::Color previewColor;
    };

    struct ThemeChoice {
        std::string key;
        std::string label;
        std::string background;
        std::string music;
    };

    struct LuckyOptionData {
        std::string itemTypeKey;
        float weight = 1.0f;

        bool operator==(const LuckyOptionData&) const = default;
    };

    struct CellPlacement {
        std::string prefabId;
        std::vector<LuckyOptionData> luckyOptions;
        int coinCapacity = 10;
        std::string luckyTexture = "default";
        int luckyCapacity = 1;
        std::string pipeOrientation = "vertical";
        std::string pipeEndSide = "top";
        int pipeBodyLength = 2;
        bool pipeIsWarp = false;
        int warpID = 1;
        int warpTarget = 2;
        bool pipeContainsPiranha = false;
        bool pipeContentsStatic = false;

        bool operator==(const CellPlacement&) const = default;
    };

    struct MapSnapshot {
        std::vector<char> cells;
        std::vector<std::optional<CellPlacement>> placements;
        int width = 0;
        int height = 0;
    };

    struct PreviewSpec {
        std::string textureKey;
        std::string animationId;
        sf::IntRect textureRect{};
        sf::Vector2f size{64.0f, 64.0f};
        sf::Vector2f visualScale{1.0f, 1.0f};
        sf::Vector2f offset{};
        bool centerVertically = false;
        bool alignToCellBottom = false;
    };

    enum class ConfigMode {
        None,
        CoinBlock,
        LuckyBlock,
        Pipe,
        MapSize
    };

    static constexpr int DefaultMapWidth = 80;
    static constexpr int DefaultMapHeight = 40;
    static constexpr int MinimumMapWidth = 10;
    static constexpr int MaximumMapWidth = 500;
    static constexpr int MinimumMapHeight = 8;
    static constexpr int MaximumMapHeight = 60;
    static constexpr float CellSize = 64.0f;
    static constexpr float LogicalScreenWidth = 1920.0f;
    static constexpr float LogicalScreenHeight = 1080.0f;
    static constexpr float MapViewportWidth = 1584.0f;
    static constexpr float MapViewportHeight = LogicalScreenHeight;
    static constexpr float MinimumZoom = 0.20f;
    static constexpr float MaximumZoom = 3.0f;
    static constexpr float CameraPanMargin = CellSize * 8.0f;
    static constexpr float PaletteLeft = 1595.0f;
    static constexpr float PaletteViewportTop = 210.0f;
    static constexpr float PaletteViewportHeight = 420.0f;
    static constexpr float PaletteButtonHeight = 50.0f;
    static constexpr float PaletteButtonSpacing = 53.0f;
    static constexpr float PaletteScrollX = 1872.0f;
    static constexpr float GeneralMapTop = 650.0f;
    static constexpr float GeneralMapSpacing = 42.0f;
    static constexpr float MapSizeButtonTop = GeneralMapTop + GeneralMapSpacing;
    static constexpr float MapSizeDropdownTop = MapSizeButtonTop + 42.0f;
    static constexpr float MapSizeDropdownHeight = 84.0f;
    static constexpr float MapSizeInputHeight = 36.0f;
    static constexpr float MapSizeInputSpacing = 40.0f;
    static constexpr float ConfigViewportTop = 245.0f;
    static constexpr float ConfigViewportHeight = 520.0f;
    static constexpr float ConfigButtonHeight = 42.0f;
    static constexpr float ConfigButtonSpacing = 47.0f;
    static constexpr float ConfigScrollX = 1308.0f;

    void setupMenus();
    void setupCategoryMenu();
    void setupPaletteMenu();
    void setupGeneralMapMenu();
    void setupActionMenu();
    void setupInstructionsMenu();
    void setupConfigMenu();
    void selectCategory(Category category);
    void selectSymbol(char symbol);
    void openCoinBlockConfig();
    void openLuckyBlockConfig();
    void openPipeConfig();
    void openMapSizeConfig();
    void refreshMapSizeDropdown();
    void tryAutoApplyMapSize();
    void closeMapSizeDropdown();
    void cycleLuckyOption(std::size_t optionIndex);
    void cycleLuckyCapacity();
    void cyclePipeOrientation();
    void cyclePipeEndSide();
    void cyclePipeLength();
    void togglePipeWarp();
    void cyclePipeWarpID();
    void cyclePipeWarpTarget();
    void togglePipePiranha();
    void togglePipeContentsStatic();
    void confirmConfig();
    void cancelConfig();
    void applyMapSize();
    void resizeMap(int width, int height, bool preserveCells);
    void refreshConfigMenu();
    void updateScrollVisuals();
    void scrollPalette(float wheelDelta);
    void scrollConfig(float wheelDelta);
    void cycleTheme();
    void setThemeIndex(std::size_t index, bool markDirty);
    void refreshThemeButton();
    void applyLoadedTheme(
        const std::string& theme,
        const std::string& background,
        const std::string& music
    );
    void handleMouseMoved(const sf::Event::MouseMoved& mouseEvent);
    void handleMousePressed(const sf::Event::MouseButtonPressed& mouseEvent);
    void handleMouseReleased(const sf::Event::MouseButtonReleased& mouseEvent);
    void updateHover(sf::Vector2i screenPosition);
    void beginPaint(sf::Mouse::Button button, sf::Vector2i screenPosition);
    void continuePaint(sf::Vector2i screenPosition);
    void endPaint(sf::Mouse::Button button);
    void panMap(sf::Vector2i screenPosition);
    void zoomMap(float wheelDelta, sf::Vector2i screenPosition);
    void clampMapView();
    sf::Vector2f mapScreenToWorld(sf::Vector2f screenPosition) const;
    std::optional<sf::Vector2i> mapCellAtScreen(sf::Vector2i screenPosition) const;
    bool isMapCanvasPosition(sf::Vector2i screenPosition) const;
    bool isShiftHeld() const;
    void drawMap(sf::RenderTarget& target);
    void drawMapGrid(sf::RenderTarget& target);
    void drawPreviewSprite(
        sf::RenderTarget& target,
        const PreviewSpec& spec,
        sf::Vector2f cellCenter
    ) const;
    void drawInvisibleLuckyBlockMarker(
        sf::RenderTarget& target,
        int column,
        int row
    ) const;
    void drawEntrySprite(
        sf::RenderTarget& target,
        const PaletteEntry& entry,
        sf::Vector2f cellCenter
    ) const;
    void drawPlacement(
        sf::RenderTarget& target,
        const CellPlacement& placement,
        char symbol,
        int column,
        int row
    ) const;
    void drawPipePreview(
        sf::RenderTarget& target,
        const CellPlacement& placement,
        int column,
        int row
    ) const;
    PreviewSpec previewSpecFor(const PaletteEntry& entry) const;
    sf::IntRect firstAnimationFrame(const std::string& animationId) const;
    std::vector<sf::Vector2i> placementFootprint(
        const CellPlacement& placement,
        int column,
        int row
    ) const;
    bool placementFits(
        const CellPlacement& placement,
        int column,
        int row
    ) const;
    void clearOverlappingPlacements(int column, int row);
    void eraseCell(int column, int row);
    void placeCell(int column, int row);
    void applyPaintCell(
        int column,
        int row,
        char symbol,
        const std::optional<CellPlacement>& placement = std::nullopt
    );
    void applyPaintRectangle(
        int startColumn,
        int startRow,
        int endColumn,
        int endRow,
        char symbol,
        const std::optional<CellPlacement>& placement = std::nullopt
    );
    void clearMap();
    void undoLastEdit();
    void redoLastEdit();
    void rememberBeforeEdit();
    bool canPlayMap();
    bool loadSavedMap();
    bool saveMap();
    void saveAndPlay();
    void setStatus(const std::string& status, const sf::Color& color = sf::Color::White);
    const PaletteEntry* findEntry(char symbol) const;
    static const char* categoryName(Category category);

    std::vector<PaletteEntry> _paletteEntries;
    int MapWidth = DefaultMapWidth;
    int MapHeight = DefaultMapHeight;
    std::vector<char> _cells;
    std::vector<std::optional<CellPlacement>> _cellPlacements;
    std::vector<MapSnapshot> _undoHistory;
    std::vector<MapSnapshot> _redoHistory;
    sf::View _mapView;
    std::vector<ThemeChoice> _themeOptions;
    std::size_t _themeIndex = 0;
    std::string _themeKey = "sky";
    float _paletteScrollOffset = 0.0f;
    float _configScrollOffset = 0.0f;
    std::optional<CellPlacement> _selectedPlacement;
    char _selectionBeforeConfig = '#';
    std::optional<CellPlacement> _placementBeforeConfig;
    Category _activeCategory = Category::Blocks;
    char _selectedSymbol = '#';
    float _zoom = 1.0f;
    int _hoverColumn = -1;
    int _hoverRow = -1;
    int _dragStartColumn = -1;
    int _dragStartRow = -1;
    sf::Vector2i _lastMousePosition{};
    sf::Mouse::Button _paintButton = sf::Mouse::Button::Left;
    bool _leftMouseHeld = false;
    bool _rightMouseHeld = false;
    bool _middleMouseHeld = false;
    bool _paintActive = false;
    bool _rectangleDrag = false;
    bool _strokeUndoCaptured = false;
    bool _dirty = false;
    bool _showInstructions = false;
    bool _mapSizeExpanded = false;
    ConfigMode _configMode = ConfigMode::None;
    CellPlacement _draftPlacement;
    int _draftMapWidth = DefaultMapWidth;
    int _draftMapHeight = DefaultMapHeight;

    UI::ButtonMenu _categoryMenu;
    UI::ButtonMenu _paletteMenu;
    UI::ButtonMenu _generalMapMenu;
    UI::ButtonMenu _actionMenu;
    UI::ButtonMenu _mapSizeMenu;
    UI::ButtonMenu _instructionsMenu;
    UI::ButtonMenu _configMenu;
    std::shared_ptr<UI::Dropdown> _themeDropdown;
    std::shared_ptr<UI::Dropdown> _coinCapacityDropdown;
    std::shared_ptr<UI::Dropdown> _luckyTextureDropdown;
    std::shared_ptr<UI::Dropdown> _luckyCapacityDropdown;
    std::vector<std::shared_ptr<UI::CheckBox>> _luckyOptionChecks;
    std::shared_ptr<UI::Dropdown> _pipeOrientationDropdown;
    std::shared_ptr<UI::Dropdown> _pipeEndSideDropdown;
    std::shared_ptr<UI::TextInput> _pipeLengthInput;
    std::shared_ptr<UI::CheckBox> _pipeWarpCheck;
    std::shared_ptr<UI::TextInput> _pipeWarpIDInput;
    std::shared_ptr<UI::TextInput> _pipeWarpTargetInput;
    std::shared_ptr<UI::CheckBox> _pipePiranhaCheck;
    std::shared_ptr<UI::CheckBox> _pipeContentsStaticCheck;
    std::shared_ptr<UI::TextInput> _mapWidthInput;
    std::shared_ptr<UI::TextInput> _mapHeightInput;

    sf::RectangleShape _screenBackdrop;
    sf::RectangleShape _gridBackdrop;
    sf::RectangleShape _paletteBackdrop;
    sf::Text _titleText;
    sf::Text _paletteTitleText;
    sf::Text _selectedText;
    sf::Text _statusText;
    sf::RectangleShape _instructionsBackdrop;
    sf::RectangleShape _instructionsPanel;
    sf::Text _instructionsTitle;
    sf::Text _instructionsBody;
    sf::RectangleShape _configBackdrop;
    sf::RectangleShape _configPanel;
    sf::Text _configTitle;
    sf::Text _configBody;
    sf::RectangleShape _paletteScrollTrack;
    sf::RectangleShape _paletteScrollThumb;
    sf::RectangleShape _mapSizeDropdownBackdrop;
    sf::RectangleShape _configScrollTrack;
    sf::RectangleShape _configScrollThumb;
};

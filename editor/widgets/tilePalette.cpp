#include "tilePalette.h"
#include <QButtonGroup>
#include <QScrollArea>

TilePalette::TilePalette(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(250);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollContent = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(8);

    auto *terrainGroup = new QGroupBox("Terreno", scrollContent);
    auto *terrainLayout = new QVBoxLayout(terrainGroup);
    _rbGrass = new QRadioButton("🌿 Pasto", terrainGroup);
    _rbSand = new QRadioButton("🏜 Arena", terrainGroup);
    _rbWater = new QRadioButton("💧 Agua", terrainGroup);
    _rbGrass->setChecked(true);

    auto *terrainBtns = new QButtonGroup(this);
    for (auto *rb : {_rbGrass, _rbSand, _rbWater})
    {
        terrainBtns->addButton(rb);
        terrainLayout->addWidget(rb);
    }
    mainLayout->addWidget(terrainGroup);

    auto *structGroup = new QGroupBox("Estructuras", scrollContent);
    auto *structLayout = new QVBoxLayout(structGroup);
    _rbForest = new QRadioButton("🌲 Árbol", structGroup);
    _rbCactus = new QRadioButton("🌵 Cactus", structGroup);
    _rbStone = new QRadioButton("🪨 Piedra", structGroup);

    auto *structBtns = new QButtonGroup(this);
    for (auto *rb : {_rbForest, _rbCactus, _rbStone})
    {
        structBtns->addButton(rb);
        structLayout->addWidget(rb);
    }
    mainLayout->addWidget(structGroup);

    auto *cityGroup = new QGroupBox("Ciudad", scrollContent);
    auto *cityLayout = new QVBoxLayout(cityGroup);
    _rbCityFloor = new QRadioButton("🏙 Piso ciudad", cityGroup);
    _rbHouse = new QRadioButton("🏠 Casa", cityGroup);
    _rbChurch = new QRadioButton("⛪ Iglesia", cityGroup);
    _rbMill = new QRadioButton("⚙ Molino", cityGroup);

    auto *cityBtns = new QButtonGroup(this);
    for (auto *rb : {_rbCityFloor, _rbHouse, _rbChurch, _rbMill})
    {
        cityBtns->addButton(rb);
        cityLayout->addWidget(rb);
    }
    mainLayout->addWidget(cityGroup);

    auto *subGroup = new QGroupBox("Subterráneo", scrollContent);
    auto *subLayout = new QVBoxLayout(subGroup);
    _rbCavernFloor = new QRadioButton("🟫 Piso Caverna", subGroup);
    _rbCavernWallH = new QRadioButton("➖ Pared Cav H", subGroup);
    _rbCavernWallV = new QRadioButton("🦺 Pared Cav V", subGroup);
    _rbDungeonFloor = new QRadioButton("⬛ Piso Mazmo", subGroup);
    _rbDungeonWallH = new QRadioButton("➖ Pared Maz H", subGroup);
    _rbDungeonWallV = new QRadioButton("🦺 Pared Maz V", subGroup);

    auto *subBtns = new QButtonGroup(this);
    for (auto *rb : {_rbCavernFloor, _rbCavernWallH, _rbCavernWallV, _rbDungeonFloor,
                     _rbDungeonWallH, _rbDungeonWallV})
    {
        subBtns->addButton(rb);
        subLayout->addWidget(rb);
    }
    mainLayout->addWidget(subGroup);

    auto *specGroup = new QGroupBox("Especiales", scrollContent);
    auto *specLayout = new QVBoxLayout(specGroup);
    _rbDungeonEntrance = new QRadioButton("⬛ Entrada Mazmorra", specGroup);
    _rbCavernEntrance = new QRadioButton("🕳 Entrada Caverna", specGroup);
    _rbExit = new QRadioButton("🔼 Salida Instancia", specGroup);

    auto *specBtns = new QButtonGroup(this);
    for (auto *rb : {_rbDungeonEntrance, _rbCavernEntrance, _rbExit})
    {
        specBtns->addButton(rb);
        specLayout->addWidget(rb);
    }
    mainLayout->addWidget(specGroup);

    auto *decoGroup = new QGroupBox("Decoraciones", scrollContent);
    auto *decoLayout = new QVBoxLayout(decoGroup);
    _rbWell = new QRadioButton("🪣 Pozo", decoGroup);
    _rbBanner = new QRadioButton("💀 Estandarte Esqueleto", decoGroup);
    _rbWitchBanner = new QRadioButton("🔮 Estandarte Bruja", decoGroup);
    _rbBoxes = new QRadioButton("📦 Cajas", decoGroup);
    _rbShop = new QRadioButton("🏪 Tienda", decoGroup);

    auto *decoBtns = new QButtonGroup(this);
    for (auto *rb : {_rbWell, _rbBanner, _rbWitchBanner, _rbBoxes, _rbShop})
    {
        decoBtns->addButton(rb);
        decoLayout->addWidget(rb);
    }
    mainLayout->addWidget(decoGroup);

    auto *zoneGroup = new QGroupBox("Zona", scrollContent);
    auto *zoneLayout = new QVBoxLayout(zoneGroup);
    _rbZoneSafe = new QRadioButton("🛡 Segura", zoneGroup);
    _rbZoneCombat = new QRadioButton("⚔ Combate", zoneGroup);
    _rbZoneCombat->setChecked(true);

    auto *zoneBtns = new QButtonGroup(this);
    for (auto *rb : {_rbZoneSafe, _rbZoneCombat})
    {
        zoneBtns->addButton(rb);
        zoneLayout->addWidget(rb);
    }
    mainLayout->addWidget(zoneGroup);

    auto *propGroup = new QGroupBox("Propiedades", scrollContent);
    auto *propLayout = new QVBoxLayout(propGroup);
    _cbWalkable = new QCheckBox("Caminable", propGroup);
    _cbWalkable->setChecked(true);
    propLayout->addWidget(_cbWalkable);
    mainLayout->addWidget(propGroup);

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);

    auto *allTileBtns = new QButtonGroup(this);
    allTileBtns->setExclusive(true);
    for (auto *g : {terrainBtns, structBtns, specBtns, cityBtns, subBtns, decoBtns})
    {
        for (auto *b : g->buttons())
        {
            allTileBtns->addButton(b);
        }
    }

    connect(allTileBtns, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),
            this, &TilePalette::selectionChanged);
    connect(zoneBtns, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),
            this, &TilePalette::selectionChanged);
    connect(_cbWalkable, &QCheckBox::stateChanged, this, &TilePalette::selectionChanged);
}

void TilePalette::setMapType(MapType type)
{
    bool isInstanced = (type == MapType::DUNGEON || type == MapType::CAVE);
    _rbExit->setVisible(isInstanced);

    if (!isInstanced && _rbExit->isChecked())
    {
        _rbGrass->setChecked(true);
        emit selectionChanged();
    }
}

TileType TilePalette::selectedTileType() const
{
    if (_rbSand->isChecked())
        return TileType::SAND;
    if (_rbWater->isChecked())
        return TileType::WATER;
    if (_rbForest->isChecked())
        return TileType::FOREST;
    if (_rbDungeonEntrance->isChecked())
        return TileType::DUNGEON_ENTRANCE;
    if (_rbCavernEntrance->isChecked())
        return TileType::CAVERN_ENTRANCE;
    if (_rbExit->isChecked())
        return TileType::EXIT;
    if (_rbCactus->isChecked())
        return TileType::CACTUS;
    if (_rbStone->isChecked())
        return TileType::STONE;
    if (_rbCityFloor->isChecked())
        return TileType::CITY_FLOOR;
    if (_rbHouse->isChecked())
        return TileType::HOUSE;
    if (_rbChurch->isChecked())
        return TileType::CHURCH;
    if (_rbMill->isChecked())
        return TileType::MILL;
    if (_rbCavernFloor->isChecked())
        return TileType::CAVERN_FLOOR;
    if (_rbCavernWallH->isChecked())
        return TileType::CAVERN_WALL_H;
    if (_rbCavernWallV->isChecked())
        return TileType::CAVERN_WALL_V;
    if (_rbDungeonFloor->isChecked())
        return TileType::DUNGEON_FLOOR;
    if (_rbDungeonWallH->isChecked())
        return TileType::DUNGEON_WALL_H;
    if (_rbDungeonWallV->isChecked())
        return TileType::DUNGEON_WALL_V;
    if (_rbWell->isChecked())
        return TileType::WELL;
    if (_rbBanner->isChecked())
        return TileType::BANNER;
    if (_rbWitchBanner->isChecked())
        return TileType::WITCH_BANNER;
    if (_rbBoxes->isChecked())
        return TileType::BOXES;
    if (_rbShop->isChecked())
        return TileType::SHOP;
    return TileType::GRASS;
}

ZoneType TilePalette::selectedZoneType() const
{
    if (_rbZoneCombat->isChecked())
        return ZoneType::COMBAT;
    return ZoneType::SAFE;
}

bool TilePalette::selectedWalkable() const { return _cbWalkable->isChecked(); }

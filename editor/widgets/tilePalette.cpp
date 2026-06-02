#include "tilePalette.h"
#include <QButtonGroup>

#include "tilePalette.h"
#include <QButtonGroup>

TilePalette::TilePalette(QWidget *parent) : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    setFixedWidth(160);

    auto *tileGroup = new QGroupBox("Tipo de tile", this);
    auto *tileLayout = new QVBoxLayout(tileGroup);

    _rbGrass = new QRadioButton("Pasto", tileGroup);
    _rbWater = new QRadioButton("Agua", tileGroup);
    _rbWall = new QRadioButton("Pared", tileGroup);
    _rbFloor = new QRadioButton("Piso", tileGroup);
    _rbDoor = new QRadioButton("Puerta", tileGroup);
    _rbCavern = new QRadioButton("Entrada Caverna", tileGroup);
    _rbDungeon = new QRadioButton("Entrada Mazmorra", tileGroup);
    _rbExit = new QRadioButton("Salida instancia", tileGroup);
    _rbGrass->setChecked(true);

    auto *tileButtons = new QButtonGroup(this);
    tileButtons->addButton(_rbGrass);
    tileButtons->addButton(_rbWater);
    tileButtons->addButton(_rbWall);
    tileButtons->addButton(_rbFloor);
    tileButtons->addButton(_rbDoor);
    tileButtons->addButton(_rbCavern);
    tileButtons->addButton(_rbDungeon);
    tileButtons->addButton(_rbExit);

    tileLayout->addWidget(_rbGrass);
    tileLayout->addWidget(_rbWater);
    tileLayout->addWidget(_rbWall);
    tileLayout->addWidget(_rbFloor);
    tileLayout->addWidget(_rbDoor);
    tileLayout->addWidget(_rbCavern);
    tileLayout->addWidget(_rbDungeon);
    tileLayout->addWidget(_rbExit);

    mainLayout->addWidget(tileGroup);

    auto *zoneGroup = new QGroupBox("Zona", this);
    auto *zoneLayout = new QVBoxLayout(zoneGroup);

    _rbSafe = new QRadioButton("Segura (ciudad)", zoneGroup);
    _rbCombat = new QRadioButton("Combate", zoneGroup);
    _rbCavernZone = new QRadioButton("Caverna", zoneGroup);
    _rbDungeonZone = new QRadioButton("Mazmorra", zoneGroup);
    _rbSafe->setChecked(true);

    auto *zoneButtons = new QButtonGroup(this);
    zoneButtons->addButton(_rbSafe);
    zoneButtons->addButton(_rbCombat);
    zoneButtons->addButton(_rbCavernZone);
    zoneButtons->addButton(_rbDungeonZone);

    zoneLayout->addWidget(_rbSafe);
    zoneLayout->addWidget(_rbCombat);
    zoneLayout->addWidget(_rbCavernZone);
    zoneLayout->addWidget(_rbDungeonZone);

    mainLayout->addWidget(zoneGroup);

    auto *walkGroup = new QGroupBox("Propiedades", this);
    auto *walkLayout = new QVBoxLayout(walkGroup);

    _cbWalkable = new QCheckBox("Caminable", walkGroup);
    _cbWalkable->setChecked(true);
    walkLayout->addWidget(_cbWalkable);

    mainLayout->addWidget(walkGroup);

    connect(tileButtons,
            static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
                &QButtonGroup::buttonClicked),
            this, &TilePalette::selectionChanged);
    connect(zoneButtons,
            static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
                &QButtonGroup::buttonClicked),
            this, &TilePalette::selectionChanged);
    connect(_cbWalkable, &QCheckBox::stateChanged,
            this, &TilePalette::selectionChanged);
}

TileType TilePalette::selectedTileType() const
{
    if (_rbWater->isChecked())
        return TileType::WATER;
    if (_rbWall->isChecked())
        return TileType::WALL;
    if (_rbFloor->isChecked())
        return TileType::FLOOR;
    if (_rbDoor->isChecked())
        return TileType::DOOR;
    if (_rbCavern->isChecked())
        return TileType::CAVERN_ENTRANCE;
    if (_rbDungeon->isChecked())
        return TileType::DUNGEON_ENTRANCE;
    if (_rbExit->isChecked())
        return TileType::EXIT;
    return TileType::GRASS;
}

ZoneType TilePalette::selectedZoneType() const
{
    if (_rbCombat->isChecked())
        return ZoneType::COMBAT;
    if (_rbCavernZone->isChecked())
        return ZoneType::CAVERN;
    if (_rbDungeonZone->isChecked())
        return ZoneType::DUNGEON;
    return ZoneType::SAFE;
}

bool TilePalette::selectedWalkable() const
{
    return _cbWalkable->isChecked();
}

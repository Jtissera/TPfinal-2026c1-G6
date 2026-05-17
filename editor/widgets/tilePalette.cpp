#include "tilePalette.h"
#include <QButtonGroup>

TilePalette::TilePalette(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    setFixedWidth(160);

    auto* tileGroup = new QGroupBox("Tipo de tile", this);
    auto* tileLayout = new QVBoxLayout(tileGroup);

    _rbGrass   = new QRadioButton("Pasto",                    tileGroup);
    _rbWater   = new QRadioButton("Agua",                     tileGroup);
    _rbWall    = new QRadioButton("Pared",                    tileGroup);
    _rbFloor   = new QRadioButton("Piso",                     tileGroup);
    _rbDoor    = new QRadioButton("Puerta",                   tileGroup);
    _rbDungeon = new QRadioButton("Entrada a caverna/mazmorra", tileGroup);
    _rbGrass->setChecked(true);

    auto* tileButtons = new QButtonGroup(this);
    tileButtons->addButton(_rbGrass);
    tileButtons->addButton(_rbWater);
    tileButtons->addButton(_rbWall);
    tileButtons->addButton(_rbFloor);
    tileButtons->addButton(_rbDoor);
    tileButtons->addButton(_rbDungeon);

    tileLayout->addWidget(_rbGrass);
    tileLayout->addWidget(_rbWater);
    tileLayout->addWidget(_rbWall);
    tileLayout->addWidget(_rbFloor);
    tileLayout->addWidget(_rbDoor);
    tileLayout->addWidget(_rbDungeon);
    mainLayout->addWidget(tileGroup);

    auto* zoneGroup = new QGroupBox("Zona", this);
    auto* zoneLayout = new QVBoxLayout(zoneGroup);

    _rbSafe   = new QRadioButton("Segura (ciudad)",   zoneGroup);
    _rbCombat = new QRadioButton("Combate (caverna)", zoneGroup);
    _rbSafe->setChecked(true);

    auto* zoneButtons = new QButtonGroup(this);
    zoneButtons->addButton(_rbSafe);
    zoneButtons->addButton(_rbCombat);

    zoneLayout->addWidget(_rbSafe);
    zoneLayout->addWidget(_rbCombat);
    mainLayout->addWidget(zoneGroup);

    auto* walkGroup = new QGroupBox("Propiedades", this);
    auto* walkLayout = new QVBoxLayout(walkGroup);

    _cbWalkable = new QCheckBox("Caminable", walkGroup);
    _cbWalkable->setChecked(true);
    walkLayout->addWidget(_cbWalkable);
    mainLayout->addWidget(walkGroup);

    connect(tileButtons,
            static_cast<void(QButtonGroup::*)(QAbstractButton*)>(
                &QButtonGroup::buttonClicked),
            this, &TilePalette::selectionChanged);
    connect(zoneButtons,
            static_cast<void(QButtonGroup::*)(QAbstractButton*)>(
                &QButtonGroup::buttonClicked),
            this, &TilePalette::selectionChanged);
    connect(_cbWalkable, &QCheckBox::stateChanged,
            this, &TilePalette::selectionChanged);
}

TileType TilePalette::selectedTileType() const {
    if (_rbWater->isChecked())   return TileType::WATER;
    if (_rbWall->isChecked())    return TileType::WALL;
    if (_rbFloor->isChecked())   return TileType::FLOOR;
    if (_rbDoor->isChecked())    return TileType::DOOR;
    if (_rbDungeon->isChecked()) return TileType::DUNGEON_ENTRANCE;
    return TileType::GRASS;
}

ZoneType TilePalette::selectedZoneType() const {
    return _rbCombat->isChecked() ? ZoneType::COMBAT : ZoneType::SAFE;
}

bool TilePalette::selectedWalkable() const {
    return _cbWalkable->isChecked();
}

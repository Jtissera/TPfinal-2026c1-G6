#include "tilePalette.h"
#include <QButtonGroup>

TilePalette::TilePalette(QWidget *parent) : QWidget(parent) {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setAlignment(Qt::AlignTop);
  setFixedWidth(160);

  // ── Terreno ───────────────────────────────────────────
  auto *terrainGroup = new QGroupBox("Terreno", this);
  auto *terrainLayout = new QVBoxLayout(terrainGroup);

  _rbGrass = new QRadioButton("🌿 Pasto", terrainGroup);
  _rbSand = new QRadioButton("🏜 Arena", terrainGroup);
  _rbWater = new QRadioButton("💧 Agua", terrainGroup);
  _rbFloor = new QRadioButton("🟫 Piso", terrainGroup);
  _rbGrass->setChecked(true);

  auto *terrainBtns = new QButtonGroup(this);
  for (auto *rb : {_rbGrass, _rbSand, _rbWater, _rbFloor}) {
    terrainBtns->addButton(rb);
    terrainLayout->addWidget(rb);
  }
  mainLayout->addWidget(terrainGroup);

  // ── Estructuras ───────────────────────────────────────
  auto *structGroup = new QGroupBox("Estructuras", this);
  auto *structLayout = new QVBoxLayout(structGroup);

  _rbWall = new QRadioButton("🧱 Pared", structGroup);
  _rbDoor = new QRadioButton("🚪 Puerta", structGroup);
  _rbForest = new QRadioButton("🌲 Bosque", structGroup);
  _rbCactus = new QRadioButton("🌵 Cactus", structGroup);
  _rbStone = new QRadioButton("🪨 Piedra", structGroup);

  auto *structBtns = new QButtonGroup(this);
  for (auto *rb : {_rbWall, _rbDoor, _rbForest, _rbCactus, _rbStone}) {
    structBtns->addButton(rb);
    structLayout->addWidget(rb);
  }
  mainLayout->addWidget(structGroup);

  // ── Especiales ────────────────────────────────────────
  auto *specGroup = new QGroupBox("Especiales", this);
  auto *specLayout = new QVBoxLayout(specGroup);

  _rbDungeonEntrance = new QRadioButton("⬛ Entrada Mazmorra", specGroup);
  _rbCavernEntrance = new QRadioButton("🕳 Entrada Caverna", specGroup);
  _rbExit = new QRadioButton("🔼 Salida instancia", specGroup);

  auto *specBtns = new QButtonGroup(this);
  for (auto *rb : {_rbDungeonEntrance, _rbCavernEntrance, _rbExit}) {
    specBtns->addButton(rb);
    specLayout->addWidget(rb);
  }
  mainLayout->addWidget(specGroup);

  // Un único ButtonGroup para todos los tiles (exclusión global)
  auto *allTileBtns = new QButtonGroup(this);
  allTileBtns->setExclusive(true);
  for (auto *g : {terrainBtns, structBtns, specBtns})
    for (auto *b : g->buttons())
      allTileBtns->addButton(b);

  // ── Zona ──────────────────────────────────────────────
  auto *zoneGroup = new QGroupBox("Zona", this);
  auto *zoneLayout = new QVBoxLayout(zoneGroup);

  _rbZoneSafe = new QRadioButton("🛡 Segura", zoneGroup);
  _rbZoneCity = new QRadioButton("🏘 Ciudad", zoneGroup);
  _rbZoneCombat = new QRadioButton("⚔ Combate", zoneGroup);
  _rbZoneDesert = new QRadioButton("☀ Desierto", zoneGroup);
  _rbZoneForest = new QRadioButton("🌳 Bosque", zoneGroup);
  _rbZoneCavern = new QRadioButton("🌑 Caverna", zoneGroup);
  _rbZoneDungeon = new QRadioButton("💀 Mazmorra", zoneGroup);
  _rbZoneCombat->setChecked(true);

  auto *zoneBtns = new QButtonGroup(this);
  for (auto *rb : {_rbZoneSafe, _rbZoneCity, _rbZoneCombat, _rbZoneDesert,
                   _rbZoneForest, _rbZoneCavern, _rbZoneDungeon}) {
    zoneBtns->addButton(rb);
    zoneLayout->addWidget(rb);
  }
  mainLayout->addWidget(zoneGroup);

  // ── Propiedades ───────────────────────────────────────
  auto *propGroup = new QGroupBox("Propiedades", this);
  auto *propLayout = new QVBoxLayout(propGroup);

  _cbWalkable = new QCheckBox("Caminable", propGroup);
  _cbWalkable->setChecked(true);
  propLayout->addWidget(_cbWalkable);
  mainLayout->addWidget(propGroup);

  // ── Señales ───────────────────────────────────────────
  connect(allTileBtns,
          static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
              &QButtonGroup::buttonClicked),
          this, &TilePalette::selectionChanged);
  connect(zoneBtns,
          static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
              &QButtonGroup::buttonClicked),
          this, &TilePalette::selectionChanged);
  connect(_cbWalkable, &QCheckBox::stateChanged, this,
          &TilePalette::selectionChanged);
}

TileType TilePalette::selectedTileType() const {
  if (_rbSand->isChecked())
    return TileType::SAND;
  if (_rbWater->isChecked())
    return TileType::WATER;
  if (_rbFloor->isChecked())
    return TileType::FLOOR;
  if (_rbWall->isChecked())
    return TileType::WALL;
  if (_rbDoor->isChecked())
    return TileType::DOOR;
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
  return TileType::GRASS;
}

ZoneType TilePalette::selectedZoneType() const {
  if (_rbZoneCity->isChecked())
    return ZoneType::CITY;
  if (_rbZoneCombat->isChecked())
    return ZoneType::COMBAT;
  if (_rbZoneDesert->isChecked())
    return ZoneType::DESERT;
  if (_rbZoneForest->isChecked())
    return ZoneType::FOREST;
  if (_rbZoneCavern->isChecked())
    return ZoneType::CAVERN;
  if (_rbZoneDungeon->isChecked())
    return ZoneType::DUNGEON;
  return ZoneType::SAFE;
}

bool TilePalette::selectedWalkable() const { return _cbWalkable->isChecked(); }

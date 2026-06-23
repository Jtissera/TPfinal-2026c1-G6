#pragma once

#include "../map/tile.h"
#include "../map/mapData.h"
#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class TilePalette : public QWidget
{
    Q_OBJECT

public:
    explicit TilePalette(QWidget *parent = nullptr);

    TileType selectedTileType() const;
    ZoneType selectedZoneType() const;
    bool selectedWalkable() const;
    void setMapType(MapType type);

signals:
    void selectionChanged();

private:
    QRadioButton *_rbGrass;
    QRadioButton *_rbSand;
    QRadioButton *_rbWater;

    QRadioButton *_rbForest;
    QRadioButton *_rbCactus;
    QRadioButton *_rbStone;

    QRadioButton *_rbCityFloor;
    QRadioButton *_rbHouse;
    QRadioButton *_rbChurch;
    QRadioButton *_rbMill;

    QRadioButton *_rbDungeonEntrance;
    QRadioButton *_rbCavernEntrance;
    QRadioButton *_rbExit;

    QRadioButton *_rbZoneSafe;
    QRadioButton *_rbZoneCombat;

    QCheckBox *_cbWalkable;

    QRadioButton *_rbCavernFloor;
    QRadioButton *_rbCavernWallH;
    QRadioButton *_rbCavernWallV;
    QRadioButton *_rbDungeonFloor;
    QRadioButton *_rbDungeonWallH;
    QRadioButton *_rbDungeonWallV;

    QRadioButton *_rbWell = nullptr;
    QRadioButton *_rbBanner = nullptr;
    QRadioButton *_rbWitchBanner = nullptr;
    QRadioButton *_rbBoxes = nullptr;
    QRadioButton *_rbShop = nullptr;
};
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QButtonGroup>
#include <QAbstractButton>
#include "../map/tile.h"

class TilePalette : public QWidget
{
    Q_OBJECT

public:
    explicit TilePalette(QWidget *parent = nullptr);

    TileType selectedTileType() const;
    ZoneType selectedZoneType() const;
    bool selectedWalkable() const;

signals:
    void selectionChanged();

private:
    // --- Terreno ---
    QRadioButton *_rbGrass;
    QRadioButton *_rbSand;
    QRadioButton *_rbWater;
    QRadioButton *_rbFloor;

    // --- Estructuras ---
    QRadioButton *_rbWall;
    QRadioButton *_rbDoor;
    QRadioButton *_rbForest;

    // --- Especiales ---
    QRadioButton *_rbDungeonEntrance;
    QRadioButton *_rbCavernEntrance;
    QRadioButton *_rbExit;

    // --- Zona ---
    QRadioButton *_rbZoneSafe;
    QRadioButton *_rbZoneCity;
    QRadioButton *_rbZoneCombat;
    QRadioButton *_rbZoneDesert;
    QRadioButton *_rbZoneForest;
    QRadioButton *_rbZoneCavern;
    QRadioButton *_rbZoneDungeon;

    // --- Propiedades ---
    QCheckBox *_cbWalkable;
};
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QButtonGroup>
#include <QAbstractButton>
#include "../map/tile.h"

class TilePalette : public QWidget {
    Q_OBJECT

public:
    explicit TilePalette(QWidget* parent = nullptr);

    TileType selectedTileType() const;
    ZoneType selectedZoneType() const;
    bool     selectedWalkable() const;

signals:
    void selectionChanged();

private:
    QRadioButton* _rbGrass;
    QRadioButton* _rbWater;
    QRadioButton* _rbWall;
    QRadioButton* _rbFloor;
    QRadioButton* _rbDoor;
    QRadioButton* _rbSafe;
    QRadioButton* _rbCombat;
    QCheckBox* _cbWalkable;
};

#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include "../map/mapData.h"
#include "../map/tile.h"

// Canvas interactivo donde se pinta el mapa tile por tile.
class MapCanvas : public QWidget {
    Q_OBJECT

public:
    explicit MapCanvas(QWidget* parent = nullptr);

    void setMap(MapData* map);

    // Tile que se pinta al hacer click
    void setActiveTileType(TileType type)   { _activeTileType = type; }
    void setActiveZoneType(ZoneType zone)   { _activeZoneType = zone; }
    void setActiveWalkable(bool walkable)   { _activeWalkable = walkable; }

    static constexpr int TILE_SIZE = 32;  // pixeles por tile

signals:
    void tileChanged(uint16_t x, uint16_t y);
    void mapClicked(uint16_t x, uint16_t y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    MapData* _map = nullptr;

    TileType _activeTileType = TileType::GRASS;
    ZoneType _activeZoneType = ZoneType::SAFE;
    bool     _activeWalkable = true;

    bool     _painting = false;

    QColor tileColor(const Tile& tile) const;
    void   paintTileAt(uint16_t x, uint16_t y);
    bool   screenToTile(const QPoint& pos, uint16_t& tx, uint16_t& ty) const;
};

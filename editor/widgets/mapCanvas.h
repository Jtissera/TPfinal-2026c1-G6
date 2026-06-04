#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include "../map/mapData.h"
#include "../map/tile.h"
#include "common/npcType.h"

// Modos de edicion del canvas
enum class EditMode
{
    TILES, // Pintar tipos de tile y zonas
    NPCS,  // Poner/sacar npcs
};

class MapCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit MapCanvas(QWidget *parent = nullptr);

    void setMap(MapData *map);

    void setEditMode(EditMode mode)
    {
        _editMode = mode;
        update();
    }

    void setActiveTileType(TileType type) { _activeTileType = type; }
    void setActiveZoneType(ZoneType zone) { _activeZoneType = zone; }
    void setActiveWalkable(bool walkable) { _activeWalkable = walkable; }

    void setActiveNpc(NpcType npc) { _activeNpc = npc; }

    static constexpr int TILE_SIZE = 32;

signals:
    void tileChanged(uint16_t x, uint16_t y);
    void mapClicked(uint16_t x, uint16_t y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    MapData *_map = nullptr;

    EditMode _editMode = EditMode::TILES;

    TileType _activeTileType = TileType::GRASS;
    ZoneType _activeZoneType = ZoneType::SAFE;
    bool _activeWalkable = true;
    NpcType _activeNpc = NpcType::NONE;

    bool _painting = false;

    QColor tileColor(const Tile &tile) const;
    void applyToTile(uint16_t x, uint16_t y);
    bool screenToTile(const QPoint &pos, uint16_t &tx, uint16_t &ty) const;
    void drawNpcIndicator(QPainter &painter, const QRect &r, NpcType npc) const;

    float _zoomFactor = 1.0f;
    static constexpr float ZOOM_MIN = 0.25f;
    static constexpr float ZOOM_MAX = 4.0f;
    static constexpr float ZOOM_STEP = 0.15f;

    int tileSize() const
    {
        return static_cast<int>(TILE_SIZE * _zoomFactor);
    }
};

#include "mapCanvas.h"
#include <QPainter>
#include <QMouseEvent>

MapCanvas::MapCanvas(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(200, 200);
}

void MapCanvas::setMap(MapData* map) {
    _map = map;
    if (_map) {
        setMinimumSize(
            _map->width()  * TILE_SIZE,
            _map->height() * TILE_SIZE
        );
    }
    update();
}

QColor MapCanvas::tileColor(const Tile& tile) const {
    QColor base;
    switch (tile.type) {
        case TileType::GRASS:  base = QColor(100, 160,  70); break;
        case TileType::WATER:  base = QColor( 60, 120, 200); break;
        case TileType::WALL:   base = QColor( 80,  80,  80); break;
        case TileType::FLOOR:  base = QColor(180, 150, 100); break;
        case TileType::DOOR:   base = QColor(160, 100,  50); break;
        default:               base = QColor(200, 200, 200); break;
    }

    // Zona de combate: tinte rojizo
    if (tile.zone == ZoneType::COMBAT)
        base = base.darker(130);

    // No caminable: más oscuro
    if (!tile.walkable)
        base = base.darker(160);

    return base;
}

void MapCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (!_map) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Sin mapa cargado");
        return;
    }

    for (uint16_t y = 0; y < _map->height(); ++y) {
        for (uint16_t x = 0; x < _map->width(); ++x) {
            const Tile& tile = _map->at(x, y);
            QRect r(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);

            painter.fillRect(r, tileColor(tile));
            painter.setPen(QColor(0, 0, 0, 60));
            painter.drawRect(r);

            if (tile.npcId != 0) {
                painter.setPen(Qt::yellow);
                painter.drawText(r, Qt::AlignCenter, "N");
            }
        }
    }
}

bool MapCanvas::screenToTile(const QPoint& pos, uint16_t& tx, uint16_t& ty) const {
    if (!_map) return false;
    int x = pos.x() / TILE_SIZE;
    int y = pos.y() / TILE_SIZE;
    if (x < 0 || y < 0) return false;
    if (!_map->inBounds(static_cast<uint16_t>(x), static_cast<uint16_t>(y)))
        return false;
    tx = static_cast<uint16_t>(x);
    ty = static_cast<uint16_t>(y);
    return true;
}

void MapCanvas::paintTileAt(uint16_t tx, uint16_t ty) {
    if (!_map) return;
    Tile& t    = _map->at(tx, ty);
    t.type     = _activeTileType;
    t.zone     = _activeZoneType;
    t.walkable = _activeWalkable;
    update();
    emit tileChanged(tx, ty);
}

void MapCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        _painting = true;
        uint16_t tx, ty;
        if (screenToTile(event->pos(), tx, ty)) {
            paintTileAt(tx, ty);
            emit mapClicked(tx, ty);
        }
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (_painting && (event->buttons() & Qt::LeftButton)) {
        uint16_t tx, ty;
        if (screenToTile(event->pos(), tx, ty))
            paintTileAt(tx, ty);
    }
}

void MapCanvas::wheelEvent(QWheelEvent*) {
    // Zoom para iteraciones futuras
}

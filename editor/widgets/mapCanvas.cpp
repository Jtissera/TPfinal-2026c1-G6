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
        case TileType::GRASS:            base = QColor(100, 160,  70); break;
        case TileType::WATER:            base = QColor( 60, 120, 200); break;
        case TileType::WALL:             base = QColor( 80,  80,  80); break;
        case TileType::FLOOR:            base = QColor(180, 150, 100); break;
        case TileType::DOOR:             base = QColor(160, 100,  50); break;
        case TileType::DUNGEON_ENTRANCE: base = QColor( 80,  20, 120); break;
        default:                         base = QColor(200, 200, 200); break;
    }
    if (tile.zone == ZoneType::COMBAT) base = base.darker(130);
    if (!tile.walkable)                base = base.darker(160);
    return base;
}

void MapCanvas::drawNpcIndicator(QPainter& painter, const QRect& r, NpcType npc) const {
    if (npc == NpcType::NONE) return;

    QColor color;
    QString label;
    switch (npc) {
        case NpcType::PRIEST:   color = Qt::white;            label = "S";  break;
        case NpcType::MERCHANT: color = Qt::yellow;           label = "C";  break;
        case NpcType::BANKER:   color = Qt::cyan;             label = "B";  break;
        case NpcType::GOBLIN:   color = Qt::red;              label = "G";  break;
        case NpcType::SKELETON: color = Qt::gray;             label = "E";  break;
        case NpcType::ZOMBIE:   color = QColor(150, 200, 50); label = "Z";  break;
        case NpcType::GUARD:    color = Qt::blue;             label = "Gu"; break;
        default: return;
    }

    QRect indicator(r.x() + 2, r.y() + 2, 18, 18);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(indicator);

    painter.setPen(color);
    QFont f = painter.font();
    f.setBold(true);
    f.setPixelSize(11);
    painter.setFont(f);
    painter.drawText(indicator, Qt::AlignCenter, label);
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

            // Color base del tile
            painter.save();
            painter.setBrush(tileColor(tile));
            painter.setPen(Qt::NoPen);
            painter.drawRect(r);
            painter.restore();

            // Entrada a mazmorra
            if (tile.type == TileType::DUNGEON_ENTRANCE) {
                painter.save();
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(200, 100, 255), 2));
                painter.drawRect(r.adjusted(1, 1, -1, -1));
                painter.setPen(Qt::white);
                QFont f = painter.font();
                f.setPixelSize(9);
                painter.setFont(f);
                painter.drawText(r.adjusted(0, 14, 0, 0), Qt::AlignCenter, "ENT");
                painter.restore();
            }

            // Grid
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QColor(0, 0, 0, 60));
            painter.drawRect(r);
            painter.restore();

            // Indicador de NPC
            if (tile.npc != NpcType::NONE) {
                painter.save();
                drawNpcIndicator(painter, r, tile.npc);
                painter.restore();
            }
        }
    }

    // Borde indicador de modo NPC
    if (_editMode == EditMode::NPCS) {
        painter.save();
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 200, 0), 4));
        painter.drawRect(rect().adjusted(2, 2, -2, -2));
        painter.restore();
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

void MapCanvas::applyToTile(uint16_t tx, uint16_t ty) {
    if (!_map) return;
    Tile& t = _map->at(tx, ty);

    if (_editMode == EditMode::TILES) {
        t.type     = _activeTileType;
        t.zone     = _activeZoneType;
        t.walkable = _activeWalkable;
        if (_activeTileType == TileType::DUNGEON_ENTRANCE) {
            t.zone     = ZoneType::COMBAT;
            t.walkable = true;
        }
    } else if (_editMode == EditMode::NPCS) {
        t.npc = _activeNpc;
    }

    update();
    emit tileChanged(tx, ty);
}

void MapCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        _painting = true;
        uint16_t tx, ty;
        if (screenToTile(event->pos(), tx, ty)) {
            applyToTile(tx, ty);
            emit mapClicked(tx, ty);
        }
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (!_painting) return;
    if (!(event->buttons() & Qt::LeftButton)) return;
    if (_editMode == EditMode::NPCS) return;

    uint16_t tx, ty;
    if (screenToTile(event->pos(), tx, ty))
        applyToTile(tx, ty);
}

void MapCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        _painting = false;
}

void MapCanvas::wheelEvent(QWheelEvent*) {
    // Zoom para iteraciones futuras
}

#include "mapCanvas.h"
#include <QPainter>
#include <QMouseEvent>

MapCanvas::MapCanvas(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(200, 200);
}

void MapCanvas::setMap(MapData *map)
{
    _map = map;
    if (_map)
    {
        setMinimumSize(
            _map->width() * tileSize(),
            _map->height() * tileSize());
    }
    update();
}

QColor MapCanvas::tileColor(const Tile &tile) const
{
    QColor base;
    switch (tile.type)
    {
    case TileType::GRASS:
        base = QColor(100, 160, 70);
        break;
    case TileType::SAND:
        base = QColor(210, 185, 110);
        break;
    case TileType::WATER:
        base = QColor(60, 120, 200);
        break;
    case TileType::WALL:
        base = QColor(80, 80, 80);
        break;
    case TileType::FLOOR:
        base = QColor(180, 150, 100);
        break;
    case TileType::DOOR:
        base = QColor(160, 100, 50);
        break;
    case TileType::FOREST:
        base = QColor(34, 90, 34);
        break;
    case TileType::DUNGEON_ENTRANCE:
        base = QColor(80, 20, 120);
        break;
    case TileType::CAVERN_ENTRANCE:
        base = QColor(100, 50, 150);
        break;
    case TileType::EXIT:
        base = QColor(50, 180, 80);
        break;
    default:
        base = QColor(200, 200, 200);
        break;
    }

    // Overlay de zona encima del color base
    switch (tile.zone)
    {
    case ZoneType::CITY:
        // borde dorado — el overlay lo aplicamos en paintEvent, acá solo oscurecemos un poco
        break;
    case ZoneType::COMBAT:
        base = base.darker(130);
        break;
    case ZoneType::DESERT:
        // arena ya es visualmente desierto; si el tile es pasto, teñimos
        if (tile.type == TileType::GRASS)
            base = QColor(210, 185, 110).darker(110);
        break;
    case ZoneType::FOREST:
        if (tile.type == TileType::GRASS)
            base = QColor(60, 120, 50);
        break;
    default:
        break;
    }

    if (!tile.walkable)
    {
        // Hatching visual: oscurecer + tinte rojizo para no caminable
        base = base.darker(150);
        base = QColor(
            qMin(base.red() + 40, 255),
            base.green(),
            base.blue());
    }
    return base;
}

void MapCanvas::drawNpcIndicator(QPainter &painter, const QRect &r, NpcType npc) const
{
    if (npc == NpcType::NONE)
        return;

    QColor color;
    QString label;
    switch (npc)
    {
    case NpcType::PRIEST:
        color = Qt::white;
        label = "S";
        break;
    case NpcType::MERCHANT:
        color = Qt::yellow;
        label = "C";
        break;
    case NpcType::BANKER:
        color = Qt::cyan;
        label = "B";
        break;
    case NpcType::GOBLIN:
        color = Qt::red;
        label = "G";
        break;
    case NpcType::SKELETON:
        color = Qt::gray;
        label = "E";
        break;
    case NpcType::ZOMBIE:
        color = QColor(150, 200, 50);
        label = "Z";
        break;
    case NpcType::GUARD:
        color = Qt::blue;
        label = "Gu";
        break;
    case NpcType::GOBLIN_CAVE:
        color = QColor(180, 80, 80);
        label = "GC";
        break;
    case NpcType::SKELETON_CAVE:
        color = QColor(160, 160, 180);
        label = "SC";
        break;
    case NpcType::SPIDER_CAVE:
        color = QColor(140, 60, 180);
        label = "AC";
        break;
    case NpcType::GOLEM_CAVE:
        color = QColor(100, 100, 140);
        label = "GoC";
        break;
    case NpcType::GOBLIN_DUNGEON:
        color = QColor(220, 50, 50);
        label = "GD";
        break;
    case NpcType::SKELETON_DUNGEON:
        color = QColor(200, 200, 220);
        label = "SD";
        break;
    case NpcType::SPIDER_DUNGEON:
        color = QColor(180, 30, 220);
        label = "AD";
        break;
    case NpcType::GOLEM_DUNGEON:
        color = QColor(80, 80, 200);
        label = "GoD";
        break;
    case NpcType::GOBLIN_DESERT:
        color = QColor(210, 160, 50);
        label = "GDe";
        break;
    case NpcType::SKELETON_DESERT:
        color = QColor(200, 180, 120);
        label = "SDe";
        break;
    case NpcType::SPIDER_DESERT:
        color = QColor(180, 140, 30);
        label = "ADe";
        break;
    case NpcType::GOLEM_DESERT:
        color = QColor(160, 130, 80);
        label = "GoDe";
        break;
    default:
        return;
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

void MapCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));
    const int ts = tileSize();

    if (!_map)
    {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Sin mapa cargado");
        return;
    }

    for (uint16_t y = 0; y < _map->height(); ++y)
    {
        for (uint16_t x = 0; x < _map->width(); ++x)
        {
            const Tile &tile = _map->at(x, y);
            QRect r(x * ts, y * ts, ts, ts);

            // Color base del tile
            painter.save();
            painter.setBrush(tileColor(tile));
            painter.setPen(Qt::NoPen);
            painter.drawRect(r);
            painter.restore();

            // Entrada a mazmorra
            if (tile.type == TileType::DUNGEON_ENTRANCE)
            {
                painter.save();
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(200, 100, 255), 2));
                painter.drawRect(r.adjusted(1, 1, -1, -1));
                painter.setPen(Qt::white);
                QFont f = painter.font();
                f.setPixelSize(9);
                painter.setFont(f);
                painter.drawText(r.adjusted(0, ts / 2, 0, 0), Qt::AlignCenter, "ENT");
                painter.restore();
            }

            if (tile.type == TileType::CAVERN_ENTRANCE)
            {
                painter.save();
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(150, 100, 255), 2));
                painter.drawRect(r.adjusted(1, 1, -1, -1));
                painter.setPen(Qt::white);
                QFont f = painter.font();
                f.setPixelSize(9);
                painter.setFont(f);
                painter.drawText(r.adjusted(0, ts / 2, 0, 0), Qt::AlignCenter, "CAV");
                painter.restore();
            }

            if ((tile.type == TileType::DUNGEON_ENTRANCE ||
                 tile.type == TileType::CAVERN_ENTRANCE) &&
                !tile.targetMap.empty())
            {
                painter.save();
                painter.setBrush(QColor(0, 220, 80));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(r.right() - 8, r.top() + 2, 6, 6);
                painter.restore();
            }

            if (tile.zone == ZoneType::CITY)
            {
                painter.save();
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(220, 180, 50, 180), 1));
                painter.drawRect(r.adjusted(1, 1, -1, -1));
                painter.restore();
            }

            // Símbolo de árbol para FOREST
            if (tile.type == TileType::FOREST)
            {
                painter.save();
                painter.setPen(QColor(150, 220, 100));
                QFont f = painter.font();
                f.setPixelSize(14);
                painter.setFont(f);
                painter.drawText(r, Qt::AlignCenter, "▲");
                painter.restore();
            }

            // Hatching para no caminable (X encima del color oscurecido)
            if (!tile.walkable && tile.type != TileType::FOREST && tile.type != TileType::WALL && tile.type != TileType::WATER)
            {
                painter.save();
                painter.setPen(QPen(QColor(200, 60, 60, 120), 1));
                painter.drawLine(r.topLeft(), r.bottomRight());
                painter.drawLine(r.topRight(), r.bottomLeft());
                painter.restore();
            }

            // Grid
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QColor(0, 0, 0, 60));
            painter.drawRect(r);
            painter.restore();

            // Indicador de NPC
            if (tile.npc != NpcType::NONE)
            {
                painter.save();
                drawNpcIndicator(painter, r, tile.npc);
                painter.restore();
            }
        }
    }

    // Borde indicador de modo NPC
    if (_editMode == EditMode::NPCS)
    {
        painter.save();
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 200, 0), 4));
        painter.drawRect(rect().adjusted(2, 2, -2, -2));
        painter.restore();
    }
}

bool MapCanvas::screenToTile(const QPoint &pos, uint16_t &tx, uint16_t &ty) const
{
    if (!_map)
        return false;
    int x = pos.x() / tileSize();
    int y = pos.y() / tileSize();
    if (x < 0 || y < 0)
        return false;
    if (!_map->inBounds(static_cast<uint16_t>(x), static_cast<uint16_t>(y)))
        return false;
    tx = static_cast<uint16_t>(x);
    ty = static_cast<uint16_t>(y);
    return true;
}

void MapCanvas::applyToTile(uint16_t tx, uint16_t ty)
{
    if (!_map)
        return;
    Tile &t = _map->at(tx, ty);

    if (_editMode == EditMode::TILES)
    {
        t.type = _activeTileType;
        t.zone = _activeZoneType;
        t.walkable = _activeWalkable;

        // Comportamientos automáticos por tipo
        switch (_activeTileType)
        {
        case TileType::FOREST:
            // Bosque siempre no caminable
            t.walkable = false;
            break;
        case TileType::CAVERN_ENTRANCE:
            t.zone = ZoneType::CAVERN;
            t.walkable = true;
            break;
        case TileType::DUNGEON_ENTRANCE:
            t.zone = ZoneType::DUNGEON;
            t.walkable = true;
            break;
        case TileType::EXIT:
            t.walkable = true;
            break;
        default:
            break;
        }
    }
    else if (_editMode == EditMode::NPCS)
    {
        t.npc = _activeNpc;
    }

    update();
    emit tileChanged(tx, ty);
}

void MapCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        _painting = true;
        uint16_t tx, ty;
        if (screenToTile(event->pos(), tx, ty))
        {
            applyToTile(tx, ty);
            emit mapClicked(tx, ty);
        }
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (!_painting)
        return;
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if (_editMode == EditMode::NPCS)
        return;

    uint16_t tx, ty;
    if (screenToTile(event->pos(), tx, ty))
        applyToTile(tx, ty);
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        _painting = false;
}

void MapCanvas::wheelEvent(QWheelEvent *event)
{
    float delta = (event->angleDelta().y() > 0) ? ZOOM_STEP : -ZOOM_STEP;
    _zoomFactor = std::clamp(_zoomFactor + delta, ZOOM_MIN, ZOOM_MAX);

    if (_map)
    {
        setMinimumSize(
            _map->width() * tileSize(),
            _map->height() * tileSize());
    }
    update();
    event->accept();
}

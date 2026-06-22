#include "mainWindow.h"

#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QCloseEvent>
#include <QFileInfo>

#include "map/mapSerializer.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setMinimumSize(900, 600);
    setupMenuBar();
    setupCentralWidget();
    setupStatusBar();
    updateTitle();

    _map = std::make_unique<MapData>(20, 15);
    _canvas->setMap(_map.get());
    _tilePalette->setMapType(_map->mapType());
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("&Archivo");

    fileMenu->addAction("&Nuevo mapa", this, &MainWindow::onNewMap, QKeySequence::New);
    fileMenu->addAction("&Abrir mapa", this, &MainWindow::onOpenMap, QKeySequence::Open);
    fileMenu->addSeparator();
    _actSave = fileMenu->addAction("&Guardar", this, &MainWindow::onSaveMap, QKeySequence::Save);
    _actSaveAs = fileMenu->addAction("Guardar &como...", this, &MainWindow::onSaveMapAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Salir", qApp, &QApplication::quit, QKeySequence::Quit);

    QMenu *helpMenu = menuBar()->addMenu("A&yuda");
    helpMenu->addAction("Acerca de", this, [this]()
                        { QMessageBox::about(this, "Editor de Mapas",
                                             "Argentum Online - Editor de Mapas\n"
                                             "Taller de Programación I - FIUBA\n\n"
                                             "Pestaña Tiles: pintá el terreno y las zonas.\n"
                                             "Pestaña NPCs: colocá personajes y criaturas."); });
}

void MainWindow::setupCentralWidget()
{
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _tabs = new QTabWidget(central);
    _tabs->setFixedWidth(170);

    _tilePalette = new TilePalette();
    connect(_tilePalette, &TilePalette::selectionChanged,
            this, &MainWindow::onPaletteChanged);
    _tabs->addTab(_tilePalette, "Tiles");

    _npcPalette = new NpcPalette();
    connect(_npcPalette, &NpcPalette::selectionChanged,
            this, &MainWindow::onNpcPaletteChanged);
    _tabs->addTab(_npcPalette, "NPCs");

    connect(_tabs, &QTabWidget::currentChanged,
            this, &MainWindow::onTabChanged);

    _tabs->setCurrentIndex(0);

    layout->addWidget(_tabs);

    _scrollArea = new QScrollArea(central);
    _canvas = new MapCanvas(_scrollArea);
    _canvas->setEditMode(EditMode::TILES);
    onPaletteChanged();
    connect(_canvas, &MapCanvas::tileChanged, this,
            [this](uint16_t x, uint16_t y)
            {
                if (!_unsavedChanges)
                {
                    _unsavedChanges = true;
                    updateTitle();
                }
                if (!_map)
                    return;
                const Tile &t = _map->at(x, y);
                QString npcStr = (t.npc != NpcType::NONE)
                                     ? QString(" | NPC: %1").arg(QString::fromStdString(npcTypeName(t.npc)))
                                     : "";
                _coordLabel->setText(
                    QString("  Tile: (%1, %2)%3").arg(x).arg(y).arg(npcStr));
            });
    connect(_canvas, &MapCanvas::mapClicked, this,
            [this](uint16_t x, uint16_t y)
            {
                if (!_map)
                    return;
                Tile &t = _map->at(x, y);

                if (t.type == TileType::DUNGEON_ENTRANCE ||
                    t.type == TileType::CAVERN_ENTRANCE)
                {
                    bool ok;
                    QString current = QString::fromStdString(t.targetMap);
                    QString label = (t.type == TileType::DUNGEON_ENTRANCE)
                                        ? "Archivo .argmap de la mazmorra:"
                                        : "Archivo .argmap de la caverna:";
                    QString path = QInputDialog::getText(
                        this, "Mapa destino", label,
                        QLineEdit::Normal, current, &ok);
                    if (ok)
                    {
                        t.targetMap = path.toStdString();
                        _unsavedChanges = true;
                        updateTitle();
                    }
                }

                const Tile &tc = _map->at(x, y);
                QString npcStr = (tc.npc != NpcType::NONE)
                                     ? QString(" | NPC: %1").arg(QString::fromStdString(npcTypeName(tc.npc)))
                                     : "";
                QString mapStr = !tc.targetMap.empty()
                                     ? QString(" → %1").arg(
                                           QString::fromStdString(tc.targetMap))
                                     : "";
                _coordLabel->setText(
                    QString("  Tile: (%1, %2)%3%4").arg(x).arg(y).arg(npcStr).arg(mapStr));
            });

    _scrollArea->setWidget(_canvas);
    _scrollArea->setWidgetResizable(false);
    _scrollArea->setAlignment(Qt::AlignCenter);
    layout->addWidget(_scrollArea, 1);

    setCentralWidget(central);
}

void MainWindow::setupStatusBar()
{
    _coordLabel = new QLabel("  Tile: (-, -)", this);
    statusBar()->addWidget(_coordLabel);
    statusBar()->showMessage("Listo — Pestaña Tiles activa");
}

void MainWindow::onNewMap()
{
    if (!confirmUnsavedChanges())
        return;

    bool okW, okH;
    int w = QInputDialog::getInt(this, "Nuevo mapa", "Ancho (tiles):",
                                 20, MAP_MIN_SIZE, MAP_MAX_WIDTH, 1, &okW);
    if (!okW)
        return;
    int h = QInputDialog::getInt(this, "Nuevo mapa", "Alto (tiles):",
                                 15, MAP_MIN_SIZE, MAP_MAX_HEIGHT, 1, &okH);
    if (!okH)
        return;

    QStringList tipos = {"Mundo principal", "Mazmorra", "Caverna"};
    bool okT;
    QString tipoStr = QInputDialog::getItem(
        this, "Tipo de mapa", "¿Qué tipo de mapa es?",
        tipos, 0, false, &okT);
    if (!okT)
        return;

    MapType tipo = MapType::WORLD;
    if (tipoStr == "Mazmorra")
        tipo = MapType::DUNGEON;
    else if (tipoStr == "Caverna")
        tipo = MapType::CAVE;

    _map = std::make_unique<MapData>(
        static_cast<uint16_t>(w),
        static_cast<uint16_t>(h),
        tipo);

    _canvas->setMap(_map.get());
    _tilePalette->setMapType(_map->mapType());
    _currentFilePath.clear();
    _unsavedChanges = false;
    updateTitle();
    statusBar()->showMessage("Nuevo mapa creado");
}

void MainWindow::onOpenMap()
{
    if (!confirmUnsavedChanges())
        return;

    QString path = QFileDialog::getOpenFileName(
        this, "Abrir mapa", "",
        "Mapas de Argentum (*.argmap);;Todos los archivos (*)");
    if (path.isEmpty())
        return;

    try
    {
        MapData loaded = MapSerializer::load(path.toStdString());
        _map = std::make_unique<MapData>(std::move(loaded));
        _canvas->setMap(_map.get());
        _tilePalette->setMapType(_map->mapType());
        _currentFilePath = path;
        _unsavedChanges = false;
        updateTitle();
        statusBar()->showMessage("Mapa cargado: " + path);
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Error al abrir",
                              QString("No se pudo cargar:\n%1").arg(e.what()));
    }
}

void MainWindow::onSaveMap()
{
    if (_currentFilePath.isEmpty())
    {
        onSaveMapAs();
        return;
    }
    try
    {
        MapSerializer::save(*_map, _currentFilePath.toStdString());
        _unsavedChanges = false;
        updateTitle();
        statusBar()->showMessage("Guardado: " + _currentFilePath);
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Error al guardar",
                              QString("No se pudo guardar:\n%1").arg(e.what()));
    }
}

void MainWindow::onSaveMapAs()
{
    if (!_map)
        return;

    QString defaultPath = "assets/sprites/MapAssets/worlds/";

    switch (_map->mapType())
    {
    case MapType::DUNGEON:
        defaultPath += "mazmorra/";
        break;
    case MapType::CAVE:
        defaultPath += "caverna/";
        break;
    case MapType::WORLD:
    default:
        break;
    }

    defaultPath += QString::fromStdString(_map->name()) + ".argmap";

    QString path = QFileDialog::getSaveFileName(
        this,
        "Guardar mapa como",
        defaultPath,
        "Mapas de Argentum (*.argmap);;Todos los archivos (*)");

    if (path.isEmpty())
        return;

    if (!path.endsWith(".argmap"))
        path += ".argmap";

    _currentFilePath = path;
    onSaveMap();
}

void MainWindow::onPaletteChanged()
{
    if (!_canvas)
        return;
    _canvas->setActiveTileType(_tilePalette->selectedTileType());
    _canvas->setActiveZoneType(_tilePalette->selectedZoneType());
    _canvas->setActiveWalkable(_tilePalette->selectedWalkable());
}

void MainWindow::onNpcPaletteChanged()
{
    if (!_canvas)
        return;
    _canvas->setActiveNpc(_npcPalette->selectedNpc());
}

void MainWindow::onTabChanged(int index)
{
    if (!_canvas)
        return;
    if (index == 0)
    {
        _canvas->setEditMode(EditMode::TILES);
        onPaletteChanged();
        statusBar()->showMessage("Pestaña Tiles activa — click para pintar terreno");
    }
    else
    {
        _canvas->setEditMode(EditMode::NPCS);
        onNpcPaletteChanged();
        statusBar()->showMessage("Pestaña NPCs activa — click para colocar/borrar NPCs");
    }
}

void MainWindow::updateTitle()
{
    QString title = "Editor de Mapas - Argentum Online";
    if (!_currentFilePath.isEmpty())
    {
        QFileInfo fi(_currentFilePath);
        title += " [" + fi.fileName() + "]";
    }

    if (_map)
    {
        QString tipoStr;
        switch (_map->mapType())
        {
        case MapType::WORLD:
            tipoStr = "Mundo";
            break;
        case MapType::DUNGEON:
            tipoStr = "Mazmorra";
            break;
        case MapType::CAVE:
            tipoStr = "Caverna";
            break;
        }
        title += " [" + tipoStr + "]";
    }
    if (_unsavedChanges)
        title += " *";

    setWindowTitle(title);
}

bool MainWindow::confirmUnsavedChanges()
{
    if (!_unsavedChanges)
        return true;
    auto btn = QMessageBox::question(
        this, "Cambios sin guardar",
        "Hay cambios sin guardar. ¿Querés continuar y perderlos?",
        QMessageBox::Yes | QMessageBox::No);
    return btn == QMessageBox::Yes;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsavedChanges())
    {
        event->ignore();
        return;
    }
    event->accept();
}

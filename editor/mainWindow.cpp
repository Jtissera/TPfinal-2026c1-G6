#include "mainWindow.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QLabel>

#include <QApplication>
#include "map/mapSerializer.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setMinimumSize(900, 600);
    setupMenuBar();
    setupCentralWidget();
    setupStatusBar();
    updateTitle();

    // Mapa inicial vacio 20x15
    _map = std::make_unique<MapData>(20, 15);
    _canvas->setMap(_map.get());
}

// -------------------------------- Setup --------------------------------

void MainWindow::setupMenuBar() {
    // Menu de Archivo
    QMenu* fileMenu = menuBar()->addMenu("&Archivo");

    QAction* actNew  = fileMenu->addAction("&Nuevo mapa",  this, &MainWindow::onNewMap,  QKeySequence::New);
    QAction* actOpen = fileMenu->addAction("&Abrir mapa",  this, &MainWindow::onOpenMap, QKeySequence::Open);
    fileMenu->addSeparator();
    _actSave   = fileMenu->addAction("&Guardar",    this, &MainWindow::onSaveMap,   QKeySequence::Save);
    _actSaveAs = fileMenu->addAction("Guardar &como...", this, &MainWindow::onSaveMapAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Salir", qApp, &QApplication::quit, QKeySequence::Quit);

    (void)actNew; (void)actOpen;

    // Menu de Ayuda
    QMenu* helpMenu = menuBar()->addMenu("A&yuda");
    helpMenu->addAction("Acerca de", this, [this]() {
        QMessageBox::about(this, "Editor de Mapas",
            "Argentum Online - Editor de Mapas\n"
            "Taller de Programación I - FIUBA\n\n"
            "Click izquierdo + arrastrar para pintar tiles.");
    });
}

void MainWindow::setupCentralWidget() {
    auto* central = new QWidget(this);
    auto* layout  = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _palette = new TilePalette(central);
    connect(_palette, &TilePalette::selectionChanged,
            this, &MainWindow::onPaletteChanged);
    layout->addWidget(_palette);
    _scrollArea = new QScrollArea(central);
    _canvas = new MapCanvas(_scrollArea);
    connect(_canvas, &MapCanvas::tileChanged,
            this, &MainWindow::onTileChanged);
    connect(_canvas, &MapCanvas::mapClicked, this,
            [this](uint16_t x, uint16_t y) {
                _coordLabel->setText(QString("  Tile: (%1, %2)").arg(x).arg(y));
            });

    _scrollArea->setWidget(_canvas);
    _scrollArea->setWidgetResizable(false);
    _scrollArea->setAlignment(Qt::AlignCenter);
    layout->addWidget(_scrollArea, 1);

    setCentralWidget(central);
}

void MainWindow::setupStatusBar() {
    _coordLabel = new QLabel("  Tile: (-, -)", this);
    statusBar()->addWidget(_coordLabel);
    statusBar()->showMessage("Listo");
}

// -------------------------------- Slots --------------------------------

void MainWindow::onNewMap() {
    if (!confirmUnsavedChanges()) return;

    bool okW, okH;
    int w = QInputDialog::getInt(this, "Nuevo mapa", "Ancho (tiles):",
                                 20, MAP_MIN_SIZE, MAP_MAX_WIDTH, 1, &okW);
    if (!okW) return;
    int h = QInputDialog::getInt(this, "Nuevo mapa", "Alto (tiles):",
                                 15, MAP_MIN_SIZE, MAP_MAX_HEIGHT, 1, &okH);
    if (!okH) return;

    _map = std::make_unique<MapData>(static_cast<uint16_t>(w),
                                     static_cast<uint16_t>(h));
    _canvas->setMap(_map.get());
    _currentFilePath.clear();
    _unsavedChanges = false;
    updateTitle();
    statusBar()->showMessage("Nuevo mapa creado");
}

void MainWindow::onOpenMap() {
    if (!confirmUnsavedChanges()) return;

    QString path = QFileDialog::getOpenFileName(
        this, "Abrir mapa", "",
        "Mapas de Argentum (*.argmap);;Todos los archivos (*)");
    if (path.isEmpty()) return;

    try {
        MapData loaded = MapSerializer::load(path.toStdString());
        _map = std::make_unique<MapData>(std::move(loaded));
        _canvas->setMap(_map.get());
        _currentFilePath = path;
        _unsavedChanges  = false;
        updateTitle();
        statusBar()->showMessage("Mapa cargado: " + path);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error al abrir",
                              QString("No se pudo cargar el mapa:\n%1").arg(e.what()));
    }
}

void MainWindow::onSaveMap() {
    if (_currentFilePath.isEmpty()) {
        onSaveMapAs();
        return;
    }
    try {
        MapSerializer::save(*_map, _currentFilePath.toStdString());
        _unsavedChanges = false;
        updateTitle();
        statusBar()->showMessage("Guardado: " + _currentFilePath);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error al guardar",
                              QString("No se pudo guardar:\n%1").arg(e.what()));
    }
}

void MainWindow::onSaveMapAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Guardar mapa como", "",
        "Mapas de Argentum (*.argmap);;Todos los archivos (*)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".argmap")) path += ".argmap";
    _currentFilePath = path;
    onSaveMap();
}

void MainWindow::onPaletteChanged() {
    if (!_canvas) return;
    _canvas->setActiveTileType(_palette->selectedTileType());
    _canvas->setActiveZoneType(_palette->selectedZoneType());
    _canvas->setActiveWalkable(_palette->selectedWalkable());
}

void MainWindow::onTileChanged(uint16_t, uint16_t) {
    if (!_unsavedChanges) {
        _unsavedChanges = true;
        updateTitle();
    }
}

// -------------------------------- Helpers --------------------------------

void MainWindow::updateTitle() {
    QString title = "Editor de Mapas - Argentum Online";
    if (!_currentFilePath.isEmpty()) {
        QFileInfo fi(_currentFilePath);
        title += " [" + fi.fileName() + "]";
    }
    if (_unsavedChanges) title += " *";
    setWindowTitle(title);
}

bool MainWindow::confirmUnsavedChanges() {
    if (!_unsavedChanges) return true;
    auto btn = QMessageBox::question(
        this, "Cambios sin guardar",
        "Hay cambios sin guardar. ¿Querés continuar y perderlos?",
        QMessageBox::Yes | QMessageBox::No);
    return btn == QMessageBox::Yes;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!confirmUnsavedChanges()) {
        event->ignore();
        return;
    }
    event->accept();
}

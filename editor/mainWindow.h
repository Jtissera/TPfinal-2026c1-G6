#pragma once

#include <QMainWindow>
#include <QScrollArea>
#include <QLabel>
#include <QAction>
#include <QCloseEvent>
#include <memory>

#include "map/mapData.h"
#include "widgets/mapCanvas.h"
#include "widgets/tilePalette.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNewMap();
    void onOpenMap();
    void onSaveMap();
    void onSaveMapAs();
    void onPaletteChanged();
    void onTileChanged(uint16_t x, uint16_t y);

private:
    void setupMenuBar();
    void setupStatusBar();
    void setupCentralWidget();
    void updateTitle();
    bool confirmUnsavedChanges();

    MapCanvas*    _canvas   = nullptr;
    TilePalette*  _palette  = nullptr;
    QLabel*       _coordLabel = nullptr;
    QScrollArea*  _scrollArea = nullptr;
    std::unique_ptr<MapData> _map;
    QString   _currentFilePath;
    bool      _unsavedChanges = false;
    QAction* _actSave   = nullptr;
    QAction* _actSaveAs = nullptr;
};

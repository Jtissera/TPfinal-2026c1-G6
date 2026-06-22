#pragma once

#include <QMainWindow>
#include <QScrollArea>
#include <QLabel>
#include <QAction>
#include <QCloseEvent>
#include <QTabWidget>
#include <memory>

#include "map/mapData.h"
#include "widgets/mapCanvas.h"
#include "widgets/tilePalette.h"
#include "widgets/npcPalette.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewMap();
    void onOpenMap();
    void onSaveMap();
    void onSaveMapAs();
    void onPaletteChanged();
    void onNpcPaletteChanged();
    void onTabChanged(int index);

private:
    void setupMenuBar();
    void setupStatusBar();
    void setupCentralWidget();
    void updateTitle();
    bool confirmUnsavedChanges();

    MapCanvas *_canvas = nullptr;
    TilePalette *_tilePalette = nullptr;
    NpcPalette *_npcPalette = nullptr;
    QTabWidget *_tabs = nullptr;
    QLabel *_coordLabel = nullptr;
    QScrollArea *_scrollArea = nullptr;

    std::unique_ptr<MapData> _map;
    QString _currentFilePath;
    bool _unsavedChanges = false;

    QAction *_actSave = nullptr;
    QAction *_actSaveAs = nullptr;
};

#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Argentum Online - Editor de mapas");
    app.setApplicationVersion("0.1.0");

    MainWindow window;
    window.show();

    return app.exec();
}

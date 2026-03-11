#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setStyle("Fusion");
    app.setStyleSheet(
        "QMainWindow { background-color: #090C10; color: #FFFFFF; }"
        "QWidget { background-color: #090C10; color: #FFFFFF; }"
        "QPushButton {"
        "  background-color: #1E283C; border: 1px solid #32405A;"
        "  color: #FFFFFF; padding: 5px; border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #283655; }"
        "QPushButton:pressed { background-color: #00FF44; color: #000000; }"
        "QListWidget { background-color: #202020; color: #FFFFFF; border: 1px solid #32405A; }"
        "QTextEdit { background-color: #202020; color: #00FF44; font-family: Consolas; border: 1px solid #32405A; }"
        "QLabel { color: #FFFFFF; }"
    );

    MainWindow window;
    window.showMaximized();

    return app.exec();
}

#include <QApplication>
#include "PolygonEditor.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    PolygonEditor editor;
    editor.setWindowTitle("Map Region Polygon Editor");
    editor.resize(1400, 820);
    editor.show();

    return app.exec();
}

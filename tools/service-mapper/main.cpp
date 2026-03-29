#include <QApplication>
#include "ServiceMapper.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    ServiceMapper mapper;
    mapper.setWindowTitle("Service to Region Mapper");
    mapper.resize(700, 500);
    mapper.show();

    return app.exec();
}

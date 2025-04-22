#include "ScreenshotTool.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    ScreenshotTool tool;
    tool.start();
    
    return a.exec();
} 
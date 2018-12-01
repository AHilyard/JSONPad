/**
 * @file main.cpp
 *
 * @date 11/27/2018
 * @author Anthony Hilyard
 * @brief
 */
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}

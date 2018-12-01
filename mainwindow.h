/**
 * @file mainwindow.h
 *
 * @date 11/27/2018
 * @author Anthony Hilyard
 * @brief
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

public slots:
	void newDocument();
	bool openDocument();
	bool closeDocument();
	bool save();
	bool saveAs();

protected:
	virtual void closeEvent(QCloseEvent *event);

private slots:
	void documentChanged();
	void updateWindowTitle();

	void on_actionPreferences_triggered();

	void on_actionFormat_JSON_triggered();

	void on_actionCompress_JSON_triggered();

private:
	bool saveDocument();
	Ui::MainWindow *ui;
	QFile _currentDocument;
	bool _unsavedChanges;
};

#endif // MAINWINDOW_H

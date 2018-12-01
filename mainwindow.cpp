/**
 * @file mainwindow.cpp
 *
 * @date 11/27/2018
 * @author Anthony Hilyard
 * @brief
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	_unsavedChanges(false)
{
	ui->setupUi(this);
	setWindowIcon(QIcon::fromTheme("emblem-documents"));

	connect(ui->centralWidget, &JsonEditor::textChanged, this, &MainWindow::documentChanged);
	connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newDocument);
	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openDocument);
	connect(ui->actionSave, &QAction::triggered, this, &MainWindow::save);
	connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
	connect(ui->actionUndo, &QAction::triggered, ui->centralWidget, &JsonEditor::undo);
	connect(ui->actionRedo, &QAction::triggered, ui->centralWidget, &JsonEditor::redo);
	connect(ui->centralWidget, &JsonEditor::undoAvailable, ui->actionUndo, &QAction::setEnabled);
	connect(ui->centralWidget, &JsonEditor::redoAvailable, ui->actionRedo, &QAction::setEnabled);
	connect(ui->actionFormat, &QAction::triggered, ui->centralWidget, &JsonEditor::setFormatted);
	connect(ui->centralWidget, &JsonEditor::documentFormatted, ui->actionFormat, &QAction::setChecked);

	updateWindowTitle();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::newDocument()
{
	closeDocument();
}

bool MainWindow::openDocument()
{
	QString selectedFilename = QFileDialog::getOpenFileName(this, "Select a JSON file to open.");

	if (!selectedFilename.isEmpty())
	{
		QFile selectedFile(selectedFilename);
		if (!selectedFile.open(QFile::ReadOnly))
		{
			// Could not open the file for some reason!
			return false;
		}

		ui->centralWidget->setText(selectedFile.readAll());
		selectedFile.close();

		_currentDocument.setFileName(selectedFilename);
		_unsavedChanges = false;
		updateWindowTitle();
		return true;
	}
	else
	{
		return false;
	}
}

bool MainWindow::closeDocument()
{
	if (_unsavedChanges)
	{
		switch(QMessageBox::warning(this, "There are unsaved changes!", "Would you like to save your changes before closing?", QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel))
		{
			case QMessageBox::Cancel:
				return false;
			case QMessageBox::Save:
				if (!save())
				{
					return false;
				}
			default:
			case QMessageBox::Discard:
				// Do nothing.
				break;
		}
	}

	ui->centralWidget->clear();
	_currentDocument.setFileName("");
	_unsavedChanges = false;
	updateWindowTitle();
	return true;
}

bool MainWindow::save()
{
	if (_currentDocument.fileName().isEmpty())
	{
		return saveAs();
	}

	return saveDocument();
}

bool MainWindow::saveAs()
{
	QString saveFilename = QFileDialog::getSaveFileName(this, "Save document as...");

	if (!saveFilename.isEmpty())
	{
		_currentDocument.setFileName(saveFilename);
	}

	return saveDocument();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (_unsavedChanges && !closeDocument())
	{
		event->ignore();
	}
	else
	{
		event->accept();
	}
}

void MainWindow::documentChanged()
{
	_unsavedChanges = true;
	updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
	QString documentName = _currentDocument.fileName();
	if (documentName.isEmpty())
	{
		documentName = "New Document";
	}
	if (_unsavedChanges)
	{
		documentName = "*" + documentName;
	}
	setWindowTitle(documentName + " - JSONPad");
}

bool MainWindow::saveDocument()
{
	if (!_currentDocument.open(QFile::WriteOnly))
	{
		// Could not open the file to write.
		return false;
	}

	if (!ui->centralWidget->text().isEmpty())
	{

		_currentDocument.write(ui->centralWidget->text().toUtf8().data());
	}

	_currentDocument.close();
	_unsavedChanges = false;
	updateWindowTitle();
	return true;
}

void MainWindow::on_actionPreferences_triggered()
{
	/// TODO: Show preferences window.
}

void MainWindow::on_actionFormat_JSON_triggered()
{
}

void MainWindow::on_actionCompress_JSON_triggered()
{
	ui->centralWidget->setText(QJsonDocument::fromJson(ui->centralWidget->text().toLatin1()).toJson(QJsonDocument::Compact));
}

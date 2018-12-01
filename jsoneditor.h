/**
 * @file jsoneditor.h
 *
 * @date 11/30/2018
 * @author Anthony Hilyard
 * @brief
 */
#ifndef JSONEDITOR_H
#define JSONEDITOR_H

#include <QPlainTextEdit>

class JsonMarginWidget;
class JsonEditor : public QPlainTextEdit
{
	Q_OBJECT
	friend class JsonMarginWidget;

public:
	explicit JsonEditor(QWidget *parent = nullptr);
	virtual ~JsonEditor();

	void setText(const QString &text);
	QString text();

	static QString formattedText(QString text);

public slots:
	void setFormatted(bool);

protected:
	void keyPressEvent(QKeyEvent *e);
	void paintEvent(QPaintEvent *e);

	bool eventFilter(QObject *, QEvent *);

signals:
	void documentFormatted(bool);

private slots:
	void updateText();
	void paintMarginWidget(QPaintEvent *e);

private:
	int formattedPosition(int position);
	int unformattedPosition(int position);
	int cursorPositionHelper(int pos, QString before, QString after);
	int positionOverLine(QPoint position);

	bool _formatDocument;
	QString _formattedText;
	QPlainTextEdit *_unformattedTextEdit;
	JsonMarginWidget *_marginWidget;
};

class JsonMarginWidget : public QWidget
{
	Q_OBJECT
public:
	explicit JsonMarginWidget(JsonEditor *parent);
	virtual ~JsonMarginWidget();

protected:
	void paintEvent(QPaintEvent *e);

private:
	JsonEditor *_editor;
};

#endif // JSONEDITOR_H

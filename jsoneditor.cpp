/**
 * @file jsoneditor.cpp
 *
 * @date 11/30/2018
 * @author Anthony Hilyard
 * @brief
 */
#include "jsoneditor.h"
#include <QJsonDocument>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QLayout>
#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QMainWindow>
#include <QScrollBar>
#include <QDateTime>

#define HIDDEN_CHAR		'\31'
#define ELLIPSES		"\u2060\u2026\u2060"

JsonMarginWidget::JsonMarginWidget(JsonEditor *parent) :
	QWidget(parent),
	_editor(parent)
{
	installEventFilter(parent);
}

JsonMarginWidget::~JsonMarginWidget()
{
}

void JsonMarginWidget::paintEvent(QPaintEvent *e)
{
	_editor->paintMarginWidget(e);
}

JsonEditor::JsonEditor(QWidget *parent) :
	QPlainTextEdit(parent),
	_formatDocument(false),
	_unformattedTextEdit(NULL)
{
	setViewportMargins(20, 0, 0, 0);

	_marginWidget = new JsonMarginWidget(this);

	_marginWidget->setFixedWidth(20);
	_marginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	setLayout(new QHBoxLayout());

	layout()->setMargin(0);
	layout()->addWidget(_marginWidget);
	layout()->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

	connect(this, &QPlainTextEdit::textChanged, this, &JsonEditor::updateText);
	emit documentFormatted(false);
}

JsonEditor::~JsonEditor()
{
}

void JsonEditor::setText(const QString &text)
{
	if (_unformattedTextEdit == NULL)
	{
		_unformattedTextEdit = new QPlainTextEdit();
		_unformattedTextEdit->setEnabled(true);
	}
	_unformattedTextEdit->setPlainText(text);
	setFormatted(_formatDocument);
}

QString JsonEditor::text()
{
	if (_unformattedTextEdit == NULL)
	{
		_unformattedTextEdit = new QPlainTextEdit(this);
		_unformattedTextEdit->setEnabled(true);
	}
	return _unformattedTextEdit->toPlainText();
}

void JsonEditor::setFormatted(bool formatted)
{
	// Set tab width to four spaces.
	setTabStopWidth(QFontMetrics(font()).width("    "));

	bool formattingChanged = _formatDocument != formatted;
	_formatDocument = formatted;

	int scrollBarPosition = verticalScrollBar()->value();
	blockSignals(true);
	if (_formatDocument)
	{
		_formattedText = formattedText(text());

		int cursorPosition = formattingChanged ? formattedPosition(textCursor().position()) : textCursor().position();
		int anchorPosition = formattingChanged ? (textCursor().anchor() != textCursor().position() ? formattedPosition(textCursor().anchor()) : cursorPosition) : textCursor().anchor();

		if (text() != _formattedText)
		{
			QTextCharFormat format = currentCharFormat();
			format.setForeground(Qt::darkBlue);
			setCurrentCharFormat(format);
		}

		QPlainTextEdit::setPlainText(_formattedText);

		QTextCursor cursor = textCursor();
		cursor.setPosition(anchorPosition);
		if (anchorPosition < cursorPosition)
		{
			cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, cursorPosition - anchorPosition);
		}
		else if (anchorPosition > cursorPosition)
		{
			cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, anchorPosition - cursorPosition);
		}

		setTextCursor(cursor);
	}
	else
	{
		int cursorPosition = unformattedPosition(textCursor().position());
		int anchorPosition = textCursor().anchor() != textCursor().position() ? unformattedPosition(textCursor().anchor()) : cursorPosition;

		QTextCharFormat format = currentCharFormat();
		format.setForeground(Qt::black);
		setCurrentCharFormat(format);

		QPlainTextEdit::setPlainText(text());

		QTextCursor cursor = textCursor();
		cursor.setPosition(anchorPosition);
		if (anchorPosition < cursorPosition)
		{
			cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, cursorPosition - anchorPosition);
		}
		else if (anchorPosition > cursorPosition)
		{
			cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, anchorPosition - cursorPosition);
		}
		setTextCursor(cursor);
	}

	emit documentFormatted(formatted);
	blockSignals(false);

	verticalScrollBar()->setValue(scrollBarPosition);
	_marginWidget->update();
}

void JsonEditor::keyPressEvent(QKeyEvent *keyEvent)
{
	if (_formatDocument && !keyEvent->text().isEmpty())
	{
		int cursorPosition = unformattedPosition(textCursor().position());
		int anchorPosition = textCursor().anchor() != textCursor().position() ? unformattedPosition(textCursor().anchor()) : cursorPosition;

		QTextCursor newCursor = _unformattedTextEdit->textCursor();
		newCursor.setPosition(anchorPosition);
		if (anchorPosition < cursorPosition)
		{
			newCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, cursorPosition - anchorPosition);
		}
		else if (anchorPosition > cursorPosition)
		{
			newCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, anchorPosition - cursorPosition);
		}

		// Move the cursor to the proper unformatted position.
		_unformattedTextEdit->setTextCursor(newCursor);

		int cursorOffset = 0;
		bool skipCharacter = false;

		// Skip closing quotes and braces.
		if (keyEvent->text() == "\"" || keyEvent->text() == "}" || keyEvent->text() == "]")
		{
			if (text().length() > newCursor.position() && text().at(newCursor.position()) == keyEvent->text())
			{
				skipCharacter = true;
				cursorOffset = -1;
			}
		}

		if (!skipCharacter)
		{
			// Forward the key event to the unformatted text edit.
			QApplication::sendEvent(_unformattedTextEdit, keyEvent);

			// Complete quotes.
			if (keyEvent->text() == "\"")
			{
				QKeyEvent matchingKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), "\"");
				QApplication::sendEvent(_unformattedTextEdit, &matchingKeyEvent);
				cursorOffset = 1;
			}

			// Complete braces.
			if (keyEvent->text() == "{")
			{
				QKeyEvent matchingKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), "}");
				QApplication::sendEvent(_unformattedTextEdit, &matchingKeyEvent);
				cursorOffset = 1;
			}

			// Complete brackets.
			if (keyEvent->text() == "[")
			{
				QKeyEvent matchingKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), "]");
				QApplication::sendEvent(_unformattedTextEdit, &matchingKeyEvent);
				cursorOffset = 1;
			}

			_formatDocument = false;
			setFormatted(true);
		}
		QTextCursor cursor = textCursor();
		cursor.setPosition(formattedPosition(_unformattedTextEdit->textCursor().position() - cursorOffset));
		setTextCursor(cursor);
	}
	else
	{
		QPlainTextEdit::keyPressEvent(keyEvent);
	}
}

void JsonEditor::paintEvent(QPaintEvent *e)
{
	QPlainTextEdit::paintEvent(e);
	_marginWidget->update();
}

bool JsonEditor::eventFilter(QObject *object, QEvent *event)
{
	if (object == _marginWidget && event->type() == QEvent::MouseButtonPress && _formatDocument)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

		int lineIndex = positionOverLine(mouseEvent->pos());
		QStringList lines = _formattedText.split("\n");
		if (lineIndex != -1 && lineIndex < lines.count())
		{
			int firstInsertionPosition, secondInsertionPosition;

			// Get the index of the '{' character within the clicked line...
			int characterPosition = -1;
			for (int i = 0; i < lineIndex; i++)
			{
				characterPosition = _formattedText.indexOf('\n', characterPosition + 1);
			}
			characterPosition = _formattedText.indexOf('{', characterPosition + 1);

			// Check if this bracket is within a quote.
			int quoteCount = _formattedText.mid(0, characterPosition).count("\"") -
							 _formattedText.mid(0, characterPosition).count("\\\"");

			// If we're at an odd number of quotes, we're inside a string so bail.
			if (quoteCount % 2 == 1)
			{
				return false;
			}

			// Insert the first hidden char here.
			firstInsertionPosition = unformattedPosition(characterPosition + 1);

			int previousCursorPosition;
			QString previousText;

			// If the section is already compressed, we're expanding it.
			if (_formattedText.mid(characterPosition + 2, 3) == ELLIPSES)
			{
				characterPosition = firstInsertionPosition;
				QString unformattedText = _unformattedTextEdit->toPlainText();

				int braceLevel = 1;
				bool insideQuotes = false;

				// First split by quotation marks. (But not by quotation marks preceded by a backslash)
				foreach (QString const &item, unformattedText.mid(characterPosition, unformattedText.length() - characterPosition).split(QRegularExpression(R"((?<!\\)\")")))
				{
					if (!insideQuotes)
					{
						for (int i = 0; i < item.length(); i++)
						{
							if (item.at(i) == '{')
							{
								braceLevel++;
							}
							else if (item.at(i) == '}')
							{
								braceLevel--;
							}

							if (braceLevel == 0)
							{
								characterPosition += i;
								break;
							}
						}
					}

					if (braceLevel > 0)
					{
						characterPosition += item.length();
						if (insideQuotes)
						{
							characterPosition += 2;
						}
					}
					insideQuotes = !insideQuotes;
				}

				secondInsertionPosition = characterPosition - 1;

				QTextCursor cursor = _unformattedTextEdit->textCursor();
				cursor.setPosition(secondInsertionPosition);
				cursor.deleteChar();

				cursor.setPosition(firstInsertionPosition);
				cursor.deleteChar();
			}
			// Otherwise we're compressing this section.
			else
			{
				int braceLevel = 1;

				// Now find the matching '}'...
				for (int i = lineIndex + 1; i < lines.count() && braceLevel > 0; i++)
				{
					characterPosition = _formattedText.indexOf('\n', characterPosition + 1);

					if (lines.at(i).endsWith("{"))
					{
						braceLevel++;
					}
					else if (lines.at(i).endsWith("}") || lines.at(i).endsWith("},"))
					{
						if (!lines.at(i).endsWith("{ " ELLIPSES " }"))
						{
							braceLevel--;
						}
					}
				}
				characterPosition = _formattedText.indexOf('}', characterPosition + 1);

				// Insert the second hidden char here.
				secondInsertionPosition = unformattedPosition(characterPosition + 1) - 1;

				QTextCursor cursor = _unformattedTextEdit->textCursor();
				previousCursorPosition = cursor.position();
				previousText = _formattedText;

				cursor.setPosition(secondInsertionPosition);
				_unformattedTextEdit->setTextCursor(cursor);
				_unformattedTextEdit->insertPlainText(QString(HIDDEN_CHAR));

				cursor.setPosition(firstInsertionPosition);
				_unformattedTextEdit->setTextCursor(cursor);
				_unformattedTextEdit->insertPlainText(QString(HIDDEN_CHAR));
			}
			setFormatted(true);

			textCursor().setPosition(cursorPositionHelper(previousCursorPosition, previousText, _formattedText));
			return true;
		}

	}
	return QPlainTextEdit::eventFilter(object, event);
}

void JsonEditor::updateText()
{
	if (!_formatDocument)
	{
		setText(toPlainText());
	}

	update();
	_marginWidget->update();
}

void JsonEditor::paintMarginWidget(QPaintEvent *)
{
	if (_formatDocument)
	{
		QPainter p(_marginWidget);

		p.setPen(Qt::black);

		QFontMetrics metrics(font());
		QStringList lines = _formattedText.split("\n");
		for (int i = 0; i < lines.count(); i++)
		{
			if (lines.at(i).contains("{"))
			{
				int yCoord = ((i + 1) * metrics.height()) - (verticalScrollBar()->value() * metrics.height()) - (metrics.height() / 2) + 1;
				p.drawLine(8, yCoord + 3, 12, yCoord + 3);
				p.drawEllipse(7, yCoord, 6, 6);

				if (lines.at(i).contains(ELLIPSES))
				{
					p.drawLine(10, yCoord, 10, yCoord + 6);
				}
			}
		}
	}
}

int JsonEditor::formattedPosition(int position)
{
	return cursorPositionHelper(position, text(), _formattedText);
}

int JsonEditor::unformattedPosition(int position)
{
	return cursorPositionHelper(position, _formattedText, text());
}

int JsonEditor::cursorPositionHelper(int position, QString before, QString after)
{
	int currentPosition = 0;

	// Some quick trivial checks...
	if (before.length() <= position)
	{
		return after.length();
	}
	if (position == 0)
	{
		return 0;
	}

	// Loop through both strings...
	while (currentPosition < position)
	{
		if (before.at(currentPosition) != after.at(currentPosition))
		{
			int startPosition = currentPosition;
			if (before.at(startPosition) == HIDDEN_CHAR)
			{
				int bracketLevel = 0;
				before.remove(startPosition, 1);
				position--;
				while (before.at(startPosition) != HIDDEN_CHAR && bracketLevel == 0)
				{
					if (before.at(startPosition) == '{')
					{
						bracketLevel++;
					}
					else if (before.at(startPosition) == '}')
					{
						bracketLevel--;
					}

					before.remove(startPosition, 1);
					if (position > currentPosition)
					{
						position--;
					}
				}
				before.remove(startPosition, 1);
				position--;
			}

			if (before.mid(startPosition, 3) == ELLIPSES)
			{
				before.remove(startPosition, 3);
				position -= 3;
			}

			if (after.at(currentPosition) == HIDDEN_CHAR)
			{
				int bracketLevel = 0;
				before.insert(currentPosition, after.at(currentPosition));
				position++;
				while (after.at(++currentPosition) != HIDDEN_CHAR && bracketLevel == 0)
				{
					if (after.at(currentPosition) == '{')
					{
						bracketLevel++;
					}
					else if (after.at(currentPosition) == '}')
					{
						bracketLevel--;
					}
					before.insert(currentPosition, after.at(currentPosition));
					position++;
				}
				before.insert(currentPosition, after.at(currentPosition));
				position++;
			}

			if (after.mid(currentPosition, 3) == ELLIPSES)
			{
				before.insert(currentPosition, ELLIPSES);
				position += 3;
			}

			if (before.at(currentPosition).isSpace())
			{
				before.remove(currentPosition, 1);
				position--;
				currentPosition--;
			}

			if (after.at(startPosition).isSpace())
			{
				before.insert(startPosition, after.at(startPosition));
				position++;
			}
		}
		currentPosition++;
	}
	return position;
}

#define INDENT (QString("\t").repeated(indent))
QString JsonEditor::formattedText(QString text)
{
	u_int64_t timeStart = QDateTime::currentMSecsSinceEpoch();

	int indent = 0;
	QString formatted;

	text = text.trimmed();
	bool insideQuotes = false;
	bool insideArray = false;
	int hidden = 0;

	// First split by quotation marks. (But not by quotation marks preceded by a backslash)
	foreach (QString const &item, text.split(QRegularExpression(R"((?<!\\)\")")))
	{
		if (insideQuotes)
		{
			if (!hidden)
			{
				if (formatted.endsWith("\n"))
				{
					formatted += INDENT;
				}
				formatted += "\"";
				formatted += item;
				formatted += "\"";
			}
		}
		else
		{
			QString cleaned = item.trimmed();

			while (!cleaned.isEmpty())
			{
				if (cleaned.startsWith(':'))
				{
					if (!hidden)
					{
						formatted += ": ";
					}
					cleaned.remove(0, 1);
					cleaned = cleaned.trimmed();
				}
				else if (cleaned.startsWith(","))
				{
					if (!hidden)
					{
						formatted += ",";
						if (insideArray)
						{
							formatted += " ";
						}
						else
						{
							formatted += "\n";
						}
					}
					cleaned.remove(0, 1);
					cleaned = cleaned.trimmed();
				}
				else if (cleaned.startsWith('{') || cleaned.startsWith('['))
				{
					if (!hidden)
					{
						if (formatted.endsWith("\n"))
						{
							formatted += INDENT;
						}

						formatted += cleaned.at(0);
						if (cleaned.startsWith('['))
						{
							insideArray = true;
							formatted += " ";
						}
						else
						{
							formatted += "\n";
						}
					}
					cleaned.remove(0, 1);
					if (cleaned.startsWith(HIDDEN_CHAR))
					{
						cleaned.remove(0, 1);

						if (!hidden)
						{
							// Remove the newline at the end and replace with an ellipses.
							formatted.chop(1);
							formatted += " " ELLIPSES " ";
						}
						hidden++;
					}
					else
					{
						cleaned = cleaned.trimmed();
						indent++;
					}
				}
				else if (cleaned.startsWith('}') || cleaned.startsWith(']'))
				{
					indent--;
					if (!hidden)
					{
						if (!insideArray)
						{
							formatted += "\n";
							formatted += INDENT;
						}
						else
						{
							formatted += " ";
						}
						formatted += cleaned.at(0);
					}
					if (cleaned.startsWith(']'))
					{
						insideArray = false;
					}
					cleaned.remove(0, 1);
					cleaned = cleaned.trimmed();
				}
				else
				{
					if (cleaned.startsWith(HIDDEN_CHAR))
					{
						hidden--;
						cleaned.remove(0, 1);
					}
					if (!hidden)
					{
						if (formatted.endsWith("\n"))
						{
							formatted += INDENT;
						}
						if (!cleaned.isEmpty())
						{
							formatted += cleaned.at(0);
						}
					}

					cleaned.remove(0, 1);
					cleaned = cleaned.trimmed();
				}
			}
		}
		insideQuotes = !insideQuotes;
	}

	// If we erroneously put an ending quotation mark, remove it.
	if (!text.endsWith("\"") && formatted.endsWith("\""))
	{
		formatted.chop(1);
	}

	qDebug("time to format text: %llu ms", QDateTime::currentMSecsSinceEpoch() - timeStart);
	return formatted;
}

int JsonEditor::positionOverLine(QPoint position)
{
	if (_marginWidget->rect().contains(position))
	{
		QFontMetrics metrics(font());
		return position.y() / metrics.height() + verticalScrollBar()->value();
	}
	else
	{
		return -1;
	}
}

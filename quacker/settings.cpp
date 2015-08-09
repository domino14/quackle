/*
 *  Quackle -- Crossword game artificial intelligence and analysis tool
 *  Copyright (C) 2005-2006 Jason Katz-Brown and John O'Laughlin.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

#include <iostream>
#include <sstream>

#include <QtGui>
#include <QMessageBox>

#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif // Q_WS_MAC

#include "alphabetparameters.h"
#include "board.h"
#include "boardparameters.h"
#include "computerplayercollection.h"
#include "datamanager.h"
#include "game.h"
#include "lexiconparameters.h"
#include "rack.h"
#include "strategyparameters.h"

#include "quackleio/flexiblealphabet.h"
#include "quackleio/util.h"

#include "settings.h"
#include "boardsetupdialog.h"
#include "customqsettings.h"
#include "graphicalboard.h"
#include "lexicondialog.h"

Settings *Settings::m_self = 0;
Settings *Settings::self()
{
	return m_self;
}

Settings::Settings(QWidget *parent)
	: QWidget(parent), m_lexiconNameCombo(0), m_alphabetNameCombo(0), m_themeNameCombo(0)
{
	m_self = this;
	QDir directory = QFileInfo(qApp->arguments().at(0)).absoluteDir();

 #ifdef Q_WS_MAC
	if (CFBundleGetMainBundle())
	{
		 CFURLRef dataUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("data"), NULL, NULL);
		 if (dataUrlRef)
		 {
		 	 CFStringRef macPath = CFURLCopyFileSystemPath(dataUrlRef, kCFURLPOSIXPathStyle);
			 size_t sizeOfBuf = CFStringGetMaximumSizeOfFileSystemRepresentation(macPath);
			 char* buf = (char*) malloc(sizeOfBuf);

			 CFStringGetFileSystemRepresentation(macPath, buf, sizeOfBuf);
		 	 directory = QDir(buf);
		 	 directory.cdUp();

			 free(buf);
			 CFRelease(dataUrlRef);
			 CFRelease(macPath);
		 }
	}
 #endif

	if (QFile::exists("data"))
		m_appDataDir = "data";
	else if (QFile::exists("../data"))
		m_appDataDir = "../data";
	else if (QFile::exists("Quackle.app/Contents/data"))
		m_appDataDir = "Quackle.app/Contents/data";
	else
	{
		if (!directory.cd("data") || !directory.cd("../data"))
			QMessageBox::critical(0, tr("Error Initializing Data Files - Quacker"), tr("<p>Could not open data directory. Quackle will be useless. Try running the quacker executable with quackle/quacker/ as the current directory.</p>"));
		m_appDataDir = directory.absolutePath();
	}
	m_userDataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
}

void Settings::createGUI()
{
	if (m_lexiconNameCombo != 0)
		return;

	QGridLayout *layout = new QGridLayout(this);

	m_lexiconNameCombo = new QComboBox;
	connect(m_lexiconNameCombo, SIGNAL(activated(const QString &)), this, SLOT(lexiconChanged(const QString &)));

	populateComboFromFilenames(m_lexiconNameCombo, "lexica", "lexicon");

	QLabel *lexiconNameLabel = new QLabel(tr("&Lexicon:"));
	lexiconNameLabel->setBuddy(m_lexiconNameCombo);
	m_editLexicon = new QPushButton(tr("Edit..."));
	m_editLexicon->setMaximumWidth(60);
	connect(m_editLexicon, SIGNAL(clicked()), this, SLOT(editLexicon()));

	m_alphabetNameCombo = new QComboBox;
	connect(m_alphabetNameCombo, SIGNAL(activated(const QString &)), this, SLOT(alphabetChanged(const QString &)));

	populateComboFromFilenames(m_alphabetNameCombo, "alphabets", "");

	QLabel *alphabetNameLabel = new QLabel(tr("&Alphabet:"));
	alphabetNameLabel->setBuddy(m_alphabetNameCombo);
	m_editAlphabet = new QPushButton(tr("Edit..."));
	m_editAlphabet->setMaximumWidth(60);
	connect(m_editAlphabet, SIGNAL(clicked()), this, SLOT(editAlphabet()));

	m_themeNameCombo = new QComboBox;
	connect(m_themeNameCombo, SIGNAL(activated(const QString &)), this, SLOT(themeChanged(const QString &)));

	populateComboFromFilenames(m_themeNameCombo, "themes", "");

	QLabel *themeNameLabel = new QLabel(tr("&Theme:"));
	themeNameLabel->setBuddy(m_themeNameCombo);
	m_editTheme = new QPushButton(tr("Edit..."));
	m_editTheme->setMaximumWidth(60);
	connect(m_editTheme, SIGNAL(clicked()), this, SLOT(editTheme()));

	m_boardNameCombo = new QComboBox;
	connect(m_boardNameCombo, SIGNAL(activated(const QString &)), this, SLOT(boardChanged(const QString &)));

	populateComboFromFilenames(m_boardNameCombo, "boards", "board");

	QLabel *boardNameLabel = new QLabel(tr("&Board:"));
	boardNameLabel->setBuddy(m_boardNameCombo);
	m_editBoard = new QPushButton(tr("Edit..."));
	m_editBoard->setMaximumWidth(60);
	connect(m_editBoard, SIGNAL(clicked()), this, SLOT(editBoard()));

	layout->addWidget(lexiconNameLabel, 0, 0, Qt::AlignRight);
	layout->addWidget(m_lexiconNameCombo, 0, 1);
	layout->addWidget(m_editLexicon, 0, 2);
	layout->addWidget(alphabetNameLabel, 1, 0, Qt::AlignRight);
	layout->addWidget(m_alphabetNameCombo, 1, 1);
	// layout->addWidget(m_editAlphabet, 1, 2);
	layout->addWidget(themeNameLabel, 2, 0, Qt::AlignRight);
	layout->addWidget(m_themeNameCombo, 2, 1);
	// layout->addWidget(m_editTheme, 2, 2);
	layout->addWidget(boardNameLabel, 3, 0, Qt::AlignRight);
	layout->addWidget(m_boardNameCombo, 3, 1);
	layout->addWidget(m_editBoard, 3, 2);

	layout->setColumnMinimumWidth(3, 0);
	layout->setColumnStretch(3, 1);
	layout->setRowMinimumHeight(4, 0);
	layout->setRowStretch(4, 1);

	load();
}

void Settings::load()
{
	m_lexiconNameCombo->setCurrentIndex(m_lexiconNameCombo->findText(QuackleIO::Util::stdStringToQString(QUACKLE_LEXICON_PARAMETERS->lexiconName())));
	m_alphabetNameCombo->setCurrentIndex(m_alphabetNameCombo->findText(QuackleIO::Util::stdStringToQString(QUACKLE_ALPHABET_PARAMETERS->alphabetName())));
	m_themeNameCombo->setCurrentIndex(m_themeNameCombo->findText(m_themeName));
	m_boardNameCombo->setCurrentIndex(m_boardNameCombo->findText(QuackleIO::Util::uvStringToQString(QUACKLE_BOARD_PARAMETERS->name())));
}

void Settings::preInitialize()
{
	// load computer players
	QUACKLE_DATAMANAGER->setComputerPlayers(Quackle::ComputerPlayerCollection::fullCollection());
}

void Settings::initialize()
{
	CustomQSettings settings;

	QUACKLE_DATAMANAGER->setBackupLexicon("twl06");
	QUACKLE_DATAMANAGER->setAppDataDirectory(m_appDataDir.toStdString());
	QUACKLE_DATAMANAGER->setUserDataDirectory(m_userDataDir.toStdString());

	QString lexiconName = settings.value("quackle/settings/lexicon-name", QString("twl06")).toString();

	// Handle Collins update.
	if (lexiconName == "cswfeb07")
		lexiconName = "cswapr07";

	setQuackleToUseLexiconName(QuackleIO::Util::qstringToStdString(lexiconName));
	setQuackleToUseAlphabetName(QuackleIO::Util::qstringToStdString(settings.value("quackle/settings/alphabet-name", QString("english")).toString()));
	setQuackleToUseThemeName(settings.value("quackle/settings/theme-name", QString("traditional")).toString());
	setQuackleToUseBoardName(settings.value("quackle/settings/board-name", QString("")).toString());
}

void Settings::setQuackleToUseLexiconName(const string &lexiconName)
{
	if (QUACKLE_LEXICON_PARAMETERS->lexiconName() != lexiconName)
	{
		QUACKLE_LEXICON_PARAMETERS->setLexiconName(lexiconName);

		string gaddagFile = Quackle::LexiconParameters::findDictionaryFile(lexiconName + ".gaddag");

		if (gaddagFile.empty())
		{
			UVcout << "Gaddag for lexicon '" << lexiconName << "' does not exist." << endl;
			QUACKLE_LEXICON_PARAMETERS->unloadGaddag();
		}
		else
			QUACKLE_LEXICON_PARAMETERS->loadGaddag(gaddagFile);

		string dawgFile = Quackle::LexiconParameters::findDictionaryFile(lexiconName + ".dawg");
		if (dawgFile.empty())
		{
			UVcout << "Dawg for lexicon '" << lexiconName << "' does not exist." << endl;
			QUACKLE_LEXICON_PARAMETERS->unloadDawg();
		}
		else
			QUACKLE_LEXICON_PARAMETERS->loadDawg(dawgFile);

		QUACKLE_STRATEGY_PARAMETERS->initialize(lexiconName);
	}
}

void Settings::setQuackleToUseAlphabetName(const string &alphabetName)
{
	if (QUACKLE_ALPHABET_PARAMETERS->alphabetName() != alphabetName)
	{
		QString alphabetFile = QuackleIO::Util::stdStringToQString(Quackle::AlphabetParameters::findAlphabetFile(alphabetName + ".quackle_alphabet"));

		QuackleIO::FlexibleAlphabetParameters *flexure = new QuackleIO::FlexibleAlphabetParameters;
		flexure->setAlphabetName(alphabetName);
		if (flexure->load(alphabetFile))
		{
			if (flexure->length() != QUACKLE_ALPHABET_PARAMETERS->length() && QUACKLE_ALPHABET_PARAMETERS->alphabetName() != "default")
			{
				QMessageBox::warning(this, tr("Alphabet mismatch - Quackle"), QString("<html>%1</html>").arg(tr("%1 has a different number of letters than %2, so please start a new game or else Quackle will crash or act strangely.").arg(QuackleIO::Util::stdStringToQString(flexure->alphabetName())).arg(QuackleIO::Util::stdStringToQString(QUACKLE_ALPHABET_PARAMETERS->alphabetName()))));
			}

			QUACKLE_DATAMANAGER->setAlphabetParameters(flexure);
		}
		else
		{
			UVcerr << "Couldn't load alphabet!" << endl;
			delete flexure;
		}
	}
}

void Settings::setQuackleToUseThemeName(const QString &themeName)
{
	m_themeName = themeName;
	QString themeFile = m_userDataDir + "/themes/" + themeName + ".ini";
	if (!QFile::exists(themeFile))
		themeFile = m_appDataDir + "/themes/" + themeName + ".ini";
	if (!QFile::exists(themeFile))
	{
		m_themeName = "traditional";
		themeFile = m_appDataDir + "/themes/traditional.ini";
	}
	PixmapCacher::self()->readTheme(themeFile);
}

void Settings::setQuackleToUseBoardName(const QString &boardName)
{
	CustomQSettings settings;
	settings.beginGroup("quackle/boardparameters");

	if (boardName.isEmpty() || !settings.contains(boardName))
		QUACKLE_DATAMANAGER->setBoardParameters(new Quackle::BoardParameters());
	else
	{
		QByteArray boardParameterBytes = qUncompress(settings.value(boardName).toByteArray());
		string boardParameterBuf;
		boardParameterBuf.assign((const char *) boardParameterBytes, boardParameterBytes.size());
		istringstream boardParameterStream(boardParameterBuf);

		QUACKLE_DATAMANAGER->setBoardParameters(Quackle::BoardParameters::Deserialize(boardParameterStream));
	}
	QUACKLE_BOARD_PARAMETERS->setName(QuackleIO::Util::qstringToString(boardName));
	loadBoardNameCombo();
}

void Settings::lexiconChanged(const QString &lexiconName)
{
	if (m_lexiconNameCombo->currentIndex() == m_lexiconNameCombo->count() - 1)
	{
		editLexicon();
		return;
	}
	string lexiconNameString = QuackleIO::Util::qstringToStdString(lexiconName);
	setQuackleToUseLexiconName(lexiconNameString);

	CustomQSettings settings;
	settings.setValue("quackle/settings/lexicon-name", lexiconName);

	emit refreshViews();
}

void Settings::alphabetChanged(const QString &alphabetName)
{
	if (m_alphabetNameCombo->currentIndex() == m_alphabetNameCombo->count() - 1)
	{
		editAlphabet();
		return;
	}
	string alphabetNameString = QuackleIO::Util::qstringToStdString(alphabetName);
	setQuackleToUseAlphabetName(alphabetNameString);

	CustomQSettings settings;
	settings.setValue("quackle/settings/alphabet-name", alphabetName);

	emit refreshViews();
}

void Settings::themeChanged(const QString &themeName)
{
	if (m_themeNameCombo->currentIndex() == m_themeNameCombo->count() - 1)
	{
		editTheme();
		return;
	}
	setQuackleToUseThemeName(themeName);

	CustomQSettings settings;
	settings.setValue("quackle/settings/theme-name", themeName);

	emit refreshViews();
}

void Settings::boardChanged(const QString &boardName)
{
	if (m_boardNameCombo->currentIndex() == m_boardNameCombo->count() - 1)
	{
		addBoard();
		return;
	}
	CustomQSettings settings;
	settings.setValue("quackle/settings/board-name", boardName);

	setQuackleToUseBoardName(boardName);
	emit refreshViews();
}

void Settings::addBoard()
{
	QUACKLE_DATAMANAGER->setBoardParameters(new Quackle::BoardParameters());
	QUACKLE_BOARD_PARAMETERS->setName(MARK_UV(""));

	CustomQSettings settings;
	BoardSetupDialog dialog(this);
	if (dialog.exec())
	{
		QString boardName = QuackleIO::Util::uvStringToQString(QUACKLE_BOARD_PARAMETERS->name());
		settings.beginGroup("quackle/boardparameters");

		ostringstream boardParameterStream;
		QUACKLE_BOARD_PARAMETERS->Serialize(boardParameterStream);

		QByteArray boardParameterBytes = qCompress(
							(const uchar *)boardParameterStream.str().data(),
							boardParameterStream.str().size());
		settings.setValue(boardName, QVariant(boardParameterBytes));
		m_boardNameCombo->setCurrentIndex(-1);
		boardChanged(boardName);
	}
	else
		setQuackleToUseBoardName(settings.value("quackle/settings/board-name", QString("")).toString());

	PixmapCacher::self()->invalidate();
}

void Settings::editBoard()
{
	QString oldBoardName = m_boardNameCombo->currentText();
	QUACKLE_BOARD_PARAMETERS->setName(QuackleIO::Util::qstringToString(oldBoardName));

	BoardSetupDialog dialog(this);
	if (dialog.exec())
	{
		QString newBoardName = QuackleIO::Util::uvStringToQString(QUACKLE_BOARD_PARAMETERS->name());
		CustomQSettings settings;
		settings.beginGroup("quackle/boardparameters");

		if (newBoardName != oldBoardName)
			settings.remove(oldBoardName);

		ostringstream boardParameterStream;
		QUACKLE_BOARD_PARAMETERS->Serialize(boardParameterStream);

		QByteArray boardParameterBytes = qCompress(
							(const char *)boardParameterStream.str().data(),
							boardParameterStream.str().size());
		settings.setValue(newBoardName, QVariant(boardParameterBytes));
		boardChanged(newBoardName);
	}
	PixmapCacher::self()->invalidate();
	loadBoardNameCombo();
	emit refreshViews();
}

void Settings::loadBoardNameCombo()
{
	if (m_lexiconNameCombo == 0)
		return;

	while (m_boardNameCombo->count() > 0)
		m_boardNameCombo->removeItem(0);

	CustomQSettings settings;
	settings.beginGroup("quackle/boardparameters");
	QStringList boardNames = settings.childKeys();
	boardNames.sort();
	m_boardNameCombo->addItems(boardNames);
	m_boardNameCombo->addItem("Add new board...");
	settings.endGroup();

	QString currentItem = settings.value("quackle/settings/board-name", QString("")).toString();
	int currentItemIndex = m_boardNameCombo->findText(currentItem);
	if (m_boardNameCombo->count() > 0 && currentItemIndex < 0)
		currentItemIndex = 0;
	m_boardNameCombo->setCurrentIndex(currentItemIndex);
}

void Settings::editLexicon()
{
	QString name = m_lexiconNameCombo->currentText();
	if (m_lexiconNameCombo->currentIndex() == m_lexiconNameCombo->count() - 1)
		name = "";
	LexiconDialog dialog(this, name);
	if (dialog.exec())
	{
		populateComboFromFilenames(m_lexiconNameCombo, "lexica", "lexicon");
		load();
	}
}

void Settings::editAlphabet()
{
#if 0
	QString name = m_alphabetNameCombo->currentText();
	if (m_alphabetNameCombo->currentIndex() == m_alphabetNameCombo->count() - 1)
		name = "";
	AlphabetDialog dialog(this);
	if (dialog.exec())
	{
		populateComboFromFilenames(m_alphabetNameCombo, "alphabets", "alphabet");
		load();
	}
#endif // 0
}

void Settings::editTheme()
{
#if 0
	QString name = m_themeNameCombo->currentText();
	if (m_themeNameCombo->currentIndex() == m_themeNameCombo->count() - 1)
		name = "";
	ThemeDialog dialog(this);
	if (dialog.exec())
	{
		populateThemeFromFilenames(m_themeNameCombo, "themes", "theme");
		load();
	}
#endif // 0
}

void Settings::populateComboFromFilenames(QComboBox* combo, const QString &path, const QString &label)
{
	QStringList fileList;
	QDir dir(m_appDataDir);
	if (dir.cd(path))
		fileList << dir.entryList(QDir::Files | QDir::Readable, QDir::Name);
	dir = QDir(m_userDataDir);
	if (dir.cd(path))
		fileList << dir.entryList(QDir::Files | QDir::Readable, QDir::Name);

	QStringListIterator i(fileList);
	QString fileName;
	QStringList list;
	int periodPos;

	while (i.hasNext())
	{
		fileName = i.next();
		periodPos = fileName.indexOf('.');
		if (periodPos)
		{
			fileName.truncate(periodPos);
			list << fileName;
		}
	}
	list.removeDuplicates();

	combo->addItems(list);
	if (label.size() > 0)
		combo->addItem(QString(tr("Add new ")).append(path).append("..."));
}

/***************************************************************************
 *   Copyright (C) 2007 by Rafał Rzepecki   *
 *   divided.mind@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <QtGui>
#include "stardust.h"
#include "triview.h"
#include "ui_stardustconfig.h"
#include "stardustconnector.h"

#include <QTextEdit>
#include <QTextStream>
#include <QCloseEvent>
#include <QFileDialog>
#include <QImage>
#include <QStringList>
#include <QInputDialog>
#include <QUrl>
#include <QHttp>
#include <QBuffer>
#include <QProgressDialog>

#include <iostream>

stardust::stardust() : dlingImage(0)
{
  triView = new TriView;
  setCentralWidget( triView );
  
  info = new QLabel(this);
  triView->insertExtraWidget(info);
  
  connect(triView, SIGNAL(moved()), SLOT(updateStatusBar()));

  connector = new StardustConnector();
  connect(connector, SIGNAL(connecting()), SLOT(connecting()));
  connect(connector, SIGNAL(error()), SLOT(error()));
  connect(connector, SIGNAL(connected()), SLOT(connected()));
  connect(connector, SIGNAL(newImage(const QUrl &, int)), 
          SLOT(loadUrl(const QUrl &, int)));
  connect(connector, SIGNAL(nextImage(const QUrl &, int)), 
          SLOT(gotNextUrl(const QUrl &, int)));
  connect(connector, SIGNAL(movieId(const QString &)),
          SLOT(setTitle(const QString &)));
  connect(connector, SIGNAL(userInfo(const QString &)),
          info, SLOT(setText(const QString &)));
  
  createActions();
  createMenus();
  createToolBars();
  createStatusBar();

  progress = new QProgressDialog(tr("Downloading images..."), tr("Cancel"), 0, 0);
  connect(progress, SIGNAL(canceled()), this, SLOT(cancelDownload()));
  progress->setWindowModality(Qt::WindowModal);
  progress->hide();
  
  http = new QHttp;
  connect(http, SIGNAL(requestFinished(int, bool)), 
          this, SLOT(requestFinished(int, bool)));
  connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
          this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
  
  dlbuffer = new QBuffer;

  setAcceptDrops(true);
  
  configDialog = new QDialog;
  configDialog_ui = new Ui::StardustConfig;
  configDialog_ui->setupUi(configDialog);
  connect(configDialog, SIGNAL(accepted()), this, SLOT(preferencesAccepted()));
  
  readSettings();
  
  if (autoConnect)
    connectToServer();

//   connect( textEdit->document(), SIGNAL( contentsChanged() ),
//            this, SLOT( documentWasModified() ) );

  setCurrentFile( "" );
}

void stardust::closeEvent( QCloseEvent *event )
{
  if ( maybeSave() ) {
    writeSettings();
    event->accept();
  } else {
    event->ignore();
  }
}

void stardust::newFile()
{
  if ( maybeSave() ) {
//     textEdit->clear();
    setCurrentFile( "" );
  }
}

void stardust::open()
{
  if ( maybeSave() ) {
    QStringList fileNames = QFileDialog::getOpenFileNames( this );

    if ( !fileNames.isEmpty() )
      loadFiles( fileNames );
  }
}

void stardust::openUrl()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Enter URL"), 
                                      tr("Enter the address of <b>an image</b>:"),
                                      QLineEdit::Normal, QString(), &ok);
  
  if (ok && !text.isEmpty()) {
    QUrl url(text);
    if (url.isValid())
      loadUrl(url);
  }
}

void stardust::openFromKonqueror()
{
  QProcess dcop;
  
  // find konqueror instances
  dcop.start("dcop");
  if (!dcop.waitForFinished())
    return;
  
  QStringList konquerors;
  QByteArrayMatcher matcher("konqueror");
  for (QByteArray line = dcop.readLine(); !line.isEmpty(); line = dcop.readLine())
    if (matcher.indexIn(line) == 0)
      konquerors.append(line.trimmed());
  
  if (konquerors.isEmpty()) {
    QMessageBox::warning(this, tr("No Konquerors"), tr("No running instances of Konqueror found."));
    return;
  }
  
  for (QStringList::const_iterator it = konquerors.constBegin();
       it != konquerors.constEnd(); ++it) {
    dcop.start("dcop", QStringList() << *it);
    if (!dcop.waitForFinished())
      return;
    
    QStringList htmlwidgets;
    matcher.setPattern("html-widget");
    for (QByteArray line = dcop.readLine(); !line.isEmpty(); line = dcop.readLine())
      if (matcher.indexIn(line) == 0 && line.indexOf("-view") == -1)
        htmlwidgets.append(line.trimmed());
    
    for (QStringList::const_iterator w = htmlwidgets.constBegin();
         w != htmlwidgets.constEnd(); ++w) {
      
      QByteArray wasJsEnabled;
      dcop.start("dcop", QStringList() << *it << *w << "jScriptEnabled");
      if (!dcop.waitForFinished())
        return;
      wasJsEnabled = dcop.readLine().trimmed();
      
      dcop.start("dcop", QStringList() << *it << *w << "setJScriptEnabled" << "true");
      if (!dcop.waitForFinished())
        return;
      
      dcop.start("dcop", QStringList() << *it << *w << "evalJS" << "document.movieframe.src");
      if (!dcop.waitForFinished())
        return;
      QByteArray src = dcop.readLine().trimmed();
      
      dcop.start("dcop", QStringList() << *it << *w << "setJScriptEnabled" << wasJsEnabled);
      if (!dcop.waitForFinished())
        return;
      
      if (src.left(7) == "http://") {
        loadUrl(QUrl(src));
        dcop.start("dcop", QStringList() << *it << *w << "saveDocument" << "file:///tmp/stardust.html");
        if (dcop.waitForFinished())
          findNextUrl("/tmp/stardust.html");
        return;
      }
    }
  }
}

void stardust::findNextUrl(const QString &filename)
{
  QFile file(filename);
  file.open(QIODevice::ReadOnly);
  QByteArray contents = file.readAll();
  file.close();
  static QRegExp rx("full_filename\\(\"(http://\\S+001.jpg)\", j\\)");
  int pos = rx.indexIn(contents);
  if (pos > -1) {
    QString nU = rx.cap(1);
    nextUrl = QUrl(nU);
  }
  if (!dlingImage)
    downloadImages(nextUrl);
}

bool stardust::save()
{
  if ( curFile.isEmpty() ) {
    return saveAs();
  } else {
    return saveFile( curFile );
  }
}

bool stardust::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName( this );

  if ( fileName.isEmpty() )
    return false;

  return saveFile( fileName );
}

void stardust::about()
{
  QMessageBox::about( this, tr( "About Application" ),
                      tr( "The <b>Application</b> example demonstrates how to "
                          "write modern GUI applications using Qt, with a menu bar, "
                          "toolbars, and a status bar." ) );
}

void stardust::preferences()
{
  setupPreferences();
  configDialog->exec();
}

void stardust::setupPreferences()
{
  configDialog_ui->m_userName->setText( username );
  configDialog_ui->m_password->setText( password );
  configDialog_ui->m_savePassword->setChecked( savePassword );
  configDialog_ui->m_automaticallyConnect->setChecked( autoConnect );
  configDialog_ui->m_requestConfirmation->setChecked( requestConfirmation );
}

void stardust::preferencesAccepted()
{
  username = configDialog_ui->m_userName->text();
  password = configDialog_ui->m_password->text();
  savePassword = configDialog_ui->m_savePassword->isChecked();
  autoConnect = configDialog_ui->m_automaticallyConnect->isChecked();
  requestConfirmation = configDialog_ui->m_requestConfirmation->isChecked();
  writeSettings();
}

void stardust::connectToServer()
{
  if (password.isEmpty()) {
    QMessageBox::information(this, tr("Missing information"), tr("To connect you first need to fill in the username and the password."));
    setupPreferences();
    if (!configDialog->exec())
      return;
  }
  connector->login(username, password);
}

void stardust::connecting()
{
  statusBar()->showMessage(tr("Connecting to server..."));
}

void stardust::connected()
{
  statusBar()->showMessage(tr("Connected to server."));
  connectAct->setDisabled(true);
  
  respondTrackAct->setEnabled(true);
  respondNoTrackAct->setEnabled(true);
  respondNoFocusAct->setEnabled(true);
}

void stardust::error()
{
  statusBar()->showMessage(tr("Communication error."));
}

void stardust::documentWasModified()
{
  setWindowModified( true );
}

void stardust::createActions()
{
  newAct = new QAction( QIcon( ":/filenew.xpm" ), tr( "&New" ), this );
  newAct->setShortcut( tr( "Ctrl+N" ) );
  newAct->setStatusTip( tr( "Create a new file" ) );
  connect( newAct, SIGNAL( triggered() ), this, SLOT( newFile() ) );

  openAct = new QAction( QIcon( ":/fileopen.xpm" ), tr( "&Open..." ), this );
  openAct->setShortcut( tr( "Ctrl+O" ) );
  openAct->setStatusTip( tr( "Open an existing file" ) );
  connect( openAct, SIGNAL( triggered() ), this, SLOT( open() ) );

  openUrlAct = new QAction( QIcon( ":/fileopenurl.png" ), tr( "Open &URL..." ), this );
  openUrlAct->setShortcut( tr( "Ctrl+U" ) );
  openUrlAct->setStatusTip( tr( "Open a remote set of images" ) );
  connect( openUrlAct, SIGNAL( triggered() ), this, SLOT( openUrl() ) );
  
  openKonqAct = new QAction( QIcon( ":/konqueror.png" ), tr( "Open from &Konqueror" ), this );
  openKonqAct->setShortcut( tr( "Ctrl+K" ) );
  openKonqAct->setStatusTip( tr( "Open a set of images shown in Konqueror" ) );
  connect( openKonqAct, SIGNAL( triggered() ), this, SLOT( openFromKonqueror() ) );
  
  connectAct = new QAction( QIcon( ":/connect_no.png" ), tr( "&Connect to server" ), this );
  connectAct->setStatusTip( tr( "Connect to the server and fetch movies" ) );
  connect( connectAct, SIGNAL( triggered() ), this, SLOT( connectToServer() ) );
  
  saveAct = new QAction( QIcon( ":/filesave.xpm" ), tr( "&Save" ), this );
  saveAct->setShortcut( tr( "Ctrl+S" ) );
  saveAct->setStatusTip( tr( "Save the document to disk" ) );
  connect( saveAct, SIGNAL( triggered() ), this, SLOT( save() ) );

  saveAsAct = new QAction( tr( "Save &As..." ), this );
  saveAsAct->setStatusTip( tr( "Save the document under a new name" ) );
  connect( saveAsAct, SIGNAL( triggered() ), this, SLOT( saveAs() ) );

  exitAct = new QAction( tr( "E&xit" ), this );
  exitAct->setShortcut( tr( "Ctrl+Q" ) );
  exitAct->setStatusTip( tr( "Exit the application" ) );
  connect( exitAct, SIGNAL( triggered() ), this, SLOT( close() ) );

  cutAct = new QAction( QIcon( ":/editcut.xpm" ), tr( "Cu&t" ), this );
  cutAct->setShortcut( tr( "Ctrl+X" ) );
  cutAct->setStatusTip( tr( "Cut the current selection's contents to the "
                            "clipboard" ) );
//   connect( cutAct, SIGNAL( triggered() ), textEdit, SLOT( cut() ) );

  copyAct = new QAction( QIcon( ":/editcopy.xpm" ), tr( "&Copy" ), this );
  copyAct->setShortcut( tr( "Ctrl+C" ) );
  copyAct->setStatusTip( tr( "Copy the current selection's contents to the "
                             "clipboard" ) );
//   connect( copyAct, SIGNAL( triggered() ), textEdit, SLOT( copy() ) );
  
  preferencesAct = new QAction( QIcon( ":/configure.png" ), tr( "P&references..." ), this );
  preferencesAct->setStatusTip( tr( "Set preferences, such as the username and the password" ) );
  connect( preferencesAct, SIGNAL( triggered() ), this, SLOT( preferences() ) );

  pasteAct = new QAction( QIcon( ":/editpaste.xpm" ), tr( "&Paste" ), this );
  pasteAct->setShortcut( tr( "Ctrl+V" ) );
  pasteAct->setStatusTip( tr( "Paste the clipboard's contents into the current "
                              "selection" ) );
//   connect( pasteAct, SIGNAL( triggered() ), textEdit, SLOT( paste() ) );

  aboutAct = new QAction( tr( "&About" ), this );
  aboutAct->setStatusTip( tr( "Show the application's About box" ) );
  connect( aboutAct, SIGNAL( triggered() ), this, SLOT( about() ) );

  aboutQtAct = new QAction( tr( "About &Qt" ), this );
  aboutQtAct->setStatusTip( tr( "Show the Qt library's About box" ) );
  connect( aboutQtAct, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

  cutAct->setEnabled( false );
  copyAct->setEnabled( false );
/*  connect( textEdit, SIGNAL( copyAvailable( bool ) ),
           cutAct, SLOT( setEnabled( bool ) ) );
  connect( textEdit, SIGNAL( copyAvailable( bool ) ),
           copyAct, SLOT( setEnabled( bool ) ) );*/
  
  respondNoFocusAct = new QAction( tr( "&Bad focus" ), this);
  respondNoFocusAct->setShortcut(Qt::Key_Backspace);
  respondNoFocusAct->setStatusTip( tr( "Report bad focus" ));
  respondNoFocusAct->setDisabled(true);
  connect( respondNoFocusAct, SIGNAL( triggered() ),
           this, SLOT(respondNoFocus()) );
  
  respondNoTrackAct = new QAction( tr( "No track" ), this);
  respondNoTrackAct->setShortcut(Qt::Key_Space);
  respondNoTrackAct->setStatusTip( tr( "Report no track" ));
  respondNoTrackAct->setDisabled(true);
  connect( respondNoTrackAct, SIGNAL( triggered() ),
           this, SLOT(respondNoTrack()) );
  
  respondTrackAct = new QAction( tr( "Track" ), this);
  respondTrackAct->setShortcut(Qt::Key_Enter);
  respondTrackAct->setStatusTip( tr( "Report a track" ));
  respondTrackAct->setDisabled(true);
  connect( respondTrackAct, SIGNAL( triggered() ),
           this, SLOT(respondTrack()) );
}

void stardust::createMenus()
{
  fileMenu = menuBar()->addMenu( tr( "&File" ) );
  fileMenu->addAction( newAct );
  fileMenu->addAction( openAct );
  fileMenu->addAction( openUrlAct );
  fileMenu->addAction( openKonqAct );
  fileMenu->addAction( saveAct );
  fileMenu->addAction( saveAsAct );
  fileMenu->addSeparator();
  fileMenu->addAction( connectAct );
  fileMenu->addSeparator();
  fileMenu->addAction( exitAct );

  editMenu = menuBar()->addMenu( tr( "&Edit" ) );
  editMenu->addAction( cutAct );
  editMenu->addAction( copyAct );
  editMenu->addAction( pasteAct );
  editMenu->addSeparator();
  editMenu->addAction( preferencesAct );
  
  reportMenu = menuBar()->addMenu( tr( "&Report" ) );
  reportMenu->addAction( respondTrackAct );
  reportMenu->addAction( respondNoTrackAct );
  reportMenu->addAction( respondNoFocusAct );

  menuBar()->addSeparator();

  helpMenu = menuBar()->addMenu( tr( "&Help" ) );
  helpMenu->addAction( aboutAct );
  helpMenu->addAction( aboutQtAct );
}

void stardust::createToolBars()
{
  fileToolBar = addToolBar( tr( "File" ) );
  fileToolBar->addAction( newAct );
  fileToolBar->addAction( openAct );
  fileToolBar->addAction( openUrlAct );
  fileToolBar->addAction( openKonqAct );
  fileToolBar->addAction( saveAct );

  editToolBar = addToolBar( tr( "Edit" ) );
  editToolBar->addAction( cutAct );
  editToolBar->addAction( copyAct );
  editToolBar->addAction( pasteAct );
  
  reportToolBar = addToolBar( tr( "Report" ) );
  reportToolBar->addAction( respondTrackAct );
  reportToolBar->addAction( respondNoTrackAct );
  reportToolBar->addAction( respondNoFocusAct );
}

void stardust::createStatusBar()
{
  statusBar()->showMessage( tr( "Ready" ) );
  progressBar = new QProgressBar(statusBar());
  statusBar()->addPermanentWidget(progressBar);
}

void stardust::readSettings()
{
  QSettings settings( "Divided Mind", "Stardust Finder" );
  QPoint pos = settings.value( "pos", QPoint( 200, 200 ) ).toPoint();
  QSize size = settings.value( "size", QSize( 400, 400 ) ).toSize();
  resize( size );
  move( pos );
  
  username = settings.value( "username", QString() ).toString();
  password = settings.value( "password", QString() ).toString();
  savePassword = !password.isEmpty();
  autoConnect = settings.value( "autoConnect", false ).toBool();
  requestConfirmation = settings.value( "requestConfirmation", true ).toBool();
}

void stardust::writeSettings()
{
  QSettings settings( "Divided Mind", "Stardust Finder" );
  settings.setValue( "pos", pos() );
  settings.setValue( "size", size() );
  settings.setValue( "username", username );
  if (savePassword)
    settings.setValue( "password", password );
  else
    settings.remove( "password" );
  settings.setValue( "autoConnect", autoConnect );
  settings.setValue( "requestConfirmation", requestConfirmation );
}

bool stardust::maybeSave()
{
//   if ( textEdit->document()->isModified() ) {
//     int ret = QMessageBox::warning( this, tr( "Application" ),
//                                     tr( "The document has been modified.\n"
//                                         "Do you want to save your changes?" ),
//                                     QMessageBox::Yes | QMessageBox::Default,
//                                     QMessageBox::No,
//                                     QMessageBox::Cancel | QMessageBox::Escape );
// 
//     if ( ret == QMessageBox::Yes )
//       return save();
//     else if ( ret == QMessageBox::Cancel )
//       return false;
//   }

  return true;
}

void stardust::loadFiles( const QStringList &fileNames )
{
  QApplication::setOverrideCursor( Qt::WaitCursor );
  QVector <QImage> images;
  images.reserve(fileNames.size());
  for (QStringList::const_iterator it = fileNames.constBegin();
       it != fileNames.constEnd(); ++it)
    images.append(QImage(*it));
  triView->setImages(images);
  QApplication::restoreOverrideCursor();

  setCurrentFile( fileNames[0] );
  statusBar()->showMessage( tr( "File loaded" ), 2000 );
}

void stardust::loadUrl( const QUrl &url, int frames )
{
  if (url == nextUrl) {
    currentUrl = url;
    curFrames = frames;
    nextUrl = QUrl();
    if (dlingImage == 0)
      loadDledImages();
    else {
      backgroundLoading = false;
      hideProgress();
      showProgress(frames);
    }
    return;
  } else
    nextUrl = QUrl();
  
  if (dlingImage)
    return;
  
  downloadImages(url, frames);
}

void stardust::gotNextUrl( const QUrl &url, int frames )
{
  nextUrl = url;
  nextFrames = frames;
  if (!dlingImage && dledImages.isEmpty())
    downloadImages(nextUrl, nextFrames);
}

void stardust::setTitle(const QString &title)
{
  setWindowTitle( tr( "%1 - %2" ).arg( title ).arg( tr( "Stardust Finder" ) ) );
}

void stardust::downloadImages(const QUrl &url, int frames)
{
  refPath = url.path();
  refPath = refPath.left(refPath.length() - 7) + "%1.jpg";
  
  http->setHost(url.host());
  
  dledImages.clear();
  dlingImage = 0;
  
  backgroundLoading = !nextUrl.isEmpty();
  
  showProgress(frames);
  
  downloadNextImage();
}

void stardust::cancelDownload()
{
  http->abort();
  hideProgress();
}

void stardust::downloadNextImage()
{
  dlbuffer->setBuffer(0);
  dlbuffer->open(QIODevice::WriteOnly);
  QString path = refPath.arg(QString::number(++dlingImage), 3, '0');
  setProgress(path, dlingImage);
  httpGetId = http->get(path, dlbuffer);
}

void stardust::requestFinished(int requestId, bool error)
{
  if (requestId != httpGetId)
    return;
  if (error) {
    QMessageBox::critical(this, tr("Download failed"), tr("Error downloading files."));
    cancelDownload();
  }
  if (dlingImage != 0) {
    processImage();
    downloadNextImage();
  } else {
    finishedDling();
  }
}

void stardust::loadDledImages()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  triView->setImages(dledImages);
  dledImages.clear();
  QApplication::restoreOverrideCursor();
  if (!nextUrl.isEmpty())
    downloadImages(nextUrl, nextFrames);
}

void stardust::finishedDling()
{
  hideProgress();
  dlbuffer->close();
  if (!backgroundLoading)
    loadDledImages();
}

void stardust::hideProgress()
{
  progress->hide();
  statusBar()->showMessage(tr("Ready."));
  progressBar->hide();
  QApplication::restoreOverrideCursor();
}

void stardust::showProgress(int max)
{
  if (backgroundLoading) {
    progressBar->setMaximum(max);
    progressBar->setValue(0);
    progressBar->show();
  } else {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    progress->setMaximum(max);
    progress->setValue(0);
    progress->show();
  }
  setProgress(tr("Downloading images..."));
}

void stardust::setProgress(const QString &message, int image)
{
  if (backgroundLoading) {
    statusBar()->showMessage(message);
    progressBar->setValue(image);
  } else {
    progress->setLabelText(message);
    progress->setValue(image);
  }
}

void stardust::processImage()
{
  dlbuffer->close();
  QImage image;
  dlbuffer->open(QIODevice::ReadOnly);
  image.load(dlbuffer, 0);
  dledImages.append(image);
  dlbuffer->close();
}

void stardust::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain")) {
      QString text = event->mimeData()->text();
      if (text.left(7) == "http://")
          event->acceptProposedAction();
    }
}

void stardust::dropEvent(QDropEvent *event)
{
  loadUrl(QUrl(event->mimeData()->text()));
  event->acceptProposedAction();
}

void stardust::readResponseHeader(const QHttpResponseHeader &header)
{
  int statusCode = header.statusCode();
  if (statusCode != 200)
    dlingImage = 0;
}

bool stardust::saveFile( const QString &fileName )

{
  QFile file( fileName );

  if ( !file.open( QFile::WriteOnly | QFile::Text ) ) {
    QMessageBox::warning( this, tr( "Application" ),
                          tr( "Cannot write file %1:\n%2." )
                          .arg( fileName )
                          .arg( file.errorString() ) );
    return false;
  }

  QTextStream out( &file );

  QApplication::setOverrideCursor( Qt::WaitCursor );
  out << textEdit->toPlainText();
  QApplication::restoreOverrideCursor();

  setCurrentFile( fileName );
  statusBar()->showMessage( tr( "File saved" ), 2000 );
  return true;
}

void stardust::setCurrentFile( const QString &fileName )

{
  curFile = fileName;
//   textEdit->document()->setModified( false );
  setWindowModified( false );

  QString shownName;

  if ( curFile.isEmpty() )
    shownName = "untitled.txt";
  else
    shownName = strippedName( curFile );

  setWindowTitle( tr( "%1[*] - %2" ).arg( shownName ).arg( tr( "Application" ) ) );
}

QString stardust::strippedName( const QString &fullFileName )

{
  return QFileInfo( fullFileName ).fileName();
}

stardust::~stardust()

{
}

void stardust::updateStatusBar()
{
  statusBar()->showMessage(tr("(%1, %2, %3)")
      .arg(triView->getX()).arg(triView->getY()).arg(triView->getZ()));
}

void stardust::respondNoFocus()
{
  if (requestConfirmation)
    if (QMessageBox::question(this, tr("Confirm report"),
        tr("Do you want to report bad focus?"),
           QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
      return;

  connector->respondNoFocus();
}

void stardust::respondNoTrack()
{
  if (requestConfirmation)
    if (QMessageBox::question(this, tr("Confirm report"),
        tr("Do you want to report no track on this imageset?"),
           QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
      return;

  connector->respondNoTrack();
}

void stardust::respondTrack()
{
  if (requestConfirmation)
    if (QMessageBox::question(this, tr("Confirm report"),
        tr("Do you want to report a track in this location?"),
           QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
      return;
  connector->respondTrack(triView->getX(), triView->getY(), triView->getZ());
}

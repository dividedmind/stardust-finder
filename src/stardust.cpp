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
#include "aboutbox.h"

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

stardust::stardust() : dlingImage(0), nextFrames(0)
{
  createView();
  createActions();
  createMenus();
  createToolBars();
  createStatusBar();
  createConnector();
  createProgress();
  createHttp();
  createDialogs();

  setAcceptDrops(true);
  setTitle(tr("No image"));
  
  readSettings();
  
  cleanUp();
  
  if (autoConnect)
    connectToServer();
//   connect( textEdit->document(), SIGNAL( contentsChanged() ),
//            this, SLOT( documentWasModified() ) );

}

void stardust::createDialogs()
{
  configDialog = new QDialog;
  configDialog_ui = new Ui::StardustConfig;
  configDialog_ui->setupUi(configDialog);
  connect(configDialog, SIGNAL(accepted()), this, SLOT(preferencesAccepted()));
  
  aboutBox = new AboutBox("0.1", this);
}

void stardust::createHttp()
{
  http = new QHttp;
  connect(http, SIGNAL(requestFinished(int, bool)), 
          this, SLOT(requestFinished(int, bool)));
  connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
          this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
  
  dlbuffer = new QBuffer;
}

void stardust::createProgress()
{

  progress = new QProgressDialog(tr("Downloading images..."), tr("Cancel"), 0, 0);
  connect(progress, SIGNAL(canceled()), this, SLOT(cancelDownload()));
  progress->setWindowModality(Qt::WindowModal);
  progress->hide();
}

void stardust::open()
{
  QStringList fileNames = QFileDialog::getOpenFileNames( this );

  if ( !fileNames.isEmpty() ) {
    cleanUp();
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
    if (url.isValid()) {
      cleanUp();
      loadUrl(url);
    }
  }
}

void stardust::openFromKonqueror()
{
  cleanUp();
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

void stardust::about()
{
  aboutBox->show();
}

void stardust::preferences()
{
  setupPreferences();
  configDialog->exec();
}

void stardust::setupActionConnected(bool connected)
{
  disconnect(connectAct, SIGNAL( triggered() ));
  if (!connected) {
    connectAct->setIcon(QIcon( ":/connection-established.png" ));
    connectAct->setText(tr("&Connect to server"));
    connectAct->setStatusTip( tr( "Connect to the server and fetch movies" ) );
    connect( connectAct, SIGNAL( triggered() ), this, SLOT( connectToServer() ) );
  } else {
    connectAct->setIcon(QIcon( ":/connect-no.png" ));
    connectAct->setText(tr("&Disconnect from server"));
    connectAct->setStatusTip( tr( "Disconnect from the server" ) );
    connect( connectAct, SIGNAL( triggered() ), connector, SLOT( logout() ) );
  }
}

void stardust::cleanUp()
{
  httpGetId = 0;
  http->abort();
  refPath = QString();
  dlingImage = 0;
  hideProgress();
  dledImages.clear();
  dlbuffer->close();
  dlbuffer->setBuffer(0);
  currentUrl = QUrl();
  nextUrl = QUrl();
  curFrames = nextFrames = 0;
  connector->logout();
  triView->clear();
  info->clear();
  respondNoFocusAct->setDisabled(true);
  respondNoTrackAct->setDisabled(true);
  respondTrackAct->setDisabled(true);
  toggleCrosshairAct->setChecked(false);
  toggleCrosshairAct->setDisabled(true);
  setupActionConnected(false);
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
  cleanUp();
  connector->login(username, password);
}

void stardust::connecting()
{
  statusBar()->showMessage(tr("Connecting to server..."));
}

void stardust::connected()
{
  statusBar()->showMessage(tr("Connected to server."));
  
  respondTrackAct->setEnabled(true);
  respondNoTrackAct->setEnabled(true);
  respondNoFocusAct->setEnabled(true);
  setupActionConnected(true);
}

void stardust::error()
{
  statusBar()->showMessage(tr("Communication error."));
}

void stardust::createActions()
{
  openAct = new QAction( QIcon( ":/document-open.png" ), tr( "&Open..." ), this );
  openAct->setShortcut( tr( "Ctrl+O" ) );
  openAct->setStatusTip( tr( "Open existing files" ) );
  connect( openAct, SIGNAL( triggered() ), this, SLOT( open() ) );

  openUrlAct = new QAction( QIcon( ":/network.png" ), tr( "Open &URL..." ), this );
  openUrlAct->setShortcut( tr( "Ctrl+U" ) );
  openUrlAct->setStatusTip( tr( "Open a remote set of images" ) );
  connect( openUrlAct, SIGNAL( triggered() ), this, SLOT( openUrl() ) );
  
  openKonqAct = new QAction( QIcon( ":/konqueror.png" ), tr( "Open from &Konqueror" ), this );
  openKonqAct->setShortcut( tr( "Ctrl+K" ) );
  openKonqAct->setStatusTip( tr( "Open a set of images shown in Konqueror" ) );
  connect( openKonqAct, SIGNAL( triggered() ), this, SLOT( openFromKonqueror() ) );
  
  connectAct = new QAction( this );
  setupActionConnected(false);
  
  exitAct = new QAction( tr( "E&xit" ), this );
  exitAct->setShortcut( tr( "Ctrl+Q" ) );
  exitAct->setStatusTip( tr( "Exit the application" ) );
  connect( exitAct, SIGNAL( triggered() ), this, SLOT( close() ) );

  preferencesAct = new QAction( QIcon( ":/configure.png" ), tr( "P&references..." ), this );
  preferencesAct->setStatusTip( tr( "Set preferences, such as the username and the password" ) );
  connect( preferencesAct, SIGNAL( triggered() ), this, SLOT( preferences() ) );

  aboutAct = new QAction( tr( "&About" ), this );
  aboutAct->setStatusTip( tr( "Show the application's About box" ) );
  connect( aboutAct, SIGNAL( triggered() ), this, SLOT( about() ) );

  aboutQtAct = new QAction( tr( "About &Qt" ), this );
  aboutQtAct->setStatusTip( tr( "Show the Qt library's About box" ) );
  connect( aboutQtAct, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

  respondNoFocusAct = new QAction( tr( "&Bad focus" ), this);
  respondNoFocusAct->setShortcut(Qt::Key_Backspace);
  respondNoFocusAct->setStatusTip( tr( "Report bad focus" ));
  respondNoFocusAct->setDisabled(true);
  connect( respondNoFocusAct, SIGNAL( triggered() ),
           this, SLOT(respondNoFocus()) );
  
  respondNoTrackAct = new QAction( tr( "&No track" ), this);
  respondNoTrackAct->setShortcut(Qt::Key_Space);
  respondNoTrackAct->setStatusTip( tr( "Report no track" ));
  respondNoTrackAct->setDisabled(true);
  connect( respondNoTrackAct, SIGNAL( triggered() ),
           this, SLOT(respondNoTrack()) );
  
  respondTrackAct = new QAction( tr( "&Track" ), this);
  respondTrackAct->setShortcut(Qt::Key_Enter);
  respondTrackAct->setStatusTip( tr( "Report a track" ));
  respondTrackAct->setDisabled(true);
  connect( respondTrackAct, SIGNAL( triggered() ),
           this, SLOT(respondTrack()) );
  
  toggleCrosshairAct = new QAction( QIcon(":/cross_32.png"), tr( "&Crosshair"), this);
  toggleCrosshairAct->setCheckable(true);
  toggleCrosshairAct->setStatusTip( tr("Show or hide crosshair"));
  connect(toggleCrosshairAct, SIGNAL(toggled(bool)), triView, SLOT(setCrosshairsShown(bool)));
}

void stardust::createMenus()
{
  fileMenu = menuBar()->addMenu( tr( "&File" ) );
  fileMenu->addAction( openAct );
  fileMenu->addAction( openUrlAct );
  fileMenu->addAction( openKonqAct );
  fileMenu->addAction( connectAct );
  fileMenu->addSeparator();
  fileMenu->addAction( preferencesAct );
  fileMenu->addSeparator();
  fileMenu->addAction( exitAct );

  viewMenu = menuBar()->addMenu( tr( "&View" ) );
  viewMenu->addAction( toggleCrosshairAct );
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
  fileToolBar->addAction( openAct );
  fileToolBar->addAction( openUrlAct );
  fileToolBar->addAction( openKonqAct );
  fileToolBar->addAction( connectAct );
  
  viewToolBar = addToolBar( tr( "View" ) );
  viewToolBar->addAction( toggleCrosshairAct );

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
  progressBar->hide();
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

  setTitle(fileNames[0]);
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

void stardust::disconnected()
{
  cleanUp();
}

void stardust::imageLoaded()
{
  toggleCrosshairAct->setEnabled(true);
  toggleCrosshairAct->setChecked(true);
  statusBar()->showMessage( tr("Movie loaded."), 2000);
}

/*!
    \fn stardust::createView
 */
void stardust::createView()
{
  triView = new TriView;
  setCentralWidget( triView );
  
  info = new QLabel(this);
  triView->insertExtraWidget(info);
  connect(triView, SIGNAL(moved()), SLOT(updateStatusBar()));
  connect(triView, SIGNAL(imageLoaded()), SLOT(imageLoaded()));
}

void stardust::createConnector()
{
  connector = new StardustConnector();
  connect(connector, SIGNAL(connecting()), SLOT(connecting()));
  connect(connector, SIGNAL(error()), SLOT(error()));
  connect(connector, SIGNAL(connected()), SLOT(connected()));
  connect(connector, SIGNAL(disconnected()), SLOT(disconnected()));
  connect(connector, SIGNAL(newImage(const QUrl &, int)), 
          SLOT(loadUrl(const QUrl &, int)));
  connect(connector, SIGNAL(nextImage(const QUrl &, int)), 
          SLOT(gotNextUrl(const QUrl &, int)));
  connect(connector, SIGNAL(movieId(const QString &)),
          SLOT(setTitle(const QString &)));
  connect(connector, SIGNAL(userInfo(const QString &)),
          info, SLOT(setText(const QString &)));
}

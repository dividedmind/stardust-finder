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


#ifndef STARDUST_H
#define STARDUST_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QUrl>

class QAction;
class QMenu;
class QTextEdit;
class TriView;
class QHttp;
class QProgressDialog;
class QBuffer;
class QHttpResponseHeader;
class QProgressBar;
class QLabel;
class AboutBox;

namespace Ui {
  class StardustConfig;
};
class StardustConnector;

class stardust: public QMainWindow
{
  Q_OBJECT

public:
  stardust();
  ~stardust();

protected:
  void dragEnterEvent(QDragEnterEvent *event);
  void dropEvent(QDropEvent *event);

private slots:
  void open();
  void openUrl();
  void openFromKonqueror();
  void about();
  void preferences();
  void cancelDownload();
  void requestFinished(int, bool);
  void readResponseHeader(const QHttpResponseHeader &);
  void downloadNextImage();
  void finishedDling();
  void loadDledImages();
  void hideProgress();
  void showProgress(int max = 0);
  void downloadImages(const QUrl &url, int frames = 0);
  void setProgress(const QString &message, int image = 0);
  void preferencesAccepted();
  void connectToServer();
  void connecting();
  void error();
  void connected();
  void loadUrl( const QUrl &url, int frames = 0 );
  void gotNextUrl( const QUrl &url, int frames = 0 );
  void setTitle(const QString &title);
  void updateStatusBar();
  void respondNoTrack();
  void respondNoFocus();
  void respondTrack();
  void disconnected();
  void imageLoaded();

private:
  void setupActionConnected(bool connected);
  void cleanUp();
  void setupPreferences();
  void createActions();
  void createMenus();
  void createToolBars();
  void createStatusBar();
  void readSettings();
  void writeSettings();
  void loadFiles( const QStringList &fileNames );
  void processImage();
  void findNextUrl(const QString &filename);
    void createView();
    void createConnector();
    void createProgress();
    void createHttp();
    void createDialogs();

  QHttp *http;
  QString refPath;
  int dlingImage;
  QProgressDialog *progress;
  QVector<QImage> dledImages;
  QBuffer *dlbuffer;
  int httpGetId;
  QUrl currentUrl;
  QUrl nextUrl;
  bool backgroundLoading;
  
  QProgressBar *progressBar;
  int curFrames, nextFrames;
  
  StardustConnector *connector;
  
  TriView *triView;
  QLabel *info;

  AboutBox *aboutBox;
  
  QDialog *configDialog;
  Ui::StardustConfig *configDialog_ui;
  QString username, password;
  bool savePassword;
  bool autoConnect;
  bool requestConfirmation;
  
  QAction *respondTrackAct;
  QAction *respondNoTrackAct;
  QAction *respondNoFocusAct;
  
  
  QMenu *fileMenu;
  QMenu *viewMenu;
  QMenu *reportMenu;
  QMenu *helpMenu;
  QToolBar *fileToolBar;
  QToolBar *viewToolBar;
  QToolBar *reportToolBar;
  QAction *openAct;
  QAction *openUrlAct;
  QAction *openKonqAct;
  QAction *connectAct;
  QAction *exitAct;
  QAction *preferencesAct;
  QAction *aboutAct;
  QAction *aboutQtAct;
  
  QAction *toggleCrosshairAct;
};

#endif

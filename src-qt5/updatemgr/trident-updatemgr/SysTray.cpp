#include "SysTray.h"

#include <QIcon>
#include <QString>
#include <QMenu>
#include <QCoreApplication>
#include "updateMgr.h"

extern UpdateManager *UPMGR;

// === PUBLIC ===
SysTray::SysTray(){
  window = 0;
  updateIcon(true);
  connect(UPMGR, SIGNAL(updateStarting()), this, SLOT(updateIcon()) );
  connect(UPMGR, SIGNAL(updateFinished(bool)), this, SLOT(updateIcon()) );
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)) );
  //connect(this, SIGNAL(aboutToClose()), this, SLOT(closeApp()));
  //Setup the context menu
  this->setContextMenu(new QMenu());
  this->contextMenu()->addAction(QIcon::fromTheme("dialog-close"), tr("Close update manager"), this, SLOT(closeApp()) );
  this->contextMenu()->addSeparator();
  this->contextMenu()->addAction(QIcon::fromTheme("update-low"), tr("Check for updates"), this, SLOT(checkForUpdates()) );
  //Now the automatic recheck for updates timer (for systems that never log out)
  checkTimer = new QTimer(this);
    checkTimer->setInterval(8*60*60*1000); //8 hours
  connect(checkTimer, SIGNAL(timeout()), this, SLOT(checkForUpdates()) );
  checkTimer->start();
}

SysTray::~SysTray(){
  if(window!=0){
    window->deleteLater();
    window = 0;
  }
}


// === PUBLIC SLOTS ===
void SysTray::newInputs(QStringList list){
  if(!list.contains("-autostart")){
    QTimer::singleShot(0, this, SLOT(launchWindow()) );
  }
}


// === PRIVATE SLOTS ===
void SysTray::closeApp(){
  QCoreApplication::exit(0);
}

void SysTray::checkForUpdates(){
  UPMGR->startUpdateCheck();
}

void SysTray::launchWindow(){
  if(window==0){
    window = new MainUI();
    window->show();
    //connect(window, SIGNAL(aboutToClose()), this, SLOT(closeWindow()));
  }else{
    window->showNormal();
    window->activateWindow();
  }

}

void SysTray::closeWindow(){
  if(window==0){ return; }
  window->close();
}

void SysTray::activated(QSystemTrayIcon::ActivationReason reason){
  if(reason==QSystemTrayIcon::Context){
    this->contextMenu()->show();
  }else{
    launchWindow();
  }
}

void SysTray::updateIcon(bool firstrun){
  static QString oldstat;
  QString stat, tt;
  if(UPMGR->isRebootRequired()){
    stat = "warning";
     tt = tr("System reboot required to finish updates!");
  }else if(UPMGR->isUpdateRunning()){
    if(UPMGR->lastRunWasCheck()){
      stat =  "sync";
      tt = tr("Checking for updates");
    }else{
      stat = "download";
      tt = tr("Downloading updates");
    }
  }else if(UPMGR->updatesAvailable()){
    stat = "information";
    tt = tr("Updates Available");
  }else if(UPMGR->updatelog().isEmpty() || !UPMGR->updatelog().contains("Finished: OK")){
    stat = "offline";
    tt = "";
    //check again in 1.5 minutes (first run) or 10 minutes (later errors/checks)
    QTimer::singleShot(firstrun ? 90000 : 600000,  this, SLOT(checkForUpdates()) );
  }else{
    stat = "ok";
    tt = tr("System up to date");
  }
  this->setIcon( QIcon::fromTheme("state-"+stat) );
  this->setToolTip(tt);
  //Show a popup about available updates
  if(oldstat=="sync" && stat=="information"){ //only after a check actually ran (not just cached info)
    this->showMessage(tr("Updates Available"), tr("System updates are available.\nClick here for more details"), QIcon::fromTheme("system-upgrade"), 2000);
  }
  oldstat = stat;
}

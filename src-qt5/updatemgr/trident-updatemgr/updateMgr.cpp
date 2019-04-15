#include "updateMgr.h"

#define LOGFILE "/tmp/.sysup.status"
#define TRAINSFILE "/tmp/.sysup.trains"
#define REBOOTFILE "/tmp/.rebootRequired"

#define SYSMANIFEST "/var/db/current-manifest.json"
#define REPOMANIFEST "https://project-trident.org/repo-info.json"
#define REPOFILE "/tmp/.trident-repo.status"

#include <unistd.h>

QNetworkAccessManager *netman;
// === PUBLIC ===
UpdateManager::UpdateManager(QObject *parent) : QObject(parent) {
  repoCheck = 0;
  netman = new QNetworkAccessManager(this);
  PROC.setProcessChannelMode(QProcess::MergedChannels);
  connect(&PROC, SIGNAL(readyRead()), this, SLOT(processMessage()) );
  connect(&PROC, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int)) );

  TRPROC.setProcessChannelMode(QProcess::MergedChannels);
  connect(&TRPROC, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(trainsProcFinished(int)) );
  trainIsList = true;

  logcontents = readLocalFile(LOGFILE); //load any existing logfile first
  traincontents = readLocalFile(TRAINSFILE); //load any existing file first
  system_version = QJsonDocument::fromJson( readLocalFile(SYSMANIFEST).toLocal8Bit()).object().value("os_version").toString();
  repo_info = QJsonDocument::fromJson( readLocalFile(REPOFILE).toLocal8Bit() ).object();
  logfile = new QFile(LOGFILE);
  if(logfile->exists()){ lastcheck = logfile->fileTime(QFileDevice::FileModificationTime); }
  processIsCheck = logcontents.contains("Checking system for updates");
  connect(this, SIGNAL(startupdates(bool,bool)), this, SLOT(startUpdates(bool,bool))); //INTERNAL connection
}

UpdateManager::~UpdateManager(){
  if(logfile!=0){
    if(logfile->isOpen()){ logfile->close(); }
    logfile->deleteLater();
  }
}

QString UpdateManager::readLocalFile(QString path){
  QFile file(path);
  if(!file.exists()){ return ""; }
  QString tmp;
  if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
    QTextStream stream(&file);
    tmp = stream.readAll();
    file.close();
  }
  return tmp;
}

QString UpdateManager::updatelog(){
  return logcontents;
}

bool UpdateManager::lastRunWasCheck(){
  return processIsCheck;
}

bool UpdateManager::isUpdateRunning(){
  return (PROC.state()!=QProcess::NotRunning );
}

bool UpdateManager::isRebootRequired(){
  if(isUpdateRunning()){ return false; }
  else{
    return (logcontents.contains("Reboot your system"));
  }
}

bool UpdateManager::updatesAvailable(){
  if(isUpdateRunning() || !lastRunWasCheck()){ return false; }
  return (logcontents.contains("Checking system for updates") && !logcontents.contains("No updates available"));
}

bool UpdateManager::startUpdates(bool fullupdates){
  if( isUpdateRunning() || isRebootRequired() ){ return false; }
  this->emit startupdates(false, fullupdates);
  return true;
}

bool UpdateManager::startUpdateCheck(){
  if( isUpdateRunning() || isRebootRequired() ){ return false; }
  this->emit startupdates(true, false);
  return true;
}

QJsonObject UpdateManager::listTrains(){
  if(traincontents.isEmpty()){ startTrainsCheck(); return QJsonObject(); }
  //qDebug() << "Got Trains:" << traincontents;
  //Now parse the text into a JSON object
  QJsonObject obj;
  // First line contains the current Train
  QString ctrain = traincontents.section("\n",0,0).section("Train:", 1,-1).simplified();
  if(ctrain.isEmpty() || ctrain=="TrueOS"){ ctrain = "Trident-release"; } //default train  (release)
  //obj.insert("current", ctrain);
  QStringList trains = traincontents.section("------\n",-1).split("\n");
  for(int i=0; i<trains.length(); i++){
    if(trains[i].simplified().isEmpty()){ continue; }
    QJsonObject tobj;
    QString name =  trains[i].section("\t",0,0).simplified();
      tobj.insert("name", name);
      tobj.insert("description", trains[i].section("\t",1,-1).section("[",0,0).simplified());
      tobj.insert("current", name==ctrain);
      tobj.insert("active", !trains[i].contains("[Deprecated]", Qt::CaseInsensitive));
    obj.insert(name, tobj);
  }
  return obj;
}

bool UpdateManager::changeTrain(QString trainname){
  emit trainsStarting();
  trainIsList = false;
  TRPROC.start("/usr/local/sbin/.susysup", QStringList() << "-change-train" << trainname);
  return true;
}

// === PRIVATE ===
void UpdateManager::clear_logfile(){
  if(logfile->isOpen()){ logfile->close(); }
  logcontents.clear();
  logfile->open(QIODevice::WriteOnly | QIODevice::Truncate);
}

// === PRIVATE SLOTS ===
void UpdateManager::startUpdates(bool checkonly, bool fullupdate){
  processIsCheck=checkonly;
  QStringList args;
  args.append( checkonly ? "-check" : "-update" );
  if(!checkonly && fullupdate){ args.append("-fullupdate"); }
  //Clear out the current logs and start up the process
  clear_logfile();
  if(checkonly){ lastcheck = QDateTime::currentDateTime(); fetchRepoInfo(); }
  PROC.start("/usr/local/sbin/.susysup",args);
  emit updateStarting();
  injectIntoLog( checkonly ? tr("Checking for system updates...") : tr("Starting system updates...") );
}

void UpdateManager::fetchRepoInfo(){
  QNetworkRequest req( QUrl(REPOMANIFEST) );
  req.setHeader(QNetworkRequest::UserAgentHeader, system_version); //ensure user-agent anonymity
  repoCheck = netman->get(req);
  connect(repoCheck, SIGNAL(finished()), this, SLOT(saveRepoInfo()) );
}

void UpdateManager::saveRepoInfo(){
  if(repoCheck==0){ return; }
  QJsonObject repo = QJsonDocument::fromJson(repoCheck->readAll()).object();
  if(!repo.isEmpty()){
    repo_info = repo; //save this into internal cache
    QFile file(REPOFILE);
    if( file.open(QIODevice::WriteOnly | QIODevice::Truncate) ){
      QTextStream out(&file);
      out << QJsonDocument(repo).toJson(QJsonDocument::Indented);
      file.close();
    }
  }
  //Now close/reset that pointer
  disconnect(repoCheck);
  repoCheck->close();
  repoCheck->deleteLater();
  repoCheck = 0;
}

void UpdateManager::processMessage(){
  QString newstr = PROC.readAll();
  if(newstr.isEmpty()){ return; }
  //save into in-memory log
  logcontents.append(newstr);
  this->emit newUpdateMessage(processIsCheck, newstr);
  //save to file-based log
  //if(!processIsCheck){
    QTextStream outstr(logfile);
    outstr << newstr;
  //}
}

void UpdateManager::processFinished(int retcode){
  bool success = ( retcode==0 || (processIsCheck && retcode==10) );

  injectIntoLog( tr("Finished:")+" "+(success ? "OK" : ("ERROR ("+QString::number(retcode)+")")) );
  emit updateFinished(success);
  if(success && !processIsCheck){
    //Copy the successful log to the reboot file/flag.
    QFile::copy(LOGFILE, REBOOTFILE);
  }
}

void UpdateManager::trainsProcFinished(int retcode){
  //qDebug() << "Trains Proc Finished";
  bool success = (retcode==0);
  if(success && trainIsList){
    traincontents = TRPROC.readAll();
    if(traincontents.startsWith("Current Train:")){
      QFile file(TRAINSFILE);
      if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        QTextStream str(&file);
        str << traincontents;
        file.close();
      }
    }else{ traincontents.clear(); } //bad reply
    emit trainsAvailable();
  }else if(success){
    //Just changed trains, need to re-load
    QTimer::singleShot(0, this, SLOT(startTrainsCheck()));
    QTimer::singleShot(2000, this, SLOT(startUpdateCheck()));
  }
}

void UpdateManager::injectIntoLog(QString msg){
  QString curdt = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
  QString totalmsg = "["+curdt+"] "+msg+"\n";
  if(!logcontents.isEmpty() && !logcontents.endsWith("\n")){
    totalmsg.prepend("\n");
  }
  //save into in-memory log
  logcontents.append(totalmsg);
  this->emit newUpdateMessage(processIsCheck, totalmsg);
  //save to file-based log
  QTextStream outstr(logfile);
  outstr << totalmsg;
}

// === PUBLIC SLOTS ===
void UpdateManager::startTrainsCheck(){
  if(TRPROC.state()!=QProcess::NotRunning){ return; } //already running
  //qDebug() << "Starting check for Trains...";
  emit trainsStarting();
  trainIsList = true;
  TRPROC.start("/usr/local/sbin/.susysup", QStringList() << "-list-trains" );
}

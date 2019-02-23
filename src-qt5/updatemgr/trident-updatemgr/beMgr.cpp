#include "beMgr.h"
#include <QProcess>

beManager::beManager(QObject *parent) : QObject(parent){

}

beManager::~beManager(){

}

QJsonObject beManager::list_bootenv(){
  bool ok = false;
  QStringList lines = cmdOutput(ok, "beadm",QStringList() << "list" << "-H").split("\n");
  QJsonObject out;
  for(int i=0; i<lines.length(); i++){
    QStringList info = lines[i].split("\t");
    if(info.length()!=5){ continue; } //not a valid line
    QJsonObject tmp;
      tmp.insert("name",info[0]);
      tmp.insert("status",info[1]);
      tmp.insert("mountpoint",info[2]);
      tmp.insert("size",info[3]);
      tmp.insert("date_created",info[4]);
    out.insert(info[0], tmp);
  }
  return out;
}

bool beManager::activate_be(QString name){
  QStringList args; args << "beadm" << "activate" << name;
  bool ok = false;
  lastlog = cmdOutput(ok, "qsudo", args);
  return ok;
}

bool beManager::delete_be(QString name){
  QStringList args; args << "beadm" << "destroy" << "-F" << name;
  bool ok = false;
  lastlog = cmdOutput(ok, "qsudo", args);
  return ok;
}

QString beManager::lastCmdLog(){
  return lastlog;
}

QString beManager::cmdOutput(bool &ok, QString cmd, QStringList args){
  QProcess P;
  P.setProcessChannelMode(QProcess::MergedChannels);
  P.start(cmd, args);
  P.waitForFinished();
  ok = (P.exitCode() == 0);
  return P.readAllStandardOutput();
}

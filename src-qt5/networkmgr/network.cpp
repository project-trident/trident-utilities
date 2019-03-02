#include <network.h>

static QRegExp ip4regex = QRegExp("([0-9]{1,3}\.){3}[0-9]{1,3}");

Networking::Networking(QObject *parent) : QObject(parent){

}

Networking::~Networking(){

}

QStringList Networking::list_devices(){
  QProcess P;
    P.start("ifconfig", QStringList() << "-l" << "ether");
    P.waitForFinished();
  return QString(P.readAll()).section("\n",0,0).split(" ", QString::SkipEmptyParts);
}

QJsonObject Networking::list_config(QString device){
  static QRegExp ip4regex = QRegExp("([0-9]{1,3}\.){3}[0-9]{1,3}");
  QProcess P;
    P.start("sysrc", QStringList() << "-n" << device);
    P.waitForFinished();
  QStringList words = QString(P.readAll()).section("\n",0,0).split(" ", QString::SkipEmptyParts);
  QJsonObject obj;
  //Type of network
  obj.insert("network_type" , device.startsWIth("wlan") ? "wifi" : "lan");
  obj.insert("network_config", words.filter("DHCP").isEmpty() ? "manual" : "dhcp");
  //obj.insert();

  return obj;
}

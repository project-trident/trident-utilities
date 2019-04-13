#include <network.h>
#include <QDebug>
#include <QNetworkInterface>

static QRegExp ip4regex = QRegExp("([0-9]{1,3}\\.){3}[0-9]{1,3}");

Networking::Networking(QObject *parent) : QObject(parent){
  NETMAN = new QNetworkConfigurationManager(this);
}

Networking::~Networking(){

}

QStringList Networking::list_devices(){
  QList<QNetworkConfiguration> configs = NETMAN->allConfigurations();
  //qDebug() << "isOnline:" << NETMAN->isOnline();
  QStringList devs;
  for(int i=0; i<configs.length(); i++){
    //qDebug() << "config:" << configs[i].identifier() << configs[i].bearerTypeName() << configs[i].name() << configs[i].state();
    devs << configs[i].name();
  }
  return devs;
}

QJsonObject Networking::list_config(QString device){
  QProcess P;
    P.start("sysrc", QStringList() << "-n" << "ifconfig_"+device);
    P.waitForFinished();
  QStringList words = QString(P.readAll()).section("\n",0,0).split(" ", QString::SkipEmptyParts);
  QJsonObject obj;
  //Type of network
  obj.insert("network_type" , device.startsWith("wlan") ? "wifi" : "lan");
  if(words.filter("DHCP").isEmpty()){
    obj.insert("network_config", "manual");
    int index = words.indexOf("inet");
    if(index>=0 && words.length() > index+1){ obj.insert("ipv4_address", words[index+1]); }
    index = words.indexOf("inet6");
    if(index>=0 && words.length() > index+1){ obj.insert("ipv6_address", words[index+1]); }
    
  }else{
    obj.insert("network_config", "dhcp");
  }

  //obj.insert();
  //TO-DO
  // Need to parse out the ipv4 settings here
  qDebug() << "List Config:" << device << words;
  return obj;
}

QJsonObject Networking::current_info(QString device){
  QNetworkInterface config = QNetworkInterface::interfaceFromName(device);
  QJsonObject obj;
  if(!config.isValid()){ return obj; }
  obj.insert("hardware_address", config.hardwareAddress());
  obj.insert("is_up", config.flags().testFlag(QNetworkInterface::IsUp));
  obj.insert("is_running", config.flags().testFlag(QNetworkInterface::IsRunning));
  obj.insert("can_broadcast", config.flags().testFlag(QNetworkInterface::CanBroadcast));
  obj.insert("is_loopback", config.flags().testFlag(QNetworkInterface::IsLoopBack));
  obj.insert("can_multicast", config.flags().testFlag(QNetworkInterface::CanMulticast));
  obj.insert("is_pt2pt", config.flags().testFlag(QNetworkInterface::IsPointToPoint));
  QList<QNetworkAddressEntry> addresses = config.addressEntries();
  bool ok = false;
  for(int i=0; i<addresses.length(); i++){
    addresses[i].ip().toIPv4Address(&ok);
    if(ok){
      obj.insert("ipv4", addresses[i].ip().toString());
      obj.insert("ipv4_netmask", addresses[i].netmask().toString());
      obj.insert("ipv4_broadcast", addresses[i].broadcast().toString());
      //obj.insert("ipv4_gateway", addresses[i].gateway().toString());
    }else{
      obj.insert("ipv6", addresses[i].ip().toString());
      obj.insert("ipv6_netmask", addresses[i].netmask().toString());
      obj.insert("ipv6_broadcast", addresses[i].broadcast().toString());
     // obj.insert("ipv6_gateway", addresses[i].gateway().toString());
    }
  }
  return obj;
}

bool Networking::set_config(QString device, QJsonObject config){
  qDebug() << "set Config:" << device << config;
  return false;
}

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
  if(device.isEmpty()){ return QJsonObject(); } //nothing to do - invalid request
  QStringList words = CmdOutput("sysrc", QStringList() << "-n" << "ifconfig_"+device).section("\n",0,0).split(" ", QString::SkipEmptyParts);
  QJsonObject obj;
  //Type of network
  obj.insert("network_type" , device.startsWith("wlan") ? "wifi" : "lan");
  if(words.filter("DHCP").isEmpty()){
    obj.insert("network_config", "manual");
    int index = words.indexOf("inet");
    if(index>=0 && words.length() > index+1){ obj.insert("ipv4_address", words[index+1]); }
    index = words.indexOf("netmask");
    if(index>=0 && words.length() > index+1){ obj.insert("ipv4_netmask", words[index+1]); }
    index = words.indexOf("inet6");
    if(index>=0 && words.length() > index+1){ obj.insert("ipv6_address", words[index+1]); }
    //Also fetch the defaultrouter and put that in as the gateway
    QString defrouter = CmdOutput("sysrc", QStringList() << "-n" << "defaultrouter").simplified();
    obj.insert("ipv4_gateway", defrouter);

  }else{
    obj.insert("network_config", "dhcp");
  }
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

Networking::State Networking::deviceState(QString device){
  QNetworkInterface config = QNetworkInterface::interfaceFromName(device);
  if(!config.isValid()){ return StateUnknown; }
  if( config.flags().testFlag(QNetworkInterface::IsUp) ){ return StateRunning; }
  else{ return StateStopped; }
}


//  === PRIVATE ===
QString Networking::CmdOutput(QString proc, QStringList args){
  QProcess P;
    P.start(proc, args);
    P.waitForFinished();
  return P.readAll();
}

int Networking::CmdReturn(QString proc, QStringList args){
  QProcess P;
    P.start(proc, args);
    P.waitForFinished();
  return P.exitCode();
}

// === PUBLIC SLOTS ===
bool Networking::setDeviceState(QString device, State stat){
  bool ok = false;
  State curstate = deviceState(device);
  if(curstate == stat){ return true; } //nothing to do
  switch(stat){
    case StateRunning:
      //Start the network device
      qDebug() << "Starting network device:" << device;
      ok = (CmdReturn("ifconfig", QStringList() << device << "up") == 0);
      break;
    case StateStopped:
      //Stop the network device
      qDebug() << "Stopping network device:" << device;
      ok = (CmdReturn("ifconfig", QStringList() << device << "down") == 0);
      break;
    case StateRestart:
      //Restart the network device
      qDebug() << "Restarting network device:" << device;
      ok = (CmdReturn("ifconfig", QStringList() << device << "down") == 0);
      QThread::sleep(1);
      ok = (CmdReturn("ifconfig", QStringList() << device << "up") == 0);
      break;
    case StateUnknown:
      break; //do nothing
  }
  return ok;
}

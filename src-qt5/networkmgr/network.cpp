#include <network.h>
#include <QDebug>
#include <QNetworkInterface>

static QRegExp ip4regex = QRegExp("([0-9]{1,3}\\.){3}[0-9]{1,3}");
static QRegExp bssidRegex = QRegExp("([^:]{2}:){5}[^:]{2}");

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
  obj.insert("network_type" , device.startsWith("wl") ? "wifi" : "lan");
  if(!words.isEmpty() && words.filter("DHCP").isEmpty()){
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
  obj.insert("is_wifi", config.type() == QNetworkInterface::Wifi || device.startsWith("wl"));
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
  //qDebug() << "Device Info:" << device << obj;
  bool active_connection = obj.value("is_up").toBool(false) && !obj.value("ipv4").toString().isEmpty();
  obj.insert("is_active", active_connection);
  //Now do any wifi-specific checks
  if( obj.value("is_wifi").toBool() && obj.value("is_up").toBool() ){
    QJsonObject connection;
    QStringList tmp = CmdOutput("wpa_cli", QStringList() << "-i" << device << "status").split("\n");
    for(int i=0; i<tmp.length(); i++){
      if( !tmp[i].contains("=")){ continue; }
      connection.insert( tmp[i].section("=",0,0), tmp[i].section("=",1,-1) );
    }
    //qDebug() << "wifi Status\n" << tmp;
    obj.insert("wifi", connection);
    //qDebug() << " - Parsed:" << connection;
  }else if (obj.value("is_up").toBool() ){
    //Wired connection
    QJsonObject connection;
/*    QString tmp = CmdOutput("ifconfig", QStringList() << device);
    //qDebug() << "wifi Status\n" << tmp;
    if(!tmp.isEmpty()){
      connection.insert("media", tmp.section("\tmedia: ",-1).section("\n",0,0).simplified());
    }*/
    obj.insert("lan", connection);
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

QJsonObject Networking::scan_wifi_networks(QString device){
  qDebug() << "Scan for Wifi:" << device;
  QStringList known = known_wifi_networks();
  CmdOutput("wpa_cli", QStringList() << "-i" << device << "scan");
  QThread::sleep(2);
  QStringList lines = CmdOutput("wpa_cli", QStringList() << "-i" << device << "scan_results" ).split("\n");
  qDebug() << "Got wifi scan:" << lines;
  QJsonObject out;
  for(int i=1; i<lines.length(); i++){ //first line is column headers
    if(lines[i].simplified().isEmpty()){ continue; }
    //Columns: [bssid, frequency, signal level, [flags], ssid]
    QJsonObject tmp;
    QString ssid = lines[i].section("\t",-1).simplified();
    QString bssid = lines[i].section("\t",0,0, QString::SectionSkipEmpty);
    tmp.insert("ssid", ssid);
    tmp.insert("bssid", bssid);
    tmp.insert("freq",  lines[i].section("\t",1,1, QString::SectionSkipEmpty));
    int sig = lines[i].section("\t",2,2, QString::SectionSkipEmpty).simplified().toInt();
    tmp.insert("sig_db_level", sig);
    if(sig>0){ sig = 0; } //should always be negative
    if(sig<-100){ sig = -100; } //just set the minimum here so it gets seen as "0%"
    int sigpercent = qAbs(100 + sig); //quick and dirty percentage: 100% - signal level (sig level always negative)
    if(sigpercent<10){ tmp.insert("signal", "00"+QString::number(sigpercent)+"%"); }
    else if(sigpercent<100){ tmp.insert("signal", "0"+QString::number(sigpercent)+"%"); }
    else{ tmp.insert("signal", QString::number(sigpercent)+"%"); }
    tmp.insert("sig_percent", sigpercent);
    QStringList cap = lines[i].section("[",1,-1).section("]",0,-2).split("][");
    //for(int j=6; j<columns.length(); j++){ cap << columns[j].split(" ", QString::SkipEmptyParts); }
    tmp.insert("capabilities", QJsonArray::fromStringList(cap));
    tmp.insert("is_locked", !cap.filter("WPA").isEmpty());
    tmp.insert("is_known", known.contains(ssid) || known.contains(tmp.value("bssid").toString()) );
    if(out.contains(ssid)){
      //Convert this to an array of access points with the same ssid
      QJsonArray arr;
      if(out.value(ssid).isArray()){ arr = out.value(ssid).toArray(); }
      else { arr << out.value(ssid).toObject(); }
      arr << tmp;
      out.insert(ssid, arr);
    }else{
      out.insert(ssid, tmp); //first object with this ssid
    }
  }
  qDebug() << "Wifi:" << out;
  return out;
}

QStringList Networking::known_wifi_networks(){
  static QStringList idcache;
  static QDateTime cachecheck;
  QDateTime lastMod = QFileInfo("/etc/wpa_supplicant/wpa_supplicant.conf").lastModified();
  if(cachecheck.isNull() || (lastMod > cachecheck) ){
    //Need to re-read the file to assemble the list of ID's.
    idcache.clear();
    QStringList contents = readFile("/etc/wpa_supplicant/wpa_supplicant.conf");
    cachecheck = QDateTime::currentDateTime();
    //qDebug() << "WPA Contents:" <<  contents;
    bool inblock=false;
    for(int i=0; i<contents.length(); i++){
      QString line = contents[i].simplified();
      if(!inblock && line.startsWith("network") && line.endsWith("{" )){ inblock = true; }
      else if(inblock && line.startsWith("}") ){ inblock = false; }
      else if(inblock){
        QString key = line.section("=",0,0).simplified();
        if(key == "ssid" || key =="bssid"){ idcache << line.section("=\"",-1).section("\"",0,0); }
      }
    }
  }
  qDebug() << "Known wifi networks:" << idcache;
  return idcache;
}

bool Networking::save_wifi_network(QJsonObject obj, bool clearonly){
  static QStringList exclude_quotes;
  qDebug() << "Save Wifi Network:" << obj << clearonly;
  if(exclude_quotes.isEmpty()){ exclude_quotes << "key_mgmt" << "eap" << "proto"; }
  QString id = obj.value("bssid").toString();
  QString ssid = obj.value("ssid").toString();
  if(id.isEmpty()){ id = ssid; }
  if(id.isEmpty()){ return false; }
  QStringList contents = readFile("/etc/wpa_supplicant/wpa_supplicant.conf");
  if(contents.isEmpty()){ contents << "ctrl_interface=/var/run/wpa_supplicant"; }
    //Need to remove the currently-saved entry first
    QStringList tmp = contents.join("\n").split("{");
    for(int i=0; i<tmp.length(); i++){
      if(tmp[i].contains("=\""+id+"\"") || tmp[i].contains("=\""+ssid+"\"") ){
        tmp.removeAt(i);
        if(i>=tmp.length()){
          //Need to remove the last piece of the network= section
          tmp[i-1] = tmp[i-1].section("network=",0,-2);
        }else{ i--; }
      }
    }
    contents = tmp.join("{").split("\n");
  if(!clearonly){
    if(!contents.last().isEmpty()){ contents<<""; }
    //Now add the new entry to the file contents
    contents << "network={";
    QStringList keys = obj.keys();
    for(int i=0; i<keys.length(); i++){
      QJsonValue val = obj.value(keys[i]);
      QString sval;
      if(val.isString()){
        if(exclude_quotes.contains(keys[i])){ sval = val.toString(); }
        else{ sval = "\""+val.toString()+"\""; }
      } else if(val.isDouble()){ sval = QString::number(val.toInt()); }
      else if(val.isBool()){ sval = QString::number(val.toInt()); }

      if(!sval.isEmpty()){ contents << "  "+keys[i]+"="+sval; }
    }
    contents << "}";
  }
  bool ok = writeFile("/etc/wpa_supplicant/wpa_supplicant.conf", contents);
  if(ok){CmdReturn("wpa_cli", QStringList() << "reconfigure"); } //have it re-read the config file
  return ok;
}

bool Networking::remove_wifi_network(QString id){
  //Note - it is better to call the save_wifi_network with the clear flag, can filter by ssid AND bssid in one pass
  QJsonObject obj;
  obj.insert("bssid", id); //does not matter if bssid or ssid right now. same removal process for both
  return save_wifi_network(obj, true); //remove only
}

bool Networking::connect_to_wifi_network(QString device, QString id){
  //ssid or bssid
  if(!device.startsWith("wlan")){ return false; } //not a wifi device
  int ret = -1;
  if(bssidRegex.exactMatch(id)){
    ret = CmdReturn("ifconfig", QStringList() << device << "bssid" << id );
  } else {
    ret = CmdReturn("ifconfig", QStringList() << device << "ssid" << id );
  }
  if(ret == 0 ){
    //Now need to tell the device to re-connect to the new [b]ssid
    return setDeviceState(device, StateRestart);
  }else{
    return false;
  }
}

//General Purpose functions
QStringList Networking::readFile(QString path){
  QFile file(path);
  QStringList contents;
  if(file.open(QIODevice::ReadOnly)){
    QTextStream in(&file);
    contents = in.readAll().split("\n");
    file.close();
  }else{
    qDebug() << "Could not read file:" << path;
  }
  return contents;
}

bool Networking::writeFile(QString path, QStringList contents){
  QString newpath = path+".new";
  //Make sure the parent directory exists first
  QDir dir; dir.mkpath(path.section("/",0,-2));
  //Write the file to a new location
  QFile file(newpath);
  if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){ return false; } //could not open the file
  if(!contents.isEmpty()){
    QTextStream out(&file);
    out << contents.join("\n");
    //Most system config files need to end with a newline to be valid
    if(!contents.last().endsWith("\n")){ out << "\n"; }
  }
  file.close();
  //Now replace the original file
  if(QFile::exists(path)){ QFile::remove(path); }
  return QFile::rename(newpath, path); //now do a simple rename of the file.
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

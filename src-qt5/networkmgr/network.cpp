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

QJsonObject Networking::list_config(){
  QJsonObject obj;
  // Need to read /etc/dhcpcd.conf and pull out all profile entries
  // Example entry
  // arping 192.168.0.1
  // profile 192.168.0.1
  // static ip_address=192.168.0.10/24
  // static routers=192.168.0.1

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

bool Networking::set_config(QJsonObject config){
  qDebug() << "set Config:" << config;
  return false;
}

Networking::State Networking::deviceState(QString device){
  QNetworkInterface config = QNetworkInterface::interfaceFromName(device);
  if(!config.isValid()){ return StateUnknown; }
  if( config.flags().testFlag(QNetworkInterface::IsUp) ){ return StateRunning; }
  else{ return StateStopped; }
}

QJsonObject Networking::scan_wifi_networks(){
  //qDebug() << "Scan for Wifi:" << device;
  QJsonArray known = known_wifi_networks();
  CmdOutput("wpa_cli", QStringList() << "scan");
  QThread::sleep(5);
  QStringList lines = CmdOutput("wpa_cli", QStringList() << "scan_results" ).split("\n");
  //qDebug() << "Got wifi scan:" << lines;
  QJsonObject out;
  for(int i=1; i<lines.length(); i++){ //first line is column headers
    if(lines[i].simplified().isEmpty()){ continue; }
    //Columns: [bssid, frequency, signal level, [flags], ssid]
    QJsonObject tmp;
    QString ssid = lines[i].section("\t",-1).simplified();
    QString bssid = lines[i].section("\t",0,0, QString::SectionSkipEmpty);
    if( !bssidRegex.exactMatch(bssid) ){ continue; }
    tmp.insert("ssid", ssid);
    tmp.insert("bssid", bssid);
    tmp.insert("freq",  lines[i].section("\t",1,1, QString::SectionSkipEmpty));
    int sig = lines[i].section("\t",2,2, QString::SectionSkipEmpty).simplified().toInt();
    tmp.insert("sig_db_level", sig);
    //Quick and dirty percent: assume -80DB for noise, and double the difference between Sig/Noise
    int sigpercent = (sig+80) *2;
    if(sigpercent<0){ sigpercent = 0; }
    else if(sigpercent>100){ sigpercent = 100; }
    if(sigpercent<10){ tmp.insert("signal", "00"+QString::number(sigpercent)+"%"); }
    else if(sigpercent<100){ tmp.insert("signal", "0"+QString::number(sigpercent)+"%"); }
    else{ tmp.insert("signal", QString::number(sigpercent)+"%"); }
    tmp.insert("sig_percent", sigpercent);
    QStringList cap = lines[i].section("[",1,-1).section("]",0,-2).split("][");
    //for(int j=6; j<columns.length(); j++){ cap << columns[j].split(" ", QString::SkipEmptyParts); }
    tmp.insert("capabilities", QJsonArray::fromStringList(cap));
    tmp.insert("is_locked", !cap.filter("WPA").isEmpty());
    bool is_known = false;
    bool is_active = false;
    for(int i=0; i<known.count(); i++){
      QJsonObject obj = known[i].toObject();
      if(obj.value("bssid").toString()==bssid){ is_known = true; }
      else if(!obj.contains("bssid") && obj.value("ssid").toString() == ssid){ is_known = true; }
      if(is_known){
        //Also see if this access point is currently active
        if(obj.value("id").toString() == activeNetworkID){ is_active = true; }
      }
    }
    tmp.insert("is_known", is_known);
    tmp.insert("is_active", is_active);
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
  //qDebug() << "Wifi:" << out;
  return out;
}

QJsonArray Networking::known_wifi_networks(){
  static QJsonArray idcache;
  static QDateTime cachecheck;
  QDateTime lastMod = QFileInfo("/etc/wpa_supplicant/wpa_supplicant.conf").lastModified();
  if(cachecheck.isNull() || (lastMod > cachecheck) ){
    //Need to re-read the file to assemble the list of ID's.
    idcache = QJsonArray();
    QStringList contents = CmdOutput("wpa_cli", QStringList() << "list_networks").split("\n");
    cachecheck = QDateTime::currentDateTime();
    //qDebug() << "WPA Contents:" <<  contents;
    for(int i=0; i<contents.length(); i++){
      QStringList elems = contents[i].split("\t");
      // Elements: [ network id, ssid, bssid, flags]
      if(elems.length() < 3){ continue; } //not a valid entry line
      QJsonObject obj;
        obj.insert("id", elems[0]);
        obj.insert("ssid", elems[1]);
        if(elems[2] != "any"){ obj.insert("bssid", elems[2]); }
        if( (elems.length()>=4) && contents[i].contains("[CURRENT]") ){
          activeNetworkID = elems[0];
        }
      idcache << obj;
    }
    qDebug() << "Known wifi networks:" << idcache;
  }

  return idcache;
}

QString Networking::active_wifi_network(){
  return activeNetworkID;
}

bool Networking::save_wifi_network(QJsonObject obj, bool clearonly){
  qDebug() << "Save Wifi Network:" << obj << clearonly;
  QString id = obj.value("id").toString();
  QString ssid = obj.value("ssid").toString();
  QString bssid = obj.value("bssid").toString();
  QString password = obj.value("psk").toString();
  if(ssid.isEmpty() && bssid.isEmpty() && !clearonly){ return false; }
  else if(id.isEmpty() && clearonly){ return false; }
  bool ok = true;
  if(clearonly) {
    ok = CmdReturn("wpa_cli", QStringList() << "remove_network" << id);
  }else{
    if(id.isEmpty()){
      //Need to get a new ID
      id = CmdOutput("wpa_cli", QStringList() << "add_network").section("\n",-1,QString::SectionSkipEmpty);
    }
    if(!ssid.isEmpty()){ ok = CmdReturn("wpa_cli", QStringList() << "set_network" << id << "ssid" << "\""+ssid+"\""); }
    if(!bssid.isEmpty() && ok){ ok = CmdReturn("wpa_cli", QStringList() << "set_network" << id << "bssid" << "\""+bssid+"\""); }
    if(!password.isEmpty() && ok){ ok = CmdReturn("wpa_cli", QStringList() << "set_network" << id << "psk" << "\""+password+"\""); }
  }
  if(ok){CmdReturn("wpa_cli", QStringList() << "save_config"); } //have it sync to disk
  if(!clearonly){ connect_to_wifi_network(id); }
  return ok;
}

bool Networking::remove_wifi_network(QString id){
  //Note - it is better to call the save_wifi_network with the clear flag, can filter by ssid AND bssid in one pass
  QJsonObject obj;
  obj.insert("id", id);
  return save_wifi_network(obj, true); //remove only
}

bool Networking::connect_to_wifi_network(QString id){
  bool ok = CmdReturn("wpa_cli", QStringList() << "select_network" << id);
  if(ok){ activeNetworkID = id; }
  qDebug() << "Connect to wifi:" << id << ok;
  return ok;
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
      ok = (CmdReturn("qsudo", QStringList() << "ip" << "link" << "set" << device << "up") == 0);
      break;
    case StateStopped:
      //Stop the network device
      qDebug() << "Stopping network device:" << device;
      ok = (CmdReturn("qsudo", QStringList() << "ip" << "link" << "set" << device << "down") == 0);
      break;
    case StateRestart:
      //Restart the network device
      qDebug() << "Restarting network device:" << device;
      ok = (CmdReturn("qsudo", QStringList() << "ip" << "link" << "set" << device << "down") == 0);
      QThread::sleep(1);
      ok = (CmdReturn("qsudo", QStringList() << "ip" << "link" << "set" << device << "up") == 0);
      break;
    case StateUnknown:
      break; //do nothing
  }
  return ok;
}

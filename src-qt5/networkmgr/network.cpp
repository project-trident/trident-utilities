#include <network.h>
#include <QDebug>
#include <QNetworkInterface>
#include <QtConcurrent>

static QRegExp ip4regex = QRegExp("([0-9]{1,3}\\.){3}[0-9]{1,3}");
static QRegExp bssidRegex = QRegExp("([^:]{2}:){5}[^:]{2}");
static QString DHCPConf = "/etc/dhcpcd.conf";

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
  QStringList lines = readFile(DHCPConf);
  // Need to read /etc/dhcpcd.conf and pull out all profile entries
  // Example entry
  // arping 192.168.0.1
  // profile 192.168.0.1
  // static ip_address=192.168.0.10/24
  // static routers=192.168.0.1
  bool inblock = false;

  QJsonObject out;
  QJsonObject obj;
  for(int i=0; i<lines.length(); i++){
    if(lines[i].startsWith("arping ")){
      QJsonArray checks = out.value("pings").toArray();
      QStringList tmp = lines[i].section(" ",1,-1).split(" ", QString::SkipEmptyParts);
      for(int j=0; j<tmp.length(); j++){ checks << tmp[j]; }
      out.insert("pings", checks);
    }else if(lines[i].startsWith("profile ") ){
        //Starting a profile block
        inblock = true;
        obj.insert("profile", lines[i].section(" ",1,1));
    }else if(inblock){
      if(lines[i].startsWith("#") || lines[i].simplified().isEmpty()){
        //End of profile block - save it to the output object
        inblock = false;
        out.insert(obj.value("profile").toString(), obj);
        obj = QJsonObject();
      }else if(lines[i].startsWith("static ") ){
        //value within a profile block
        QString key = lines[i].section(" ",1,-1).section("=",0,0).simplified();
        obj.insert(key, lines[i].section("=",1,-1).simplified());
      }
    }
  }
  return out;
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
  // Example entry
  // arping 192.168.0.1 192.168.1.1
  // profile 192.168.0.1
  // static ip_address=192.168.0.10/24
  // static routers=192.168.0.1
  //
  // profile 192.168.1.1
  // static ip_address=192.168.1.10/24
  // static routers=192.168.1.1
  QStringList contents = readFile(DHCPConf);
  int startindex = -1;
  for(int i=0; i<contents.length(); i++){
    if(contents[i].startsWith("# -- Trident-networkmgr config below --")){
      startindex = i-1; break; //make sure we "start" one line above this
    }
  }
  bool changed = false;
  if(startindex>=0){
    changed = true; //have to delete entries from the end of the file
    // Delete the end of the file as needed - gets replaced in a moment
    for(int i=contents.length()-1; i>startindex; i--){ contents.removeAt(i); }
  }
  if(!config.keys().isEmpty()){
    changed = true; //have to add entries to the bottom of the file
    contents << "# -- Trident-networkmgr config below --";
    contents << "# -- Place all manual changes above this --";
    QStringList pings = config.keys();
    contents << "arping "+pings.join(" ");
    for(int i=0; i<pings.length(); i++){
      QJsonObject profile = config.value(pings[i]).toObject();
      if(profile.isEmpty() || !profile.contains("profile") ){ continue; }
      contents << "";
      contents << "profile " +profile.value("profile").toString();
      if(profile.contains("ip_address")){ contents << "static ip_address "+profile.value("ip_address").toString(); }
      if(profile.contains("routers")){ contents << "static routers "+profile.value("routers").toString(); }
    }
  }
  if(!changed){ return true; } //nothing to do.
  if(contents.last() != ""){ contents << ""; } //always leave a blank line at the end

  // Now save the new config file and update dhcpcd to use it
  QString tmpfile = "/tmp/.tmp.dhcpcd.conf";
  static QString enablebin = QCoreApplication::applicationDirPath()+"/trident-enable-dhcpcdconf";
  bool ok = writeFile(tmpfile, contents);
  if(ok){
    ok = CmdReturn("qsudo", QStringList() << enablebin << tmpfile);
  }
  if(QFile::exists(tmpfile)){ QFile::remove(tmpfile); } //clean up if needed
  return ok;
}

Networking::State Networking::deviceState(QString device){
  QNetworkInterface config = QNetworkInterface::interfaceFromName(device);
  if(!config.isValid()){ return StateUnknown; }
  if( config.flags().testFlag(QNetworkInterface::IsUp) ){ return StateRunning; }
  else{ return StateStopped; }
}

QJsonObject Networking::wifi_scan_results(){
  return last_wifi_scan;
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
      idcache << obj;
    }
    //qDebug() << "Known wifi networks:" << idcache;
  }

  return idcache;
}

QJsonObject Networking::active_wifi_network(){
  QJsonObject obj;
  QStringList info = CmdOutput("wpa_cli", QStringList() << "status").split("\n");
  for(int i=0; i<info.length(); i++){
    QString key = info[i].section("=",0,0);
    if(key == "id" || key == "bssid" || key == "ssid"){
      obj.insert(key, info[i].section("=",1,-1));
    }
  }
  return obj;
}

bool Networking::is_known(QJsonObject obj){
  QJsonArray known = known_wifi_networks();
  for(int i=0; i<known.count(); i++){
    if( sameNetwork(known[i].toObject(), obj)){ return true; }
  }
  return false;
}

bool Networking::save_wifi_network(QJsonObject obj, bool clearonly){
  QString id = obj.value("id").toString();
  QString ssid = obj.value("ssid").toString();
  QString bssid = obj.value("bssid").toString();
  QString password = obj.value("psk").toString();
  if(ssid.isEmpty() && bssid.isEmpty() && !clearonly){ return false; }
  else if(id.isEmpty() && clearonly){ return false; }
  if(clearonly){ qDebug() << "Remove Wifi Network:" << ssid; }
  else{ qDebug() << "Save Wifi Network:" << ssid; }
  if(clearonly) {
    CmdReturn("wpa_cli", QStringList() << "remove_network" << id);
  }else{
    if(id.isEmpty()){
      //Need to get a new ID
      QStringList val = CmdOutput("wpa_cli", QStringList() << "add_network").split("\n");
      for(int i=val.length()-1; i>=0; i-- ){
        if(val[i].simplified().isEmpty()){ continue; }
        else if(val[i].simplified().toInt() >= 0){ id = val[i].simplified(); break; }
      }
      if(id.isEmpty()){ return false; }
      //qDebug() << "New network ID:" << id << "raw:" << val;
    }
    if(!ssid.isEmpty()){ CmdReturn("wpa_cli", QStringList() << "set_network" << id << "ssid" << "\""+ssid+"\""); }
    if(!bssid.isEmpty()){ CmdReturn("wpa_cli", QStringList() << "set_network" << id << "bssid" << bssid); }
    if(!password.isEmpty()){ CmdReturn("wpa_cli", QStringList() << "set_network" << id << "psk" << "\""+password+"\""); }
  }
  CmdReturn("wpa_cli", QStringList() << "save_config");
  bool ok = false;
  if(clearonly){ ok = !is_known(obj); }
  else { ok = is_known(obj); }
  if(!clearonly && ok){ connect_to_wifi_network(id); }
  return ok;
}

bool Networking::remove_wifi_network(QString id){
  //Note - it is better to call the save_wifi_network with the clear flag, can filter by ssid AND bssid in one pass
  QJsonObject obj;
  obj.insert("id", id);
  return save_wifi_network(obj, true); //remove only
}

bool Networking::connect_to_wifi_network(QString id, bool noretry){
  //qDebug() << "Connect to network:" << id;
  bool ok = CmdOutput("wpa_cli", QStringList() << "select_network" << id).contains("OK");
  if(!ok && !noretry){
    CmdReturn("wpa_cli", QStringList() << "reconfigure"); //poke wpa to start attempting again
    QThread::sleep(1);
    ok = connect_to_wifi_network(id, true); //do not re-try again
  }
  if(ok){ CmdReturn("wpa_cli", QStringList() << "save_config"); } //go ahead and update config
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

bool Networking::sameNetwork(QJsonObject A, QJsonObject B){
  if(A.contains("bssid") && B.contains("bssid")){
    return (A.value("bssid") == B.value("bssid"));
  }else if(A.contains("ssid") && B.contains("ssid")){
    return (A.value("ssid") == B.value("ssid"));
  }
  return false;
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

void Networking::performWifiScan(){
  this->emit starting_wifi_scan();
  CmdOutput("wpa_cli", QStringList() << "scan");
  for(int i=0; i<10; i++){
    QThread::sleep(1);
    QStringList lines = CmdOutput("wpa_cli", QStringList() << "scan_results" ).split("\n");
    parseWifiScanResults(lines);
  }
  this->emit finished_wifi_scan();
}

void Networking::parseWifiScanResults(QStringList lines){
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
  if(out != last_wifi_scan){
    last_wifi_scan = out;
    this->emit new_wifi_scan_results();
  }
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

void Networking::startWifiScan(){
  QtConcurrent::run(this, &Networking::performWifiScan);
}

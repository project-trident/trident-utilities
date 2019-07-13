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
  obj.insert("is_wifi", device.startsWith("wlan"));
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
  //Now do any wifi-specific checks
  if( device.startsWith("wlan") && obj.value("is_up").toBool() ){
    QJsonObject connection;
    QString tmp = CmdOutput("ifconfig", QStringList() << device);
    //qDebug() << "wifi Status\n" << tmp;
    connection.insert("media", tmp.section("\tmedia: ",-1).section("\n",0,0).simplified());
    connection.insert("ssid", tmp.section("\tssid ",-1).section(" channel",0,0).simplified());
    connection.insert("bssid", tmp.section(" bssid ",-1).section("\n", 0,0, QString::SectionSkipEmpty).section(" ",0,0).simplified());
    connection.insert("authmode", tmp.section(" authmode ",-1).section(" ", 0,0, QString::SectionSkipEmpty));
    obj.insert("wifi", connection);
    //qDebug() << " - Parsed:" << connection;
  }else{
    //Wired connection
    QJsonObject connection;
    QString tmp = CmdOutput("ifconfig", QStringList() << device);
    //qDebug() << "wifi Status\n" << tmp;
    connection.insert("media", tmp.section("\tmedia: ",-1).section("\n",0,0).simplified());
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
  QStringList known = known_wifi_networks();
  QStringList lines = CmdOutput("ifconfig", QStringList() << device << "list" << "scan" ).split("\n");
  //qDebug() << "Got wifi scan:" << lines;
  QJsonObject out;
  QRegExp bssidRegex("([^:]{2}:){5}[^:]{2}");
  for(int i=1; i<lines.length(); i++){
    //Columns: [SSID, BSSID, Channel Number, Rate, Sig:Noise, INT, (rest are various capability codes)]
    QStringList columns = lines[i].split("  ", QString::SkipEmptyParts);
    if(columns.length() < 6){ continue; } //invalid line : capabilities are 7+ and all optional
    QJsonObject tmp;
    QString ssid = columns[0].simplified();
    if(bssidRegex.exactMatch(ssid) && !bssidRegex.exactMatch(columns[1]) ){
      //Got a missing ssid on this address. See this with access points that provide different channels from the same box
      // or from jokers that think this "hides" their wifi access point.
      ssid = ""; columns.prepend("");
    }
    //qDebug() << "Got Columns:" << columns;
    tmp.insert("ssid", ssid);
    tmp.insert("bssid", columns[1].simplified());
    tmp.insert("channel", columns[2].simplified().toInt());
    int sig = columns[4].section(":",0,0).simplified().toInt();
    int noise = columns[4].section(":",-1).simplified().toInt();
    int sigpercent = 2*qAbs(sig - noise); //quick and dirty percentage: 2x the difference in DB strength
    if(noise > sig){ sigpercent = 0; } //noise is greater than signal
    else if(sigpercent>100){ sigpercent = 100; }
    if(sigpercent<10){ tmp.insert("signal", "00"+QString::number(sigpercent)+"%"); }
    else if(sigpercent<100){ tmp.insert("signal", "0"+QString::number(sigpercent)+"%"); }
    else{ tmp.insert("signal", QString::number(sigpercent)+"%"); }

    tmp.insert("int", columns[5]);
    QStringList cap;
    for(int j=6; j<columns.length(); j++){ cap << columns[j].split(" ", QString::SkipEmptyParts); }
    tmp.insert("capabilities", QJsonArray::fromStringList(cap));
    tmp.insert("is_locked", (cap.contains("WPA") || cap.contains("RSN") ));
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
  //qDebug() << "Wifi:" << out;
  return out;
}

QStringList Networking::known_wifi_networks(){
  static QStringList idcache;
  static QDateTime cachecheck;
  QDateTime lastMod = QFileInfo("/etc/wpa_supplicant").lastModified();
  if(cachecheck.isNull() || (lastMod > cachecheck) ){
    //Need to re-read the file to assemble the list of ID's.
    idcache.clear();
    QStringList contents = readFile("/etc/wpa_supplicant.conf");
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
  //qDebug() << "Known wifi networks:" << idcache;
  return idcache;
}

bool Networking::save_wifi_network(QJsonObject obj, bool clearonly){
  static QStringList exclude_quotes;
  if(exclude_quotes.isEmpty()){ exclude_quotes << "key_mgmt" << "eap" << "proto"; }
  QString id = obj.value("bssid").toString();
  if(id.isEmpty()){ id = obj.value("ssid").toString(); }
  if(id.isEmpty()){ return false; }
  QStringList contents = readFile("/etc/wpa_supplicant.conf");
  if(contents.isEmpty()){ contents << "ctrl_interface=/var/run/wpa_supplicant"; }
  if(known_wifi_networks().contains("id")){
    //Need to remove the currently-saved entry first
    QStringList tmp = contents.join("\n").split("{");
    for(int i=0; i<tmp.length(); i++){
      if(tmp[i].contains("=\""+id+"\"") ){
        tmp.removeAt(i);
        if(i>=tmp.length()){
          //Need to remove the last piece of the network= section
          tmp[i-1] = tmp[i-1].section("network=",0,-2);
        }else{ i--; }
      }
    }
    contents = tmp.join("{").split("\n");
  }
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
  return writeFile("/etc/wpa_supplicant.conf", contents);
}

bool Networking::connect_to_wifi_network(QString device, QString id){
  //ssid or bssid

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
    qDebug() << "Coule not read file:" << path;
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

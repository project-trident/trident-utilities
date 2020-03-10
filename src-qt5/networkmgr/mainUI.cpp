#include "mainUI.h"
#include "ui_mainUI.h"

#include <QJsonDocument>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

// === PUBLIC ===
mainUI::mainUI() : QMainWindow(), ui(new Ui::mainUI()){
  ui->setupUi(this);
  NETWORK = new Networking(this);
  connect(NETWORK, SIGNAL(starting_wifi_scan()), this, SLOT(starting_wifi_scan()) );
  connect(NETWORK, SIGNAL(finished_wifi_scan()), this, SLOT(finished_wifi_scan()) );
  connect(NETWORK, SIGNAL(new_wifi_scan_results()), this, SLOT(updateWifiConnections()) );
  connect(ui->tool_wifi_refresh, SIGNAL(clicked()), NETWORK, SLOT(startWifiScan()) );

  page_group = new QActionGroup(this);
    page_group->setExclusive(true);
    page_group->addAction(ui->actionConnections);
    page_group->addAction(ui->actionFirewall);
    page_group->addAction(ui->actionVPN);
    page_group->addAction(ui->actionDNS);
  connect(page_group, SIGNAL(triggered(QAction*)), this, SLOT(pageChange(QAction*)) );

  ui->tabs_conn->setCurrentWidget(ui->tab_conn_status);
  connect(ui->combo_conn_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(updateConnectionInfo()) );
  connect(ui->tool_conn_status_refresh, SIGNAL(clicked()), this, SLOT(updateConnectionInfo()) );

}

mainUI::~mainUI(){

}

// === PUBLIC SLOTS ===
void mainUI::newInputs(QStringList args){
  if(args.isEmpty() || args.contains("-connections") ){
    ui->actionConnections->trigger();
  }else if(args.contains("-firewall")){
    ui->actionFirewall->trigger();
  }else if(args.contains("-vpn")){
    ui->actionVPN->trigger();
  }else if(args.contains("-dns")){
    ui->actionDNS->trigger();
  }

}

// === PRIVATE ===
//Initial page loading (on page change)
void mainUI::updateConnections(){
  qDebug() << "Update Connections";
  QStringList devs = NETWORK->list_devices();
  static QStringList lastdevs;
  if(devs == lastdevs){ return; } //no change
  lastdevs = devs;
  devs.sort();
  QString cdev = ui->combo_conn_devices->currentText();
  ui->combo_conn_devices->clear();
  bool haswifi = false;
  int index = -1;
  for(int i=0; i<devs.length(); i++){
    if(devs[i].isEmpty()){ continue; }
    bool is_wifi = devs[i].startsWith("wl");
    ui->combo_conn_devices->addItem(QIcon::fromTheme( is_wifi ? "network-wireless" : "network-wired-activated"), devs[i]);
    haswifi = (haswifi || is_wifi);
    if(cdev == devs[i]){ index = i; }
    else if(cdev.isEmpty() && is_wifi){ index = i; }
  }
  ui->tab_conn_wifi->setEnabled(haswifi);
  if(index>=0){ ui->combo_conn_devices->setCurrentIndex(index); }
  if(cdev.isEmpty() && haswifi){
    //First time loading the device list - go ahead and start a wifi scan in the background
    QTimer::singleShot(500, NETWORK, SLOT(startWifiScan()));
  }
}

void mainUI::updateFirewall(){

}
void mainUI::updateVPN(){

}
void mainUI::updateDNS(){

}

// === PRIVATE SLOTS ===
void mainUI::pageChange(QAction *triggered){
  if(triggered == ui->actionConnections){
	ui->stackedWidget->setCurrentWidget(ui->page_connections);
	updateConnections();
        loadStaticProfiles();
  }else if(triggered == ui->actionFirewall ){
	ui->stackedWidget->setCurrentWidget(ui->page_firewall);
	updateFirewall();
  }else if(triggered == ui->actionDNS ){
	ui->stackedWidget->setCurrentWidget(ui->page_dns);
	updateDNS();
  }else if(triggered == ui->actionVPN ){
	ui->stackedWidget->setCurrentWidget(ui->page_vpn);
	updateVPN();
  }
}

void mainUI::updateConnectionInfo(){
  QString cdev = ui->combo_conn_devices->currentText();
  if(cdev.isEmpty()){ return; } //no devices loaded (yet)
  //QJsonObject config = NETWORK->list_config();
  QJsonObject status = NETWORK->current_info(cdev);
  //qDebug() << "Got Info:" << cdev << config;

  // Current status display
  bool running = status.value("is_running").toBool();
  ui->tool_dev_start->setVisible(!running);
  ui->tool_dev_restart->setVisible(running);
  ui->tool_dev_stop->setVisible(running);
  //Assemble the output text for the current status
  QString skel = "<p><h3><b>%1</b></h3>%2</p>";
  QStringList textblocks;
  if(status.contains("wifi")){
    QJsonObject wifi = status.value("wifi").toObject();
    QStringList info;
      info << QString(tr("Access Point: %1")).arg("<i>"+wifi.value("ssid").toString()+"</i>");
      info << QString(tr("Security: %1")).arg("<i>"+wifi.value("key_mgmt").toString()+"</i>");
    textblocks << skel.arg( tr("Wireless Status"), "<ul><li>"+info.join("</li><li>")+"</li></ul>");
  }
/*  if(status.contains("lan")){
    QJsonObject lan = status.value("lan").toObject();
    QStringList info;
      info << QString(tr("Connection: %1")).arg("<i>"+lan.value("media").toString()+"</i>");
    textblocks << skel.arg( tr("Wired Status"), "<ul><li>"+info.join("</li><li>")+"</li></ul>");
  }*/
  if(status.contains("ipv4")){
    QStringList info;
      info << QString(tr("Address: %1")).arg("<i>"+status.value("ipv4").toString()+"</i>");
      info << QString(tr("Broadcast: %1")).arg("<i>"+status.value("ipv4_broadcast").toString()+"</i>");
      info << QString(tr("Netmask: %1")).arg("<i>"+status.value("ipv4_netmask").toString()+"</i>");
    textblocks << skel.arg( tr("IPv4"), "<ul><li>"+info.join("</li><li>")+"</li></ul>");
  }
  if(status.contains("ipv6")){
    QStringList info;
      info << QString(tr("Address: %1")).arg("<i>"+status.value("ipv6").toString()+"</i>");
      if(!status.value("ipv6_broadcast").toString().isEmpty()){ info << QString(tr("Broadcast: %1")).arg("<i>"+status.value("ipv6_broadcast").toString()+"</i>"); }
      info << QString(tr("Netmask: %1")).arg("<i>"+status.value("ipv6_netmask").toString()+"</i>");
    textblocks << skel.arg( tr("IPv6"), "<ul><li>"+info.join("</li><li>")+"</li></ul>");
  }
  QString state = tr("Inactive");
  bool auto_check = true;
  if(status.value("is_active").toBool()){ state = tr("Connected"); auto_check = false; }
  else if(status.value("is_running").toBool() && status.value("is_up").toBool()){
      state = tr("Waiting for connection");
  }
  textblocks.prepend( QString(tr("Current Status: %1")).arg("<i>"+state+"</i>") );
  //ui->text_conn_dev_status->setText(QJsonDocument(status).toJson(QJsonDocument::Indented));
  ui->text_conn_dev_status->setText(textblocks.join(""));
  if(auto_check){
    //Check again in a half second - waiting for status change
    QTimer::singleShot(500, this, SLOT(updateConnectionInfo()) );
  }
  if(status.contains("wifi")){
    //Resync the status of the wifi scan as well (does not start a new scan)
    QTimer::singleShot(10, this, SLOT(updateWifiConnections()) );
  }
}

QTreeWidgetItem* mainUI::generateWifi_item(QJsonObject obj, QJsonObject active){
  QTreeWidgetItem *it = new QTreeWidgetItem();
  QString ssid = obj.value("ssid").toString();
  it->setData(0, Qt::UserRole, obj.value("bssid").toString() );
  it->setText(1, obj.value("signal").toString() );
  it->setText(2, ssid.isEmpty() ? "[Hidden] "+obj.value("bssid").toString() : ssid);
  if(obj.value("is_locked").toBool()){ it->setIcon(0, QIcon::fromTheme("password")); }
  //See if the network is currently known
  bool is_known = NETWORK->is_known(obj);
  obj.insert("is_known", is_known);
  if(is_known){ it->setIcon(2, QIcon::fromTheme("tag")); }
  //See if the network is currently active
  bool is_active = Networking::sameNetwork(obj, active);
  obj.insert("is_active", is_active);
  if(is_active){it->setIcon(1, QIcon::fromTheme("network-wireless")); }
  //Save the current info object to the item
  it->setData(1, Qt::UserRole, obj);
  return it;
}

void mainUI::updateWifiConnections(){
  QJsonObject scan = NETWORK->wifi_scan_results();
  QJsonObject activeWifi = NETWORK->active_wifi_network();
  //Make sure the current item stays selected if possible
  QString citem = "";
  if(ui->tree_wifi_networks->currentItem() != 0){
    citem = ui->tree_wifi_networks->currentItem()->data(0, Qt::UserRole).toString();
  }
  //Now update the tree widget
  ui->tree_wifi_networks->clear();
  QStringList ssids = scan.keys();
  QTreeWidgetItem *sel = 0;
  for(int i=0; i<ssids.length(); i++){
    QJsonValue info = scan.value(ssids[i]);
    QTreeWidgetItem *it = 0;

    if(info.isObject()){
      it = generateWifi_item(info.toObject(), activeWifi);
      if(it->data(0, Qt::UserRole).toString() == citem){ sel = it; }

    }else if(info.isArray()){
      QJsonArray arr = info.toArray();
      it = new QTreeWidgetItem();
      it->setText(1, "---");
      it->setText(2, ssids[i].isEmpty() ? "[Hidden]" : ssids[i]);
      it->setData(0, Qt::UserRole, ssids[i]); //Mesh network - identify with the ssid instead of bssid
      for(int a=0; a<arr.count(); a++){
        QTreeWidgetItem *sit = generateWifi_item(arr[a].toObject(), activeWifi);
        if(sit->data(0, Qt::UserRole).toString() == citem){ sel = sit; }
        it->addChild(sit);
      }

    }
    ui->tree_wifi_networks->addTopLevelItem(it);
  }

  ui->tree_wifi_networks->sortItems(1, Qt::DescendingOrder);
  ui->tree_wifi_networks->resizeColumnToContents(0);
  ui->tree_wifi_networks->resizeColumnToContents(1);
  //Now re-select the item before the refresh if possible
  if(sel!=0){
    ui->tree_wifi_networks->setCurrentItem(sel);
    ui->tree_wifi_networks->scrollToItem(sel);
  }else{
    on_tree_wifi_networks_currentItemChanged(0);
  }
}

void mainUI::on_tree_wifi_networks_currentItemChanged(QTreeWidgetItem *it){
  bool is_known = false;
  bool is_active = false;
  bool is_group = false;
  if(it!=0){
    is_group = (it->childCount() > 0);
    QJsonObject info = it->data(1, Qt::UserRole).toJsonObject();
    is_known = info.value("is_known").toBool(false);
    is_active = info.value("is_active").toBool(false);
  }else{ is_group = true; } //just hide everything if no item selected
  ui->tool_forget_wifi->setVisible(is_known && !is_group);
  ui->tool_connect_wifi->setVisible(!is_active && !is_group);
}

void mainUI::on_tool_dev_restart_clicked(){
  QString cdev = ui->combo_conn_devices->currentText();
  if(cdev.isEmpty()){ return; } //no devices loaded (yet)
  NETWORK->setDeviceState(cdev, Networking::StateRestart);
  QTimer::singleShot(500, this, SLOT(updateConnectionInfo()));
  //Send a couple automatic status updates 5 & 10 seconds later
  QTimer::singleShot(5000, this, SLOT(updateConnectionInfo()));
  QTimer::singleShot(10000, this, SLOT(updateConnectionInfo()));
}

void mainUI::on_tool_dev_start_clicked(){
  QString cdev = ui->combo_conn_devices->currentText();
  if(cdev.isEmpty()){ return; } //no devices loaded (yet)
  NETWORK->setDeviceState(cdev, Networking::StateRunning);
  QTimer::singleShot(500, this, SLOT(updateConnectionInfo()));
  //Send a couple automatic status updates 5 & 10 seconds later
  QTimer::singleShot(5000, this, SLOT(updateConnectionInfo()));
  QTimer::singleShot(10000, this, SLOT(updateConnectionInfo()));
}

void mainUI::on_tool_dev_stop_clicked(){
  QString cdev = ui->combo_conn_devices->currentText();
  if(cdev.isEmpty()){ return; } //no devices loaded (yet)
  NETWORK->setDeviceState(cdev, Networking::StateStopped);
  QTimer::singleShot(500, this, SLOT(updateConnectionInfo()));
}

void mainUI::on_tool_forget_wifi_clicked(){
  QTreeWidgetItem *curit = ui->tree_wifi_networks->currentItem();
  if(curit == 0){ return; } //nothing selected
  QJsonObject info = curit->data(1, Qt::UserRole).toJsonObject();
  if(info.isEmpty()){ return; } // nothing to do
  //Get the known network entry which matches
  QJsonArray known = NETWORK->known_wifi_networks();
  for(int i=0; i<known.count(); i++){
    if( Networking::sameNetwork(known[i].toObject(), info) ){
      if( NETWORK->save_wifi_network(known[i].toObject(), true) ){
        QTimer::singleShot(50, this, SLOT(updateWifiConnections()) );
      }else{
        QMessageBox::warning(this, tr("Error"), QString(tr("Could not forget network settings: %1")).arg(info.value("ssid").toString()) );
      }
      break; //found network entry
    }
  }
}

void mainUI::on_tool_connect_wifi_clicked(){
  QTreeWidgetItem *curit = ui->tree_wifi_networks->currentItem();
  if(curit == 0){ return; } //nothing selected
  QJsonObject info = curit->data(1, Qt::UserRole).toJsonObject();
  bool part_of_group = (curit->parent() !=0);
  QString id = curit->data(0, Qt::UserRole).toString();
  if(info.isEmpty() || id.isEmpty()){ return; } //nothing selected
  //qDebug() << "Connect to wifi:" << info;
  if(NETWORK->is_known(info)){
    bool ok = NETWORK->connect_to_wifi_network(id); //just connect to this known network
    if(!ok){
      QMessageBox::warning(this, tr("Error"), QString(tr("Could not connect to network: %1")).arg(info.value("ssid").toString()) );
    }
  }else{
    //See if we need to save connection info first
    bool secure = info.value("is_locked").toBool();
    if(secure){
      QString psk = QInputDialog::getText(this, tr("Wifi Passphrase"), tr("Enter the wifi access point passphrase"), QLineEdit::Password);
      if(psk.isEmpty()){ return; } //cancelled
      info.insert("psk",psk);
    }
    if(part_of_group){
      //Ask whether to save the entire group, or just that one access point
      bool roam = (QMessageBox::Yes == QMessageBox::question(this, tr("Roam group?"), tr("Do you want to roam between access points for this network?")) );
      if(roam){ info.remove("bssid"); } //Just use the generic ssid
    }
    NETWORK->save_wifi_network(info, false);
  }
  ui->tabs_conn->setCurrentWidget(ui->tab_conn_status);
  QTimer::singleShot(50, this, SLOT(updateConnectionInfo()) );
}

void mainUI::starting_wifi_scan(){
  ui->tool_wifi_refresh->setEnabled(false);
}

void mainUI::finished_wifi_scan(){
  ui->tool_wifi_refresh->setEnabled(true);
}

void mainUI::loadStaticProfiles(){

  on_combo_conn_static_profile_currentIndexChanged(-1);
}

void mainUI::on_tool_conn_profile_apply_clicked(){
  QJsonObject out;
  for(int i=0; i<ui->combo_conn_static_profile->count(); i++){
    QJsonObject obj = ui->combo_conn_static_profile->itemData(i).toJsonObject();
    out.insert(obj.value("profile").toString(), obj);
  }
  bool ok = NETWORK->set_config(out);
  if(!ok){
    QMessageBox::warning(this, tr("Error"), tr("Could not save static IP settings!"));
  }
}

void mainUI::on_tool_conn_new_profile_clicked(){
  QString profile = QInputDialog::getText(this, tr("New static profile"), tr("Sentinal IP address:"));
  if(profile.isEmpty()){ return; } //cancelled
  //Check if it is an IPv4 address

  //Check if it already exists as a profile

  //Add it to the profile list
  QJsonObject obj;
  obj.insert("profile", profile);
  ui->combo_conn_static_profile->addItem(profile, obj);
}

void mainUI::on_tool_conn_remove_profile_clicked(){
  int index = ui->combo_conn_static_profile->currentIndex();
  if(index>=0){ ui->combo_conn_static_profile->removeItem(index); }
}

void mainUI::on_combo_conn_static_profile_currentIndexChanged(int){
  int index = ui->combo_conn_static_profile->currentIndex();
  ui->tool_conn_remove_profile->setVisible(index>=0);
  ui->line_static_v4_address->setEnabled(index>=0);
  ui->line_static_v4_gateway->setEnabled(index>=0);
  if( index<0 ){
    //No profiles available
    ui->line_static_v4_address->clear();
    ui->line_static_v4_gateway->clear();
  }else{
    QJsonObject info = ui->combo_conn_static_profile->currentData().toJsonObject();
    ui->line_static_v4_address->setText(info.value("ip_address").toText());
    ui->line_static_v4_gateway->clear(info.value("routers").toText());
  }
}

void mainUI::on_line_static_v4_address_textEdited(const QString &text){
  int index = ui->combo_conn_static_profile->currentIndex();
  if(index < 0){ return; } //nothing to do
  QJsonObject info = ui->combo_conn_static_profile->currentData().toJsonObject();
    info.insert("ip_address", ui->line_static_v4_address->text());
  ui->combo_conn_static_profile->setItemData(index, info);
}

void mainUI::on_line_static_v4_gateway_textEdited(const QString &text){
  int index = ui->combo_conn_static_profile->currentIndex();
  if(index < 0){ return; } //nothing to do
  QJsonObject info = ui->combo_conn_static_profile->currentData().toJsonObject();
    info.insert("routers", ui->line_static_v4_address->text());
  ui->combo_conn_static_profile->setItemData(index, info);
}

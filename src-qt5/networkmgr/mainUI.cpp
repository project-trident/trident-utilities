#include "mainUI.h"
#include "ui_mainUI.h"

#include <QJsonDocument>
#include <QDebug>
#include <QMessageBox>

// === PUBLIC ===
mainUI::mainUI() : QMainWindow(), ui(new Ui::mainUI()){
  ui->setupUi(this);
  NETWORK = new Networking(this);
  page_group = new QActionGroup(this);
    page_group->setExclusive(true);
    page_group->addAction(ui->actionConnections);
    page_group->addAction(ui->actionFirewall);
    page_group->addAction(ui->actionVPN);
    page_group->addAction(ui->actionDNS);
  connect(page_group, SIGNAL(triggered(QAction*)), this, SLOT(pageChange(QAction*)) );


  connect(ui->combo_conn_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(updateConnectionInfo()) );
  connect(ui->tool_conn_status_refresh, SIGNAL(clicked()), this, SLOT(updateConnectionInfo()) );
  connect(ui->tool_wifi_refresh, SIGNAL(clicked()), this, SLOT(updateWifiConnections()) );
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
  QStringList devs = NETWORK->list_devices();
  devs.sort();
  QString cdev = ui->combo_conn_devices->currentText();
  ui->combo_conn_devices->clear();
  for(int i=devs.length()-1; i>=0; i--){
    ui->combo_conn_devices->addItem(QIcon::fromTheme( devs[i].startsWith("wlan") ? "network-wireless" : "network-wired-activated"), devs[i]);
  }
  int index = devs.indexOf(cdev);
  if(index>=0){ ui->combo_conn_devices->setCurrentIndex( index); }
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
  QJsonObject config = NETWORK->list_config(cdev);
  QJsonObject status = NETWORK->current_info(cdev);
  //qDebug() << "Got Info:" << cdev << config;
  //Adjust the tabs based on type of device
  if(config.value("network_type").toString()=="wifi"){
    if(ui->tabs_conn->count()<3){
      ui->tabs_conn->addTab(ui->tab_conn_wifi, QIcon::fromTheme("network-wireless"), tr("Wifi Networks"));
    }
    ui->tab_conn_wifi->setEnabled( status.value("is_up").toBool());
    QTimer::singleShot(1000, this, SLOT(updateWifiConnections()) );
  }else if(ui->tabs_conn->count()==3){
      ui->tabs_conn->removeTab(2);
  }
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
      info << QString(tr("Security: %1")).arg("<i>"+wifi.value("authmode").toString()+"</i>");
      info << QString(tr("Connection: %1")).arg("<i>"+wifi.value("media").toString()+"</i>");
    textblocks << skel.arg( tr("Wireless Status"), "<ul><li>"+info.join("</li><li>")+"</li></ul>");
  }
  if(status.contains("lan")){
    QJsonObject lan = status.value("lan").toObject();
    QStringList info;
      info << QString(tr("Connection: %1")).arg("<i>"+lan.value("media").toString()+"</i>");
    textblocks << skel.arg( tr("Wired Status"), "<ul><li>"+info.join("</li><li>")+"</li></ul>");
  }
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
      info << QString(tr("Broadcast: %1")).arg("<i>"+status.value("ipv6_broadcast").toString()+"</i>");
      info << QString(tr("Netmask: %1")).arg("<i>"+status.value("ipv6_netmask").toString()+"</i>");
    textblocks << skel.arg( tr("IPv6"), "<ul><li>"+info.join("</li><br><li>")+"</li></ul>");
  }
  QString state = tr("Inactive");
  if(textblocks.isEmpty()){
    if(status.value("is_running").toBool() && status.value("is_up").toBool()){
      state = tr("Waiting for connection");
    }
  }else{ state = tr("Connected"); }
  textblocks.prepend( QString(tr("Current Status: %1")).arg("<i>"+state+"</i>") );
  //ui->text_conn_dev_status->setText(QJsonDocument(status).toJson(QJsonDocument::Indented));
  ui->text_conn_dev_status->setText(textblocks.join(""));
  //Current config settings
  if(config.value("network_config").toString()=="dhcp"){
    ui->radio_conn_dev_dhcp->setChecked(true);
    ui->group_conn_dev_static->setChecked(false);
  }else{
    ui->radio_conn_dev_dhcp->setChecked(false);
    ui->group_conn_dev_static->setChecked(true);
  }
  ui->line_static_v4_address->setText(config.value("ipv4_address").toString());
  ui->line_static_v4_gateway->setText(config.value("ipv4_gateway").toString());
  ui->line_static_v4_netmask->setText(config.value("ipv4_netmask").toString());
  ui->line_static_v6_address->setText(config.value("ipv6_address").toString());
  ui->line_static_v6_gateway->setText(config.value("ipv6_gateway").toString());
  ui->line_static_v6_netmask->setText(config.value("ipv6_netmask").toString());
}

inline QTreeWidgetItem* generateWifi_item(QJsonObject obj){
  QTreeWidgetItem *it = new QTreeWidgetItem();
  QString ssid = obj.value("ssid").toString();
  it->setData(0, Qt::UserRole, obj.value("bssid").toString() );
  it->setData(1, Qt::UserRole, obj);
  it->setText(0, obj.value("signal").toString() );
  it->setText(1, ssid.isEmpty() ? "[Hidden] "+obj.value("bssid").toString() : ssid);
  if(obj.value("is_locked").toBool()){ it->setIcon(0, QIcon::fromTheme("password")); }
  if(obj.value("is_known").toBool()){ it->setIcon(1, QIcon::fromTheme("user_auth")); }
  return it;
}

void mainUI::updateWifiConnections(){
  QString cdev = ui->combo_conn_devices->currentText();
  if(cdev.isEmpty() || !cdev.startsWith("wlan") ){ return; }
  QJsonObject scan = NETWORK->scan_wifi_networks(cdev);
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
      it = generateWifi_item(info.toObject());
      if(it->data(0, Qt::UserRole).toString() == citem){ sel = it; }

    }else if(info.isArray()){
      QJsonArray arr = info.toArray();
      it = new QTreeWidgetItem();
      it->setText(1, ssids[i].isEmpty() ? "[Hidden]" : ssids[i]);
      it->setData(0, Qt::UserRole, ssids[i]); //Mesh network - identify with the ssid instead of bssid
      for(int a=0; a<arr.count(); a++){
        QTreeWidgetItem *sit = generateWifi_item(arr[a].toObject());
        if(sit->data(0, Qt::UserRole).toString() == citem){ sel = sit; }
        it->addChild(sit);
      }

    }
    ui->tree_wifi_networks->addTopLevelItem(it);
  }

  ui->tree_wifi_networks->sortItems(0, Qt::DescendingOrder);
  ui->tree_wifi_networks->resizeColumnToContents(0);
  //Now re-select the item before the refresh if possible
  if(sel!=0){
    ui->tree_wifi_networks->setCurrentItem(sel);
    ui->tree_wifi_networks->scrollToItem(sel);
  }
}

void mainUI::on_radio_conn_dev_dhcp_toggled(bool on){
  ui->group_conn_dev_static->setChecked(!on);
}

void mainUI::on_group_conn_dev_static_clicked(bool on){
  ui->radio_conn_dev_dhcp->setChecked(!on);
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
  if( NETWORK->save_wifi_network(info, true) ){
    QTimer::singleShot(1000, this, SLOT(updateWifiConnections()) );
  }else{
    QMessageBox::warning(this, tr("Error"), QString(tr("Could not forget network settings: %1")).arg(info.value("ssid").toString()) );
  }
}

void mainUI::on_tool_connect_wifi_clicked(){
  QTreeWidgetItem *curit = ui->tree_wifi_networks->currentItem();
  if(curit == 0){ return; } //nothing selected
  QJsonObject info = curit->data(1, Qt::UserRole).toJsonObject();
  QString id = curit->data(0, Qt::UserRole).toString();
  if(info.isEmpty()){ return; } //nothing selected
  QString cdev = ui->combo_conn_devices->currentText();
  if(cdev.isEmpty()){ return; } //no devices loaded (yet)
  if(!info.value("is_known").toBool()){
     //See if we need to save connection info first
     bool secure = info.value("is_locked").toBool();
     
  }
  NETWORK->connect_to_wifi_network(cdev, id);

}


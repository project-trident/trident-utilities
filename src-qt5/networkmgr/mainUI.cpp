#include "mainUI.h"
#include "ui_mainUI.h"

#include <QDebug>

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

  //connect(ui->radio_conn_dev_dhcp, SIGNAL(toggled(bool)), ui->group_conn_dev_static, SLOT(setChecked(!bool)) );
  //connect(ui->group_conn_dev_static, SIGNAL(clicked()), ui->radio_conn_dev_dhcp, SLOT(toggle()) );


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
  QStringList devs = NETWORK->list_devices();
  QString cdev = ui->combo_conn_devices->currentText();
  ui->combo_conn_devices->clear();
  for(int i=0; i<devs.length(); i++){
    ui->combo_conn_devices->addItem(QIcon::fromTheme( devs[i].startsWith("wlan") ? "network-wireless" : "network-wired-activated"), devs[i]);
  }
  int index = devs.indexOf(cdev);
  if(index>=0){ ui->combo_conn_devices->setCurrentIndex( index); }
  //NETWORK->list_config(devs[i]);
  //qDebug() << devs[i] << NETWORK->current_info(devs[i]);

  //qDebug() << "Got Network Devices:" << NETWORK->list_devices();
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
  QJsonObject config = NETWORK->list_config(cdev);
  QJsonObject status = NETWORK->current_info(cdev);
  qDebug() << "Got Info:" << cdev << config << status;
  if(config.value("network_type").toString()=="wifi"){
    if(ui->tabs_conn->count()<3){
      ui->tabs_conn->addTab(ui->tab_conn_wifi, QIcon::fromTheme("network-wireless"), tr("Wifi Networks"));
    }
  }else{
    if(ui->tabs_conn->count()==3){
      ui->tabs_conn->removeTab(2);
    }
  }
  if(config.value("network_config").toString()=="dhcp"){
    ui->radio_conn_dev_dhcp->setChecked(true);
  }else{
    ui->group_conn_dev_static->setChecked(true);
  }
}

void mainUI::on_radio_conn_dev_dhcp_toggled(bool on){
  ui->group_conn_dev_static->setChecked(!on);
}

void mainUI::on_group_conn_dev_static_clicked(bool on){
  ui->radio_conn_dev_dhcp->setChecked(!on);
}

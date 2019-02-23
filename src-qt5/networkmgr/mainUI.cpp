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
}

mainUI::~mainUI(){

}

// === PUBLIC SLOTS ===
void mainUI::newInputs(QStringList args){
  if(args.isEmpty() || args.contains("-connections") ){
    ui->actionConnections->trigger();
  }
}

// === PRIVATE ===
//Initial page loading (on page change)
void mainUI::updateConnections(){
  qDebug() << "Got Network Devices:" << NETWORK->list_devices();
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

QT       += core gui widgets network x11extras

TARGET = trident-updatemgr
target.path = /usr/local/bin

TEMPLATE = app

SUBDIRS += susysup

SOURCES += main.cpp \
		mainUI.cpp \
		updateMgr.cpp \
		beMgr.cpp \
		SysTray.cpp

HEADERS  += mainUI.h \
		updateMgr.h \
		beMgr.h \
		SysTray.h

FORMS    += mainUI.ui

include($${PWD}/../../common/SingleApp.pri)

TRANSLATIONS =  i18n/tri-umgr_af.ts \
                i18n/tri-umgr_ar.ts \
                i18n/tri-umgr_az.ts \
                i18n/tri-umgr_bg.ts \
                i18n/tri-umgr_bn.ts \
                i18n/tri-umgr_bs.ts \
                i18n/tri-umgr_ca.ts \
                i18n/tri-umgr_cs.ts \
                i18n/tri-umgr_cy.ts \
                i18n/tri-umgr_da.ts \
                i18n/tri-umgr_de.ts \
                i18n/tri-umgr_el.ts \
                i18n/tri-umgr_en_GB.ts \
                i18n/tri-umgr_en_ZA.ts \
                i18n/tri-umgr_en_AU.ts \
                i18n/tri-umgr_es.ts \
                i18n/tri-umgr_et.ts \
                i18n/tri-umgr_eu.ts \
                i18n/tri-umgr_fa.ts \
                i18n/tri-umgr_fi.ts \
                i18n/tri-umgr_fr.ts \
                i18n/tri-umgr_fr_CA.ts \
                i18n/tri-umgr_gl.ts \
                i18n/tri-umgr_he.ts \
                i18n/tri-umgr_hi.ts \
                i18n/tri-umgr_hr.ts \
                i18n/tri-umgr_hu.ts \
                i18n/tri-umgr_id.ts \
                i18n/tri-umgr_is.ts \
                i18n/tri-umgr_it.ts \
                i18n/tri-umgr_ja.ts \
                i18n/tri-umgr_ka.ts \
                i18n/tri-umgr_ko.ts \
                i18n/tri-umgr_lt.ts \
                i18n/tri-umgr_lv.ts \
                i18n/tri-umgr_mk.ts \
                i18n/tri-umgr_mn.ts \
                i18n/tri-umgr_ms.ts \
                i18n/tri-umgr_mt.ts \
                i18n/tri-umgr_nb.ts \
                i18n/tri-umgr_nl.ts \
                i18n/tri-umgr_pa.ts \
                i18n/tri-umgr_pl.ts \
                i18n/tri-umgr_pt.ts \
                i18n/tri-umgr_pt_BR.ts \
                i18n/tri-umgr_ro.ts \
                i18n/tri-umgr_ru.ts \
                i18n/tri-umgr_sk.ts \
                i18n/tri-umgr_sl.ts \
                i18n/tri-umgr_sr.ts \
                i18n/tri-umgr_sv.ts \
                i18n/tri-umgr_sw.ts \
                i18n/tri-umgr_ta.ts \
                i18n/tri-umgr_tg.ts \
                i18n/tri-umgr_th.ts \
                i18n/tri-umgr_tr.ts \
                i18n/tri-umgr_uk.ts \
                i18n/tri-umgr_uz.ts \
                i18n/tri-umgr_vi.ts \
                i18n/tri-umgr_zh_CN.ts \
                i18n/tri-umgr_zh_HK.ts \
                i18n/tri-umgr_zh_TW.ts \
                i18n/tri-umgr_zu.ts

dotrans.path=/usr/local/share/trident-updatemgr/i18n/
dotrans.extra=cd $$PWD/i18n && lrelease -nounfinished *.ts && cp *.qm $(INSTALL_ROOT)/usr/local/share/trident-updatemgr/i18n/

#Some conf to redirect intermediate stuff in separate dirs
UI_DIR=./.build/ui/
MOC_DIR=./.build/moc/
OBJECTS_DIR=./.build/obj
RCC_DIR=./.build/rcc
QMAKE_DISTCLEAN += -r ./.build

INSTALLS += target dotrans

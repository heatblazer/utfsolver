TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES = \
   $$PWD/main.cpp \

INCLUDEPATH =

#DEFINES =


RES_DIR = $${OUT_PWD}
APPDATA =$${_PRO_FILE_PWD_}/textdata


DISTFILES += \
    textdata/data.txt \
    textdata/data_bigmix.txt \
    textdata/data_err.txt \
    textdata/data_mix.txt \
    textdata/data_noutf.txt \
    textdata/data_ok.txt \
    textdata/output99test.txt \
    readme.md \
    textdata/test100.txt \
    textdata/test99.txt \
    textdata/testcase.txt \
    textdata/testcase_output.txt


win32{
    system(cp $${APPDATA} $${OUT_PWD})
} else {
    system(cp -rvf $${APPDATA} $${OUT_PWD})
}

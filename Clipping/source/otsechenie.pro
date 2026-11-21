QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cohensutherland.cpp \
    main.cpp \
    mainwindow.cpp \
    quadtree.cpp \
    scenepainter.cpp \
    stresstest.cpp \
    weileratherton.cpp

HEADERS += \
    cohensutherland.h \
    geom.h \
    mainwindow.h \
    quadtree.h \
    scenepainter.h \
    stresstest.h \
    weileratherton.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

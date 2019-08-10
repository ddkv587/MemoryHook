CC=g++
CFLAGS= -Wall -g3 -O0 -D__ASSERT__ #-D_DEBUG
# CC=${QNX_HOST}/usr/bin/qcc
# CFLAGS= -Vgcc_ntox86_64 -Wall -g3 -O0 -DOS_QNX #-D_DEBUG #-D__ASSERT__
RM=rm -rf
MV=mv
MKDIR=mkdir -p
#TARGET=libSocket.so libPreLoad.so libTestLibrary.so MemCheck demo
TARGET=libPreLoad.so libTestLibrary.so demo
TARGET_DIR=target

LIBS        := -lm -ldl
#LIBS_DIR	:= -L${QNX_TARGET}/x86_64/lib -L${QNX_TARGET}/x86_64/usr/lib

all: $(TARGET)

demo: main.cpp
	$(CC) -O3 -g0  $^ -o $@ -ldl -lpthread
	#$(MV) $@ $(TARGET_DIR)

libPreLoad.so: PreloadMemory.cpp CMemoryManager.cpp
	$(CC) $(CFLAGS) --shared -fPIC $^ -o $@ $(LIBS_DIR) $(LIBS)
	#$(MV) $@ $(TARGET_DIR)

libTestLibrary.so: demo/TestLibrary.cpp
	$(CC) $(CFLAGS) -shared -fPIC $^ -o $@ $(LIBS_DIR) $(LIBS)
	#$(MV) $@ $(TARGET_DIR)

clean:
	$(RM) $(TARGET)
	$(RM) libPreLoad.so
	$(RM) core err PreLoad

.PHONY:clean

#������·��,��Ŀ·��,�������·��(����make��Ϊ��������)
export PLAT_PATH=arm

export WORKSPACE_PATH=$(LINUX_PLAT_COMPONENT_LINUX)



export C_DEV_LCD = -DLCD_320_240
export OUTPUT_PATH=$(LINUX_PLAT_COMPONENT_LINUX)/out/LINUX_320_240
export PREFIX_PATH=$(OUTPUT_PATH)

export TARGET_PATH=$(OUTPUT_PATH)
export LIBRARY_PATH=$(OUTPUT_PATH)/lib
export OBJECT_PATH=$(OUTPUT_PATH)/obj
export DEPEND_PATH=$(OUTPUT_PATH)/dep

# ������


export CC=arm-oe-linux-gnueabi-gcc
export CXX=arm-oe-linux-gnueabi-g++
export CAR=arm-none-linux-gnueabi-ar

#֧���ļ�����չ��
EXT=.c .cpp

export CFLAGS=-march=armv7-a -mfloat-abi=softfp -mfpu=neon  -O2 -fexpensive-optimizations -frename-registers -fomit-frame-pointer -fPIC -DOS_LINUX $(C_DEV_LCD) -DM90_DEV
export CXXFLAGS=-march=armv7-a -mfloat-abi=softfp -mfpu=neon  -O2 -fexpensive-optimizations -frename-registers -fomit-frame-pointer -fPIC -DOS_LINUX $(C_DEV_LCD) -DM90_DEV

export LDFLAGS=-Xlinker --unresolved-symbols=report-all

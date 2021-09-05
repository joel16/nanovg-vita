cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(nanovg-vita)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "nanovg-vita")
set(VITA_TITLEID  "NANOVGPSV")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -Wall -std=gnu++17")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")

add_definitions(-D__psp2__)

include_directories(
  include
  libs/nanovg/src
  libs/nanovg/example
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libs
)

add_executable(${PROJECT_NAME}
  libs/nanovg/src/nanovg.c
  libs/nanovg/example/demo.c
  source/main.cpp
)

target_link_libraries(${PROJECT_NAME}
  libgpu_es4_ext_stub_weak
  libGLESv2_stub_weak
  libIMGEGL_stub_weak
  SceCtrl_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE data/libgpu_es4_ext.suprx libgpu_es4_ext.suprx
  FILE data/libIMGEGL.suprx libIMGEGL.suprx
  FILE data/libGLESv2.suprx module/libGLESv2.suprx
  FILE data/libpvrPSP2_WSEGL.suprx module/libpvrPSP2_WSEGL.suprx
  FILE libs/nanovg/example/images/image1.jpg example/images/image1.jpg
  FILE libs/nanovg/example/images/image2.jpg example/images/image2.jpg
  FILE libs/nanovg/example/images/image3.jpg example/images/image3.jpg
  FILE libs/nanovg/example/images/image4.jpg example/images/image4.jpg
  FILE libs/nanovg/example/images/image5.jpg example/images/image5.jpg
  FILE libs/nanovg/example/images/image6.jpg example/images/image6.jpg
  FILE libs/nanovg/example/images/image7.jpg example/images/image7.jpg
  FILE libs/nanovg/example/images/image8.jpg example/images/image8.jpg
  FILE libs/nanovg/example/images/image9.jpg example/images/image9.jpg
  FILE libs/nanovg/example/images/image10.jpg example/images/image10.jpg
  FILE libs/nanovg/example/images/image11.jpg example/images/image11.jpg
  FILE libs/nanovg/example/images/image12.jpg example/images/image12.jpg
  
  FILE libs/nanovg/example/entypo.ttf example/entypo.ttf
  FILE libs/nanovg/example/Roboto-Regular.ttf example/Roboto-Regular.ttf
  FILE libs/nanovg/example/Roboto-Bold.ttf example/Roboto-Bold.ttf
  FILE libs/nanovg/example/NotoEmoji-Regular.ttf example/NotoEmoji-Regular.ttf
)

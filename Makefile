export THEOS = $(HOME)/theos

# Classic iOS jailbreak tweak (rootful). Injected into the game through
# MobileSubstrate. Build with `make package`, or `make package install` to push
# straight to a device set up in $THEOS_DEVICE_IP.
TARGET := iphone:clang:latest:14.0
ARCHS = arm64 arm64e
DEBUG = 0
FINALPACKAGE = 1

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = Reveal

Reveal_FRAMEWORKS = UIKit Foundation Metal MetalKit QuartzCore

Reveal_CFLAGS = -I$(THEOS_PROJECT_DIR) -I$(THEOS_PROJECT_DIR)/src \
                -fobjc-arc \
                -Wall -Wno-deprecated-declarations \
                -fvisibility=hidden -fvisibility-inlines-hidden
Reveal_CXXFLAGS = -std=gnu++17
Reveal_CCFLAGS = -std=gnu++17

Reveal_FILES = src/Overlay/Loader.mm \
               src/Overlay/Overlay.mm \
               src/Render/Skeleton.cpp \
               src/Game/World.cpp \
               src/Game/Memory.cpp \
               imgui/imgui.cpp \
               imgui/imgui_draw.cpp \
               imgui/imgui_tables.cpp \
               imgui/imgui_widgets.cpp \
               imgui/imgui_impl_metal.mm

include $(THEOS_MAKE_PATH)/tweak.mk

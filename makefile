PROJECT_DIR := $(CURDIR)
PICO_SDK_PATH := $(PROJECT_DIR)/pico-sdk-2.3.1
BUILD_DIR := $(PROJECT_DIR)/build

.PHONY: compile-project clean

compile-project:
	git clone --branch 2.3.1 --depth 1 --recurse-submodules https://github.com/raspberrypi/pico-sdk.git pico-sdk-2.3.1
	cp "$(PICO_SDK_PATH)/external/pico_sdk_import.cmake" "$(PROJECT_DIR)/"
	cmake -S "$(PROJECT_DIR)" -B "$(BUILD_DIR)" -DPICO_SDK_PATH="$(PICO_SDK_PATH)" -DPICO_BOARD=pico_w
	cmake --build "$(BUILD_DIR)"

clean:
	rm -rf "$(PICO_SDK_PATH)"
	rm -rf "$(PROJECT_DIR)/pico_sdk_import.cmake"
	rm -rf "$(BUILD_DIR)"
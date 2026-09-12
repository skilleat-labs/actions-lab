# 개인 레포 실습용 — GitHub 무료 러너(ubuntu)에서 그대로 동작
#   make test   호스트 유닛테스트
#   make        build/app 생성 (아티팩트용 산출물)
CC ?= cc
WARN = -Wall -Wextra -std=c11 -Iinclude
BUILD ?= build
BUILD_TYPE ?= Release
ifeq ($(BUILD_TYPE),Debug)
  OPT = -Og -g3
else
  OPT = -O2
endif

all: $(BUILD)/app
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/app: | $(BUILD)
	$(CC) $(WARN) $(OPT) -o $(BUILD)/app src/main.c src/logic.c
	@echo "빌드 타입: $(BUILD_TYPE)"
test: | $(BUILD)
	$(CC) $(WARN) -o $(BUILD)/test tests/test_logic.c src/logic.c
	./$(BUILD)/test
clean:
	rm -rf $(BUILD)
.PHONY: all test clean

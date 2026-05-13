.PHONY: all build test clean run-server run-client run-editor help

BUILD_DIR := build
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug

help:
	@echo "Targets disponibles:"
	@echo "  build        Compila todo (cliente, servidor, editor, tests)"
	@echo "  test         Compila y ejecuta los tests"
	@echo "  run-server   Compila y ejecuta el servidor"
	@echo "  run-client   Compila y ejecuta el cliente"
	@echo "  run-editor   Compila y ejecuta el editor"
	@echo "  clean        Elimina el directorio build/"
	@echo "  all          clean + test"
	@echo "  install      Compila, testea e instala"

setup:
	bash scripts/install_deps.sh

build:
	mkdir -p $(BUILD_DIR)
	cmake -S . -B ./$(BUILD_DIR) $(CMAKE_FLAGS) $(EXTRA_GENERATE)
	cmake --build $(BUILD_DIR) $(EXTRA_COMPILE)

test: build
	./$(BUILD_DIR)/taller_tests

valgrind: build
	valgrind --leak-check=full --error-exitcode=1 ./$(BUILD_DIR)/taller_tests

install: setup test
	cmake --install $(BUILD_DIR) --prefix $(HOME)
	@echo "Binarios instalados en ~/.local/bin"
	@echo "Assets instalados en ~/.local/share/argentum"
	@echo "Config instalada en ~/.config/argentum"

run-server: build
	./$(BUILD_DIR)/taller_server 8080

run-client: build
	./$(BUILD_DIR)/taller_client 127.0.0.1 8080

run-editor: build
	./$(BUILD_DIR)/taller_editor

clean:
	rm -rf $(BUILD_DIR)/

all: clean test

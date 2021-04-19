SERVER_SRC_DIR := src

OBJ_DIR := build
BIN_DIR := bin

SERVER_EXE := $(BIN_DIR)/ServerRelay

SERVER_SOURCES := $(wildcard $(SERVER_SRC_DIR)/*.cpp)
SERVER_OBJECTS := $(patsubst $(SERVER_SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SERVER_SOURCES))
COMPILED_SERVER_OBJECTS := $(wildcard $(OBJ_DIR)/ServerRelay.o)

CPPFLAGS :=-std=c++17 -Iinclude -MMD -MP
CXXFLAGS :=-O0 -W -Wall -Wextra -Wno-unused-parameter -D_FILE_OFFSET_BITS=64
LDLIBS :=-Llib -lssl -ldl -lcrypto -lpthread

.PHONY: server

server: 
	+$(MAKE) $(SERVER_EXE)

$(SERVER_EXE): $(COMPILED_SERVER_OBJECTS) $(SERVER_OBJECTS) | $(BIN_DIR)
	$(CXX) $^ $(LDLIBS) -o $@

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SERVER_SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR) # The @ disables the echoing of the command


-include $(SERVER_OBJECTS:.o=.d)
-include $(CLIENT_OBJECTS:.o=.d)
-include $(CUBE_CLIENT_OBJECTS:.o=.d)
CC=clang++

SERVER_SRC_DIR := examples/server
CLIENT_SRC_DIR := examples/client
CUBE_CLIENT_SRC_DIR := examples/cube

OBJ_DIR := build
BIN_DIR := bin

SERVER_EXE := $(BIN_DIR)/SocketServer
CLIENT_EXE := $(BIN_DIR)/SocketClient
CUBE_EXE := $(BIN_DIR)/Cube


SERVER_SOURCES := $(wildcard $(SERVER_SRC_DIR)/*.cpp)
SERVER_OBJECTS := $(patsubst $(SERVER_SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SERVER_SOURCES))
COMPILED_SERVER_OBJECTS := $(wildcard $(OBJ_DIR)/Server.o)

CLIENT_SOURCES := $(wildcard $(CLIENT_SRC_DIR)/*.cpp)
CLIENT_OBJECTS := $(patsubst $(CLIENT_SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CLIENT_SOURCES))
COMPILED_CLIENT_OBJECTS := $(wildcard $(OBJ_DIR)/Client.o)

CUBE_CLIENT_SOURCES := $(wildcard $(CUBE_CLIENT_SRC_DIR)/*.cpp)
CUBE_CLIENT_OBJECTS := $(patsubst $(CUBE_CLIENT_SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CUBE_CLIENT_SOURCES))
COMPILED_CUBE_CLIENT_OBJECTS := $(wildcard $(OBJ_DIR)/Cube.o)

CPPFLAGS :=-std=c++17 -Iinclude -MMD -MP
CXXFLAGS :=-O0 -W -Wall -Wextra -Wno-unused-parameter -D_FILE_OFFSET_BITS=64
LDLIBS :=-Llib -lssl -lcrypto -lpthread

.PHONY: all

all: 
	+$(MAKE) $(SERVER_EXE)
	+$(MAKE) $(CLIENT_EXE)
	+$(MAKE) $(CUBE_EXE)

$(SERVER_EXE): $(COMPILED_SERVER_OBJECTS) $(SERVER_OBJECTS) | $(BIN_DIR)
	$(CC) $^ $(LDLIBS) -o $@

$(CLIENT_EXE): $(COMPILED_CLIENT_OBJECTS) $(CLIENT_OBJECTS) | $(BIN_DIR)
	$(CC) $^ $(LDLIBS) -o $@

$(CUBE_EXE): $(COMPILED_CUBE_CLIENT_OBJECTS) $(CUBE_CLIENT_OBJECTS) | $(BIN_DIR)
	$(CC) $^ $(LDLIBS) -o $@

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SERVER_SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CLIENT_SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CUBE_CLIENT_SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR) # The @ disables the echoing of the command


-include $(SERVER_OBJECTS:.o=.d)
-include $(CLIENT_OBJECTS:.o=.d)
-include $(CUBE_CLIENT_OBJECTS:.o=.d)
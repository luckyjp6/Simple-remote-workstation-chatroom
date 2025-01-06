CXX := g++
CXXFLAGS := -std=c++17 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_PARAMS := $(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_PARAMS := $(addprefix -L , $(CXX_LIB_DIRS))

SRC_DIR := src/codes
OBJ_DIR := build

SOURCE := worm_server client_proc shm_manager
OBJECTS := $(OBJ_DIR)/main.o $(OBJ_DIR)/$(SOURCE:%.cpp=$(OBJ_DIR)/%.o)
EXECUTABLE := server

all: $(OBJ_DIR) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(SRC_DIR)/util.hpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/util.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# $(OBJ_DIR)/main.o: $(SRC_DIR)/main.cpp $(SRC_DIR)/util.hpp
# 	$(CXX) $(CXXFLAGS) -c $^ -o $@
# $(OBJ_DIR)/worm_server.o: $(SRC_DIR)/worm_server.cpp $(SRC_DIR)/worm_server.hpp $(SRC_DIR)/client_proc.cpp $(SRC_DIR)/client_proc.hpp $(SRC_DIR)/util.hpp
# 	$(CXX) $(CXXFLAGS) -c $^ -o $@
# $(OBJ_DIR)/client_proc.o: $(SRC_DIR)/client_proc.cpp $(SRC_DIR)/client_proc.hpp $(SRC_DIR)/util.hpp
# 	$(CXX) $(CXXFLAGS) -c $^ -o $@
# $(OBJ_DIR)/shm_manager.o: $(SRC_DIR)/shm_manager.cpp $(SRC_DIR)/shm_manager.hpp $(SRC_DIR)/util.hpp
# 	$(CXX) $(CXXFLAGS) -c $^ -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

print:
	echo $(OBJECTS)

# client: codes/worm_client.cpp	
# 	g++ -o worm_client codes/worm_client.cpp

# cp: codes/worm_cp.cpp
# 	g++ -o worm_cp codes/worm_cp.cpp


.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE)
.PHONY: all clean

CC = g++ -g -std=c++11
CFLAG = -c -Wall -fpermissive -pthread
INC_PATH = ./include/
SRC_PATH = ./src/
OBJ_PATH = ./obj/
SRC = $(wildcard $(SRC_PATH)*.cpp)
OBJ = $(patsubst $(SRC_PATH)%.cpp, $(OBJ_PATH)%.o, $(SRC))

HEADER = $(wildcard $(INC_PATH)*.h)
TARGET = SecondFileSystem

# mkdir obj
$(shell mkdir -p $(OBJ_PATH))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -I$(INC_PATH) -pthread -o $@ $^

$(OBJ):$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp $(HEADER)
	$(CC) $(CFLAG) -I$(INC_PATH) -o$ $@ $<

clean:
	rm -rf $(TARGET) *.img core.Second* $(OBJ)

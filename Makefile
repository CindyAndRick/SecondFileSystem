.PHONY: all clean img makeImg cleanImg

CC = g++ -g -std=c++11
CFLAG = -c -Wall -fpermissive
INC_PATH = ./include/
SRC_PATH = ./src/
OBJ_PATH = ./obj/
SRC = $(wildcard $(SRC_PATH)*.cpp)
OBJ = $(patsubst $(SRC_PATH)%.cpp, $(OBJ_PATH)%.o, $(SRC))

HEADER = $(wildcard $(INC_PATH)*.h)
TARGET = SecondFileSystem

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -I$(INC_PATH) -o $@ $^

$(OBJ):$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp $(HEADER)
	$(CC) $(CFLAG) -I$(INC_PATH) -o$ $@ $<

# all:
# 	$(CC) -I$(INC_PATH) -o $(TARGET) $(SRC) 


clean:
	rm -rf $(TARGET) *.img core.Second* $(OBJ)
# rm -rf $(OBJ)*.o $(TARGET)

# user:
# g++ -o user ./src/User.cpp ./include/User.h ./inlcude/File.h ./include/INode.h ./include/FileManager.h

# img: 
# 	g++ -o makeImg $(IMGSRC) -g

#./include/INode.h ./include/FileSystem.h

# makeImg: makeImg
# 	@./makeImg

# cleanImg: 
# 	rm -rf makeImg *.img core.makeImg.*
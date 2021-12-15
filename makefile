CC	= g++
CFLAGS 	= -std=c++11
TARGET1 = oss
TARGET2 = child
SOURCE1 = oss.cpp
SOURCE2 = child.cpp
OUTPUT = $(TARGET1) $(TARGET2)


all: $(OUTPUT)

$(TARGET1): $(OBJFILES)
	$(CC) -o $(TARGET1) $(SOURCE1) $(CFLAGS) oss.h pcb.h

$(TARGET2): $(OBJFILES)
	$(CC) -o $(TARGET2) $(SOURCE2) $(CFLAGS) oss.h pcb.h


clean: 
	rm -f $(TARGET1) $(TARGET2) *.o logfile

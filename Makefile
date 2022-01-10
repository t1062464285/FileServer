CC=g++ -g -Wall -std=c++17 
# List of source files for your file server
FS_SOURCES=fs_helper.cpp fs_servers.cpp

# Generate the names of the file server's object files
FS_OBJS=${FS_SOURCES:.cpp=.o}

all: fs app

# Compile the file server and tag this compilation
fs: ${FS_OBJS} libfs_server.o

	${CC} -o $@ $^ -pthread -ldl

# Compile a client program
app: test${n}.cpp libfs_client.o 
	${CC} -o $@ $^ -pthread

# private test
t: t${n}.cpp libfs_client.o 
	${CC} -o $@ $^ -pthread

11-1: t11-1.cpp libfs_client.o 
	${CC} -o $@ $^ -pthread

11-2: t11-2.cpp libfs_client.o 
	${CC} -o $@ $^ -pthread

12-1: t12-1.cpp libfs_client.o 
	${CC} -o $@ $^ -pthread

12-2: t12-2.cpp libfs_client.o 
	${CC} -o $@ $^ -pthread

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${FS_OBJS} fs app t 11-2 11-1

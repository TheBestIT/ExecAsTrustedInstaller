CPP_COMPILER=g++

all: execAsTI.exe

clean:
	del execAsTI.exe

execAsTI.exe: execAsTI.cpp
	$(CPP_COMPILER) $^ -o $@ -static
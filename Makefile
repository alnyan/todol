CXXFLAGS:=$(if ${WITH_WEB},-DWITH_WEB -lpthread,)
SOURCES:=src/todol.cpp src/main.cpp $(if ${WITH_WEB},src/todol_web.cpp src/socket.cpp,)

all:
	rm -f todol todol-socket todol-server
	g++ $(CXXFLAGS) -o todol ${SOURCES}
	$(if ${WITH_WEB}, \
		g++ $(CXXFLAGS) -o todol-socket src/socket.cpp test/socket.cpp; \
		g++ $(CXXFLAGS) -o todol-server src/todol.cpp src/todol_web.cpp test/httpserver.cpp src/socket.cpp \
	,)
CXXFLAGS:=$(if ${WITH_WEB},-DWITH_WEB -lpthread,) $(if ${WITHOUT_COLOR},,-DWITH_COLOR) $(if ${WITHOUT_AT},,-DWITH_AT)
SOURCES:=src/todol.cpp src/main.cpp $(if ${WITH_WEB},src/todol_web.cpp src/socket.cpp,) $(if ${WITHOUT_AT},,src/todol_at.cpp src/timeutil.cpp)

all:
	rm -f todol todol-socket todol-server todol-at
	g++ $(CXXFLAGS) -o todol ${SOURCES}
	$(if ${WITH_WEB}, \
		g++ $(CXXFLAGS) -o todol-socket src/socket.cpp test/socket.cpp; \
		g++ $(CXXFLAGS) -o todol-server src/todol.cpp src/todol_web.cpp test/httpserver.cpp src/socket.cpp \
	,)
	$(if ${WITHOUT_AT},,g++ $(CXXFLAGS) -o todol-at src/todol_at.cpp test/at.cpp)

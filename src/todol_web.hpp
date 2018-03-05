#pragma once
#include "socket.hpp"
#include <stdint.h>

namespace todol {
namespace web {

struct HttpRequest {
	enum HttpRequestMethod {
		HTTP_UNKNOWN = 0, HTTP_GET, HTTP_POST, HTTP_PATCH, HTTP_DELETE
	} method;
	std::string url;
	size_t contentLength;
    std::string text;
    std::string contentType;
};

struct HttpResponse {
    std::string text;
    std::string contentType;
    int fd;
    int code;
};

bool parseRequestStatusLine(const std::string &line, HttpRequest &req);
bool writeStatusLine(TcpSocket &socket, const HttpResponse &res);
bool parseRequestHeaders(TcpSocket &socket, const std::string &firstChunk, HttpRequest &req);
bool parseRequestHeaderLine(const std::string &line, HttpRequest &req);
bool readRequestPayload(TcpSocket &socket, HttpRequest &req);
void dumpBuffer(const char *buf, size_t sz);

class HttpServer {
public:
	HttpServer(uint16_t port);

	void start();

private:

	void serve(int fd, struct sockaddr_in sa);
    bool process(struct sockaddr_in &sa, const HttpRequest &req, HttpResponse &res);

	TcpServerSocket m_socket;
};

int startServer(uint16_t port);

}
}

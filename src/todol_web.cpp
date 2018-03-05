#include "todol_web.hpp"
#include <string_view>
#include <sys/stat.h>
#include "todol.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <thread>

static njson s_taskCache;
static bool s_cacheValid = false;

todol::web::HttpServer::HttpServer(uint16_t port)
		: m_socket(port) {
}

void todol::web::HttpServer::start() {
	int fd;
	struct sockaddr_in sa;
	while (true) {
		if (m_socket.accept(fd, sa)) {
			auto fn = std::bind(&HttpServer::serve, this, std::placeholders::_1,
					std::placeholders::_2);

			std::thread thr(fn, fd, sa);
			thr.detach();
		}
	}
}

void todol::web::HttpServer::serve(int fd, struct sockaddr_in sa) {
	TcpSocket socket;
	socket.fromDescriptor(sa, fd);

	HttpRequest req;
    req.contentLength = 0;

	char buf[257];
	buf[256] = 0;

	// First - read some headers
	size_t r = socket.read(buf, 256);

	if (!r) {
		return;
	}

	// Get first line of the buffer
	const char *e = strstr(buf, "\r\n");
	if (!e) {
		e = strstr(buf, "\n");
		if (!e) {
			// Malformed request
			return;
		}
	}
	std::string requestLine(buf, e - buf);

	if (!parseRequestStatusLine(requestLine, req)) {
		return;
	}

    // Read rest of headers and find Content-Length
    if (!parseRequestHeaders(socket, std::string(buf, r), req)) {
        return;
    }

    if (!readRequestPayload(socket, req)) {
        return;
    }

    HttpResponse res;
    res.fd = -1;
    res.contentType = "text/plain";

    if (process(sa, req, res)) {
        writeStatusLine(socket, res);
        std::string cl = "Content-Type: " + res.contentType + ";charset=utf-8\r\n";

        socket.write(cl.c_str(), cl.length());

        if (res.fd == -1) {
            cl = "Content-Length: " + std::to_string(res.text.length()) + "\r\n\r\n";

            socket.write(cl.c_str(), cl.length());
            socket.write(res.text.c_str(), res.text.length());

            std::cout << "Sent response" << std::endl;
        } else {
            size_t contentLength = 0;
            struct stat fs;

            if (fstat(res.fd, &fs) != 0) {
                return;
            }

            contentLength = fs.st_size;

            cl = "Content-Length: " + std::to_string(contentLength) + "\r\n\r\n";

            socket.write(cl.c_str(), cl.length());

            size_t sum = 0;
            ssize_t bread;
            while ((bread = read(res.fd, buf, 256)) > 0) {
                socket.write(buf, bread);
                std::cout << std::string(buf, bread);
                sum += bread;
            }

            std::cout << "Sent " << sum << " bytes of " << contentLength << " byte file" << std::endl;

            return;
        }

        return;
    }

    std::cerr << "Failed to process request" << std::endl;
}

bool todol::web::HttpServer::process(struct sockaddr_in &sa, const todol::web::HttpRequest &req,
        todol::web::HttpResponse &res) {
    switch (req.method) {
    case HttpRequest::HTTP_GET:
        {
            if (req.url == "/") {
                res.contentType = "text/html";
                res.fd = open("index.html", O_RDONLY);

                return true;
            } else if (req.url == "/tasks.json") {
                res.contentType = "application/json";

                if (!s_cacheValid) {
                    DbHandle db;

                    if (!readDatabase(db)) {
                        res.text = njson({
                            {"status", "error"},
                            {"message", "Failed to read database"}
                        }).dump(4, ' ', false);
                        return true;
                    }

                    s_cacheValid = true;
                    s_taskCache = db.json;
                }

                res.text = njson({
                            {"status", "success"},
                            {"data", s_taskCache}
                        }).dump(4, ' ', false);

                return true;
            }

            break;
        }
    case HttpRequest::HTTP_POST:
        {
            if (req.url == "/tasks.json") {
                res.contentType = "application/json";

                if (req.contentType != "application/json") {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Bad request"}
                    }).dump(4, ' ', false);

                    return true;
                }

                DbHandle db;

                if (!readDatabase(db)) {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Failed to read database"}
                    }).dump(4, ' ', false);
                    s_cacheValid = false;

                    return true;
                }

                auto reqJson = njson::parse(req.text);

                if (!reqJson["title"].is_string()) {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Bad request"}
                    }).dump(4, ' ', false);

                    return true;
                }

                if (addTask(db, reqJson["title"], 0) == -1) {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Failed to add task"}
                    }).dump(4, ' ', false);
                    s_cacheValid = false;

                    return true;
                }

                 if (!writeDatabase(db)) {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Failed to write database"}
                    }).dump(4, ' ', false);
                    s_cacheValid = false;

                    return true;
                }

                s_taskCache = db.json;

                res.text = njson({
                    {"status", "success"}
                }).dump(4, ' ', false);

                return true;
            }
        }
    case HttpRequest::HTTP_PATCH:
        {
            if (req.url == "/tasks.json") {
                res.contentType = "application/json";

                if (req.contentType != "application/json") {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Bad request"}
                    }).dump(4, ' ', false);

                    return true;
                }

                DbHandle db;

                if (!readDatabase(db)) {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Failed to read database"}
                    }).dump(4, ' ', false);
                    s_cacheValid = false;

                    return true;
                }

                auto reqJson = njson::parse(req.text);

                for (auto &task: db.json["tasks"]) {
                    if (task["id"] == reqJson["id"]) {
                        if (reqJson["flags"].is_number()) {
                            task["flags"] = reqJson["flags"];
                        }

                        break;
                    }
                }

                if (!writeDatabase(db)) {
                    res.text = njson({
                        {"status", "error"},
                        {"message", "Failed to write database"}
                    }).dump(4, ' ', false);
                    s_cacheValid = false;

                    return true;
                }

                s_taskCache = db.json;

                res.text = njson({{"status", "success"}}).dump(4, ' ', false);

                return true;
            }
        }
    default:
        break;
    }

    return false;
}

bool todol::web::readRequestPayload(TcpSocket &socket, todol::web::HttpRequest &req) {
    char buf[257];
    buf[256] = 0;
    size_t totalBread = req.text.length();
    size_t toRead = 256;

    if (req.contentLength - totalBread < toRead) {
        toRead = req.contentLength - totalBread;
    }

    while (totalBread < req.contentLength) {
        size_t bread = socket.read(buf, toRead - 1);

        if (bread == 0) {
            break;
        }

        req.text += std::string(buf, bread);

        totalBread += bread;

        toRead = 256;

        if (req.contentLength - totalBread < toRead) {
            toRead = req.contentLength - totalBread;
        }
    }

    return true;
}

bool todol::web::writeStatusLine(TcpSocket &socket, const todol::web::HttpResponse &res) {
    std::string status = "HTTP/1.1 " + std::to_string(res.code) + " ";

    switch (res.code) {
        case 200:
            status += "OK";
            break;
    }

    status += "\r\n";

    socket.write(status.c_str(), status.length());

    return true;
}

bool todol::web::parseRequestHeaderLine(const std::string &line, HttpRequest &req) {
    const char *p = line.c_str();
    const char *e = strchr(p, ':');

    if (!e) {
        return false;
    }

    std::string key = std::string(p, e - p);

    if (*(e + 1) == ' ') {
        ++e;
    }

    std::string value = std::string(e);

    std::cout << "Key: " << key << ", Value: " << value << std::endl;
    if (key == "Content-Length") {
        size_t sz = atoi(value.c_str());

        if (sz) {
            req.contentLength = sz;
        }
    } else if (key == "Content-Type") {
        const char *x = strchr(e, ';');
        // Drop non-mime part (XXX)
        value = std::string(e, x - e);

        req.contentType = value;
    }

    std::cout << "End line" << std::endl;

    return false;
}

bool todol::web::parseRequestHeaders(TcpSocket &socket, const std::string &firstChunk,
        HttpRequest &req) {
    char buf[257];
    buf[256] = 0;
    const char *p = firstChunk.c_str();
    std::string stolenPayload;
    const char *e2;
    bool stop = false;
    if ((e2 = strstr(p, "\r\n\r\n")) || (e2 = strstr(p, "\n\n"))) {
        stop = true;
        size_t stolenPayloadSize = strlen(p) - (e2 - p);

        stolenPayload = std::string(e2, stolenPayloadSize);
        std::cout << "FUCKUP ZONE ENTERED" << std::endl;
    }

    const char *e = strchr(p, '\n');
    p = e + 1;

    while (!stop || p) {
        e = strchr(p, '\n');
        if (!e) {
            size_t s = strlen(p);
            // Try read more
            strcpy(buf, p);

            size_t bread = socket.read(buf + s, 256 - s);

            if (bread == 0 || strstr(buf, "\r\n\r\n") || strstr(buf, "\n\n")) {
                stop = true;

                e = strstr(buf, "\r\n\r\n");

                if (e) {
                    if (bread + s > e - buf) {
                        size_t stolenPayloadSize = bread + s - (e - buf) - strlen("\r\n\r\n");
                        //std::cout << "A1" << std::endl;
                        //std::cout << stolenPayloadSize << std::endl;
                        stolenPayload = std::string(e + strlen("\r\n\r\n"), stolenPayloadSize);
                        //std::cout << "B1" << std::endl;
                    }
                }
            }

            // Finish chunk
            p = buf;

            continue;
        }

        if (*(e - 1) == '\r') {
            --e;
        }

        //std::cout << "A2" << std::endl;
        std::string line(p, e - p);
        //std::cout << "B2" << std::endl;

        if (line.empty()) {
            break;
        }

        parseRequestHeaderLine(line, req);

        if (*e == '\r') {
            ++e;
        }
        p = e + 1;
    }

    std::cout << "Request content length: " << req.contentLength << ", type: " <<
        req.contentType << std::endl;
    req.text = stolenPayload;

    return true;
}

void todol::web::dumpBuffer(const char *buf, size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
        std::cout << buf[i] << " ";

        if (i && (!(i % 4)))
            std::cout << std::endl;
    }
}

bool todol::web::parseRequestStatusLine(const std::string &line, HttpRequest &req) {
	const char *l = line.c_str();
    const char *e = strchr(l, ' ');

    if (!e) {
        return false;
    }

    size_t sz = e - l;

    if (strncmp(l, "GET", sz) == 0) {
        req.method = HttpRequest::HTTP_GET;
    } else if (strncmp(l, "PATCH", sz) == 0) {
        req.method = HttpRequest::HTTP_PATCH;
    } else {
        std::cerr << "Unknown request kind " << l << std::endl;
        return false;
    }

    l = e + 1;
    e = strchr(l, ' ');

    if (!e) {
        return false;
    }

    sz = e - l;

    req.url = std::string(l, sz);

    return true;
}

int todol::web::startServer(uint16_t port) {
	std::cout << "Starting server on " << port << "..." << std::endl;
	std::cout << "Press Ctrl+C to terminate" << std::endl;

	HttpServer server(port);
	server.start();

	return EXIT_SUCCESS;
}

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

constexpr int DEFAULT_PORT = 54000;
constexpr int BUFFER_SIZE = 1024;


void clientHandler(SOCKET clientSocket) {
	char buffer[BUFFER_SIZE];
	int bytesReceived{};

	bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
	if (bytesReceived > 0) {
		buffer[bytesReceived] = '\0';
		std::string receivedMsg(buffer);
		std::cout << "[client " << clientSocket << "] received: " << receivedMsg << std::endl;

		//answer
		std::string response = "server received: " + receivedMsg;

		int bytesSent = send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
		if (bytesSent == SOCKET_ERROR) {
			std::cerr << "[client " << clientSocket << "] send failed: "
				<< WSAGetLastError() << std::endl;
		}
	}
	else if (bytesReceived == 0) {
		std::cout << "[client " << clientSocket << "] connection closing..." << std::endl;
	}
	else {
		std::cerr << "[Client " << clientSocket << "] recv failed: "
			<< WSAGetLastError() << std::endl;
	}

	closesocket(clientSocket);
}


int main() {
	WSADATA wsaData;
	SOCKET listenSocket{ INVALID_SOCKET };

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return 1;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(DEFAULT_PORT);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "server listening on port " << DEFAULT_PORT << "..." << std::endl;

	while (true) {
		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);

		SOCKET clientSocket = accept(
			listenSocket,
			reinterpret_cast<sockaddr*>(&clientAddr),
			&clientAddrSize
		);

		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
			continue;
		}

		std::cout << "accepted connection, socket: " << clientSocket << std::endl;

		std::thread t(clientHandler, clientSocket);
		t.detach();
	}

	closesocket(listenSocket);
	WSACleanup();

	return 0;
}
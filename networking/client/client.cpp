#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#pragma comment(lib, "Ws2_32.lib")

constexpr int DEFAULT_PORT = 54000;
constexpr char DEFAULT_ADDRESS[] = "127.0.0.1";
constexpr int BUFFER_SIZE = 1024;

int main() {
	WSADATA wsaData;
	SOCKET connectSocket = INVALID_SOCKET;
	sockaddr_in serverAddr{};

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return 1;
	}

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(DEFAULT_PORT);
	inet_pton(AF_INET, DEFAULT_ADDRESS, &serverAddr.sin_addr);

	if (connect(connectSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	std::cout << "connected to server " << DEFAULT_ADDRESS << ":" << DEFAULT_PORT << std::endl;

	std::string sendMsg;
	std::cout << "Enter message to send: ";
	std::getline(std::cin, sendMsg);

	int bytesSent = send(connectSocket, sendMsg.c_str(), static_cast<int>(sendMsg.size()), 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "send failed: " << WSAGetLastError() << std::endl;
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	char recvBuf[BUFFER_SIZE];
	int bytesReceived = recv(connectSocket, recvBuf, BUFFER_SIZE - 1, 0);
	if (bytesReceived > 0) {
		recvBuf[bytesReceived] = '\0';
		std::cout << "Server response: " << recvBuf << std::endl;
	}
	else if (bytesReceived == 0) {
		std::cout << "Connection closed by server." << std::endl;
	}
	else {
		std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
	}

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}
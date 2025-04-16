#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

int main(){
    int serverSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    socklen_t serverAddressLen = sizeof(serverAddress);
    char buffer[BUFFER_SIZE];

    /*
    Создание UDP сокета socket(домен, тип, протокол)) SOCK_DGRAM пишем для UDP типа, 
    протокол = 0 чтобы система сама выбирала соответствующий протокол
    */

    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    // Настройка адреса сервера 
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = 0; /* 0 означает, что система сама выберет свободный порт
                                но можно поставить и вручную например htons(8080); */
    
    // Привязка сокета к адресу int bind ( int sid, struct sockaddr* addr_p, int len );
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Получение номера порта, который выбрала система
    if (getsockname(serverSocket, (struct sockaddr*)&serverAddress, &serverAddressLen) < 0) {
        std::cerr << "Ошибка при получении информации о сокете" << std::endl;
        close(serverSocket);
        return 1;
    }
    std::cout << "Сервер запущен на порту: " << ntohs(serverAddress.sin_port) << std::endl;

    while (true) {
        /*
        Получение данных от клиента через функцию int recvfrom ( int sid, const char* buf, int len, int flag,
        struct sockaddr* addr_p, int* len_p );
        */
        int bytesReceived = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0,
                                     (struct sockaddr*)&clientAddress, &clientAddressLen);
                                    
        if (bytesReceived < 0) {
            std::cerr << "Ошибка при получении данных" << std::endl;
            continue;
        }

        buffer[bytesReceived] = '\0'; // Добавляем завершающий нуль для строки, чтобы завершить строку в стиле C
        std::cout << "Получено от клиента [" << inet_ntoa(clientAddress.sin_addr) << ":"
                  << ntohs(clientAddress.sin_port) << "]: " << buffer << std::endl;

        // Преобразование данных (например, удвоение числа)
        int number = atoi(buffer);
        int transformedNumber = number * 2;
        std::string response = "Преобразованное число: " + std::to_string(transformedNumber);

        /*
        Отправка ответа клиенту через функцию int sendto (int sid, const char* buf, int len,
        int flag, struct sockaddr* addr_p, int* len_p );
        */ 
        if (sendto(serverSocket, response.c_str(), response.size(), 0,
                   (struct sockaddr*)&clientAddress, clientAddressLen) < 0) {
            std::cerr << "Ошибка при отправке данных" << std::endl;
        }
    }

    close(serverSocket);
    return 0;
}
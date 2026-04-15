#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

using namespace std;

//Залікова книжка
const int LAST_TWO_DIGITS = 15;   // ХХ - дві останні цифри
const int LAST_THREE_DIGITS = 315; // ХХХ - три останні цифри


void printWsaError(const string& msg) {
    cout << "[ПОМИЛКА] " << msg << " Код: " << WSAGetLastError() << endl;
}

int main() {
    // Встановлюємо кодування UTF-8 для коректного відображення кирилиці у VS Code
    SetConsoleOutputCP(CP_UTF8);

    cout << "=== ЛАБОРАТОРНА РОБОТА №1 ===" << endl;
    cout << "Студент: Дубінський Олександр, Варіант 15: " << LAST_TWO_DIGITS << endl << endl;

    //1: Ініціалізація WinSock
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    cout << "1. Перевірка та ініціалізація WinSock..." << endl;
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        cout << "Критична помилка: не вдалося знайти бібліотеку WinSock. Код: " << err << endl;
        return 1;
    }

    cout << "Підсистему WinSock успішно ініціалізовано!" << endl;
    cout << "Версія: " << int(HIBYTE(wsaData.wVersion)) << "." << int(LOBYTE(wsaData.wVersion)) << endl;
    cout << "Опис: " << wsaData.szDescription << endl;
    cout << "Статус: " << wsaData.szSystemStatus << endl << endl;

    // 2: Створення сокетів
    cout << "2. Створення та налаштування сокетів..." << endl;
    
    // Два датаграмні (UDP)
    SOCKET sock1_bcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKET sock2_fixed1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // Два віртуального каналу (TCP)
    SOCKET sock3_fixed2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKET sock4_any = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock1_bcast == INVALID_SOCKET || sock2_fixed1 == INVALID_SOCKET || 
        sock3_fixed2 == INVALID_SOCKET || sock4_any == INVALID_SOCKET) {
        printWsaError("Помилка створення сокета.");
        WSACleanup();
        return 1;
    }

    // Налаштування розміру датаграми 
    int maxMsgSize = LAST_THREE_DIGITS;
    setsockopt(sock1_bcast, SOL_SOCKET, SO_SNDBUF, (char*)&maxMsgSize, sizeof(maxMsgSize));
    setsockopt(sock2_fixed1, SOL_SOCKET, SO_RCVBUF, (char*)&maxMsgSize, sizeof(maxMsgSize));

    // Налаштування широкомовного режиму для першого сокета
    BOOL bcastOpt = TRUE;
    setsockopt(sock1_bcast, SOL_SOCKET, SO_BROADCAST, (char*)&bcastOpt, sizeof(bcastOpt));

    cout << "4 сокети створено. Макс. розмір датаграми встановлено: " << maxMsgSize << " байт." << endl << endl;

    // 3: Підготовка структур адрес
    sockaddr_in addr1, addr2, addr3, addr4;
    
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(8081);
    addr1.sin_addr.s_addr = INADDR_BROADCAST; // Широкомовна адреса

    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(8082);
    string ip2 = "10.1.2.1" + to_string(LAST_TWO_DIGITS);
    inet_pton(AF_INET, ip2.c_str(), &addr2.sin_addr); // Фіксована IP XX

    addr3.sin_family = AF_INET;
    addr3.sin_port = htons(8083);
    string ip3 = "10.1.2.1" + to_string(LAST_TWO_DIGITS + 1);
    inet_pton(AF_INET, ip3.c_str(), &addr3.sin_addr); // Фіксована IP XX+1

    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(8084);
    addr4.sin_addr.s_addr = INADDR_ANY; // Будь-яка локальна адреса

    // 4: Прив'язка та симуляція помилок
    cout << "3. Прив'язка сокетів та перевірка помилок..." << endl;

    // Успішна прив'язка 4 сокетів
    bind(sock4_any, (sockaddr*)&addr4, sizeof(addr4)); // Прив'язуємо 4-й сокет

    cout << "\n--- СИМУЛЯЦІЯ 5 ПОМИЛОК ---" << endl;
    // 1. WSAEADDRINUSE (Адреса вже використовується)
    // Пробуємо прив'язати ще один сокет до того ж порту, що і sock4_any (8084)
    SOCKET tempSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (bind(tempSock, (sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR) {
        cout << "[OK] Симуляція WSAEADDRINUSE (10048). Адреса вже зайнята." << endl;
    }
    closesocket(tempSock);

    // 2. WSAEINVAL (Сокет уже прив’язаний)
    // Пробуємо ще раз зробити bind для sock4_any, який ми вже прив'язали вище
    if (bind(sock4_any, (sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR) {
        cout << "[OK] Симуляція WSAEINVAL (10022). Сокет уже прив’язано." << endl;
    }

    // 3. WSAEFAULT (Некоректне значення параметра namelen)
    // Передаємо розмір 1 замість sizeof(sockaddr_in)
    if (bind(sock1_bcast, (sockaddr*)&addr1, 1) == SOCKET_ERROR) {
        cout << "[OK] Симуляція WSAEFAULT (10014). Неправильний розмір адреси." << endl;
    }

    // 4. WSAENOTSOCK (Дескриптор не є сокетом)
    // Передаємо просто вигадане число замість реального сокета
    if (bind((SOCKET)1234567, (sockaddr*)&addr2, sizeof(addr2)) == SOCKET_ERROR) {
        cout << "[OK] Симуляція WSAENOTSOCK (10038). Дескриптор не є сокетом." << endl;
    }

    // 5. WSANOTINITIALISED (Підсистема WinSock не ініціалізована)
    // Штучно вимикаємо WinSock, пробуємо прив'язати, а потім вмикаємо назад
    WSACleanup(); 
    if (bind(sock2_fixed1, (sockaddr*)&addr2, sizeof(addr2)) == SOCKET_ERROR) {
        cout << "[OK] Симуляція WSANOTINITIALISED (10093). WinSock вимкнено." << endl;
    }
    WSAStartup(wVersionRequested, &wsaData); // Вмикаємо назад для нормального завершення

    // --- ЕТАП 5: Коректне завершення ---
    cout << "\n4. Закриття сокетів та завершення програми..." << endl;
    closesocket(sock1_bcast);
    closesocket(sock2_fixed1);
    closesocket(sock3_fixed2);
    closesocket(sock4_any);
    
    WSACleanup();
    cout << "Всі сокети закрито. WinSock вивантажено. Робота завершена!" << endl;

    return 0;
}
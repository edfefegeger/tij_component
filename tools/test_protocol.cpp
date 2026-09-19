/*
 * Прогон команд компоненты против имитатора принтера.
 *
 * Собирается скриптом tools/test-protocol.sh, в рабочую сборку не входит и в
 * архив для 1С не попадает. Нужен, чтобы проверять логику протокола без
 * Windows, без 1С и без реального принтера.
 */

#include "TIJPrinter.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static int g_failed = 0;

static void report(const char* what, int code, const std::string& err) {
    if (code == 0) {
        printf("  %-18s код %3d   успех\n", what, code);
    } else {
        printf("  %-18s код %3d   ошибка: %s\n", what, code, err.c_str());
        ++g_failed;
    }
}

int main(int argc, char** argv) {
    const int port = (argc > 1) ? std::atoi(argv[1]) : 8010;

    TIJPrinter printer;

    printf("Подключение к 127.0.0.1:%d\n", port);
    if (!printer.Connect("127.0.0.1", port)) {
        printf("  Connect: не удалось, ошибка: %s\n", printer.GetLastError().c_str());
        return 2;
    }
    printf("  Состояние соединения: %s\n", printer.IsConnected() ? "есть" : "нет");

    report("ОчиститьБуфер", printer.ClearBuffer(), printer.GetLastError());
    report("ОтправитьПоле", printer.SendField(1, "(01)0880999084565666771(21)5/56BK"),
           printer.GetLastError());
    report("НачатьПечать", printer.StartPrint(), printer.GetLastError());
    report("ОстановитьПечать", printer.StopPrint(), printer.GetLastError());

    printer.Disconnect();
    printf("Отключено, состояние: %s\n", printer.IsConnected() ? "есть" : "нет");
    printf("Команд с ошибкой: %d\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}

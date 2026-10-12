/*
 * Прогон команд компоненты против имитатора принтера.
 *
 * Собирается скриптом tools/test-protocol.sh, в рабочую сборку не входит и в
 * архив для 1С не попадает. Нужен, чтобы проверять логику протокола без
 * Windows, без 1С и без реального принтера.
 *
 * Запуск: test_protocol <порт> [ack|nak]
 *   ack — имитатор подтверждает команды, все команды должны проходить;
 *   nak — имитатор отвечает отказом, все команды должны вернуть -2.
 */

#include "TIJPrinter.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static int  g_failed = 0;
static bool g_nak    = false;

static void report(const char* what, int code, const std::string& err) {
    if (code >= 0)
        printf("  %-34s код %4d   принято\n", what, code);
    else
        printf("  %-34s код %4d   %s\n", what, code, err.c_str());
}

/* В штатном режиме команда должна пройти, в режиме отказа вернуть -2. */
static void expect_command(TIJPrinter& printer, const char* what, int code) {
    report(what, code, printer.GetLastError());
    if (g_nak) {
        if (code != TIJ_ERR_NAK)
            ++g_failed;
    } else if (code < 0) {
        ++g_failed;
    }
}

/* Накопление поля в пакет на принтер не ходит и всегда возвращает ноль. */
static void expect_zero(TIJPrinter& printer, const char* what, int code) {
    report(what, code, printer.GetLastError());
    if (code != 0)
        ++g_failed;
}

/* Неверные параметры отсекает сама компонента, до обращения к принтеру. */
static void expect_param_error(TIJPrinter& printer, const char* what, int code) {
    report(what, code, printer.GetLastError());
    if (code != TIJ_ERR_PARAM)
        ++g_failed;
}

/* Ожидается конкретный код: например -4, когда очередь не освободилась. */
static void expect_code(TIJPrinter& printer, const char* what, int code, int want) {
    report(what, code, printer.GetLastError());
    if (code != want)
        ++g_failed;
}

/* Ожидается конкретное значение: например число сообщений в буфере. */
static void expect_value(const char* what, int got, int want) {
    printf("  %-34s получено %4d, ожидалось %d\n", what, got, want);
    if (got != want)
        ++g_failed;
}

int main(int argc, char** argv) {
    const int port = (argc > 1) ? std::atoi(argv[1]) : 8010;
    const std::string mode = (argc > 2) ? std::string(argv[2]) : std::string("ack");
    g_nak = (mode == "nak");

    TIJPrinter printer;

    printf("Подключение к 127.0.0.1:%d, режим имитатора: %s\n", port,
           g_nak ? "отказ" : "подтверждение");
    if (!printer.Connect("127.0.0.1", port)) {
        printf("  Connect: не удалось, ошибка: %s\n", printer.GetLastError().c_str());
        return 2;
    }
    printf("  Состояние соединения: %s\n", printer.IsConnected() ? "есть" : "нет");

    printf("\n--- обычные команды ---\n");
    expect_command(printer, "ОчиститьБуфер", printer.ClearBuffer());
    expect_command(printer, "ОтправитьПоле(1)", printer.SendField(1, "TIJ Printer"));
    expect_command(printer, "НачатьПечать", printer.StartPrint());
    expect_command(printer, "ОстановитьПечать", printer.StopPrint());

    printf("\n--- пакет из двух полей одним кадром ---\n");
    printer.BeginPackage();
    expect_zero(printer, "ДобавитьПоле(1)", printer.AddField(1, "TIJ Printer"));
    expect_zero(printer, "ДобавитьПоле(3)", printer.AddField(3, "04/27/2019"));
    expect_command(printer, "ОтправитьПоля (2 поля)", printer.SendPackage());
    expect_param_error(printer, "ОтправитьПоля повторно", printer.SendPackage());

    printf("\n--- проверка параметров, на принтер уходить не должно ---\n");
    expect_param_error(printer, "ОтправитьПоле(300)", printer.SendField(300, "x"));
    expect_param_error(printer, "ОтправитьПоле пустое", printer.SendField(1, ""));
    expect_param_error(printer, "ОтправитьПоле 256 байт",
                       printer.SendField(1, std::string(256, 'A')));
    expect_param_error(printer, "ДобавитьПоле(300)", printer.AddField(300, "x"));
    expect_param_error(printer, "ДобавитьПоле пустое", printer.AddField(2, ""));

    if (mode == "batch") {
        printf("\n--- пачка: в очереди принтера пусто, отправляем сразу ---\n");
        expect_value("ЗанятоВБуфере", printer.RequestBufferCount(), 0);
        expect_command(printer, "УстановитьОтказПриПереполнении(Истина)",
                       printer.SetRejectOnBufferFull(true));

        printer.BeginPackage();
        expect_zero(printer, "ДобавитьПоле(1)",
                    printer.AddField(1, "0108809990845656215X56BK1234567"));
        expect_command(printer, "ОтправитьЭтикетку(8, 5000)", printer.SendLabel(8, 5000));
    }

    if (mode == "full") {
        printf("\n--- пачка: очередь принтера занята ---\n");
        expect_value("ЗанятоБуфере", printer.RequestBufferCount(), 8);

        printer.BeginPackage();
        expect_zero(printer, "ДобавитьПоле(1)",
                    printer.AddField(1, "0108809990845656217X56BK1234567"));

        /* глубину 8 держать нельзя, ждём освобождения и не дожидаемся */
        expect_code(printer, "ОтправитьЭтикетку(8, 1500)",
                    printer.SendLabel(8, 1500), TIJ_ERR_BUSY);

        /* при глубине 10 занятые 8 мест не мешают */
        expect_command(printer, "ОтправитьЭтикетку(10, 1500)", printer.SendLabel(10, 1500));
    }

    printf("\n--- расшифровка кодов ---\n");
    const int codes[] = { 0, 66, 67, 17, -1, -2, -3, -4 };
    for (int i = 0; i < 8; ++i)
        printf("  %4d  %s\n", codes[i], printer.DescribeCode(codes[i]).c_str());

    if (g_nak) {
        const std::string desc = printer.DescribeCode(TIJ_ERR_NAK);
        if (desc.find("C_Status=0x") == std::string::npos) {
            printf("  ОШИБКА: в описании отказа нет кода C_Status от принтера\n");
            ++g_failed;
        }
    }

    printer.Disconnect();
    printf("\nОтключено, состояние: %s\n", printer.IsConnected() ? "есть" : "нет");
    printf("Замечаний: %d\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}

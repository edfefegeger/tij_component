#!/usr/bin/env python3
"""
Имитатор принтера TIJ для проверки логики обмена без железа.

Слушает TCP-порт, разбирает кадры по протоколу (раздел 2.2), проверяет
контрольную сумму (раздел 2.4), отвечает ACK с кодом P_Status и тем же
идентификатором команды (раздел 2.3). Если контрольная сумма или структура
кадра неверны, отвечает NAK с кодом C_Status, как настоящий принтер.

Запуск:
    python3 tools/mock_printer.py --port 8010 [--slave 0] [--nak-status 0]

Параметры:
    --port        порт для прослушивания
    --nak-status  если задан, имитатор отвечает NAK с этим кодом на любую
                  команду. Применяется, чтобы проверить обработку отказа.
"""

import argparse
import socket
import sys

ESC = 0x1B
STX = 0x02
ETX = 0x03
ACK = 0x06
NAK = 0x15


def checksum(message: bytes) -> int:
    """2s-complement от суммы всех байтов сообщения по модулю 256."""
    return (0x100 - (sum(message) & 0xFF)) & 0xFF


def build_reply(kind: int, slave: int, status: int, cmd: int) -> bytes:
    head = bytes([ESC, kind, slave, status, cmd, ESC, ETX])
    return head + bytes([checksum(head)])


def parse_frame(buf: bytes):
    """Возвращает (slave, cmd, data) или None с описанием ошибки."""
    if len(buf) < 7:
        return None, "кадр короче минимального"
    if buf[0] != ESC or buf[1] != STX:
        return None, "кадр не начинается с 1B 02"
    if buf[-3] != ESC or buf[-2] != ETX:
        return None, "кадр не заканчивается на 1B 03"
    expected = checksum(buf[:-1])
    if buf[-1] != expected:
        return None, f"контрольная сумма {buf[-1]:02X}, ожидалась {expected:02X}"
    return (buf[2], buf[3], buf[4:-3]), None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8010)
    ap.add_argument("--slave", type=int, default=0)
    ap.add_argument("--nak-status", type=int, default=None)
    ap.add_argument("--once", action="store_true", help="завершиться после первого соединения")
    args = ap.parse_args()

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", args.port))
    srv.listen(1)
    print(f"[mock] слушаю 127.0.0.1:{args.port}", flush=True)

    while True:
        conn, addr = srv.accept()
        print(f"[mock] подключение от {addr[0]}:{addr[1]}", flush=True)
        buf = b""
        with conn:
            while True:
                chunk = conn.recv(1024)
                if not chunk:
                    break
                buf += chunk
                while len(buf) >= 7:
                    if buf[0] != ESC or buf[1] != STX:
                        print(f"[mock] мусор в потоке: {buf[:8].hex(' ')}", flush=True)
                        buf = b""
                        break
                    end = buf.find(bytes([ESC, ETX]), 2)
                    if end < 0:
                        break
                    frame, buf = buf[:end + 3], buf[end + 3:]
                    if len(frame) < 7:
                        continue
                    parsed, err = parse_frame(frame)
                    if parsed is None:
                        slave = frame[2] if len(frame) > 2 else 0
                        cmd = frame[3] if len(frame) > 3 else 0
                        print(f"[mock] ПЛОХОЙ КАДР: {err}", flush=True)
                        print(f"[mock]   байты: {frame.hex(' ').upper()}", flush=True)
                        conn.sendall(build_reply(NAK, slave, 0x01, cmd))
                        continue
                    slave, cmd, data = parsed
                    print(f"[mock] команда {cmd:02X}, адрес {slave}, данные {data.hex(' ').upper()}",
                          flush=True)
                    if args.nak_status is not None:
                        conn.sendall(build_reply(NAK, slave, args.nak_status, cmd))
                        print(f"[mock] ответ NAK, C_Status={args.nak_status:02X}", flush=True)
                    else:
                        conn.sendall(build_reply(ACK, slave, 0x00, cmd))
                        print(f"[mock] ответ ACK, P_Status=00", flush=True)
        print("[mock] соединение закрыто", flush=True)
        if args.once:
            break

    srv.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env bash
#
# Проверка логики протокола без Windows, без 1С и без принтера.
#
# Собирает src/TIJPrinter.cpp с заглушками Winsock поверх обычных сокетов
# Linux, поднимает имитатор принтера (tools/mock_printer.py) и прогоняет все
# команды компоненты. Имитатор проверяет структуру кадра и контрольную сумму
# так же, как настоящий принтер, и отвечает ACK или NAK.
#
# Второй прогон проверяет обработку отказа: имитатор отвечает NAK с кодом.
#
# Использование:
#     tools/test-protocol.sh
#     PORT=8011 tools/test-protocol.sh
#
set -euo pipefail
cd "$(dirname "$0")/.."

PORT=${PORT:-8010}
CXX=${CXX:-g++}
OUT=build/protocol-test
mkdir -p "$OUT"

echo "=== 1. Сборка прогона ==="
"$CXX" -std=c++17 -Wall -Wextra -Wno-unknown-pragmas \
    -D_WIN32 -D'__declspec(x)=' -D'__stdcall=' \
    -I tools/stubs -I include -I src \
    src/TIJPrinter.cpp tools/stubs/posix_winsock.cpp tools/test_protocol.cpp \
    -o "$OUT/test_protocol"
echo "собрано: $OUT/test_protocol"

echo
echo "=== 2. Нормальный обмен: имитатор отвечает ACK ==="
python3 tools/mock_printer.py --port "$PORT" --once > "$OUT/mock-ack.log" 2>&1 &
MOCK=$!
sleep 0.5
set +e
"$OUT/test_protocol" "$PORT"
DRIVER=$?
set -e
wait $MOCK 2>/dev/null || true

echo
echo "--- что видел имитатор ---"
cat "$OUT/mock-ack.log"

echo
echo "=== 3. Отказ принтера: имитатор отвечает NAK с C_Status=08 ==="
python3 tools/mock_printer.py --port "$PORT" --once --nak-status 8 > "$OUT/mock-nak.log" 2>&1 &
MOCK=$!
sleep 0.5
set +e
"$OUT/test_protocol" "$PORT" nak
DRIVER_NAK=$?
set -e
wait $MOCK 2>/dev/null || true

echo
echo "--- что видел имитатор ---"
cat "$OUT/mock-nak.log"

echo
echo "=== 4. Пачка: очередь свободна, этикетка уходит сразу ==="
python3 tools/mock_printer.py --port "$PORT" --once --buffer-used 0 > "$OUT/mock-batch.log" 2>&1 &
MOCK=$!
sleep 0.5
set +e
"$OUT/test_protocol" "$PORT" batch
DRIVER_BATCH=$?
set -e
wait $MOCK 2>/dev/null || true

echo
echo "--- что видел имитатор ---"
cat "$OUT/mock-batch.log"

echo
echo "=== 5. Пачка: очередь занята, компонента ждёт свободного места ==="
python3 tools/mock_printer.py --port "$PORT" --once --buffer-used 8 > "$OUT/mock-full.log" 2>&1 &
MOCK=$!
sleep 0.5
set +e
"$OUT/test_protocol" "$PORT" full
DRIVER_FULL=$?
set -e
wait $MOCK 2>/dev/null || true

echo
echo "--- что видел имитатор ---"
cat "$OUT/mock-full.log"

echo
if [ "$DRIVER" -eq 0 ]; then
    echo "ИТОГ 1: обмен в штатном режиме прошёл без ошибок"
else
    echo "ИТОГ 1: в штатном режиме есть ошибки, см. вывод выше"
fi

if [ "$DRIVER_NAK" -eq 0 ]; then
    echo "ИТОГ 2: отказ принтера обрабатывается верно, код -2 и расшифровка C_Status"
else
    echo "ИТОГ 2: обработка отказа принтера расходится с ожиданием"
fi

if [ "$DRIVER_BATCH" -eq 0 ]; then
    echo "ИТОГ 3: отправка этикетки при свободной очереди проходит"
else
    echo "ИТОГ 3: отправка этикетки при свободной очереди расходится с ожиданием"
fi

if [ "$DRIVER_FULL" -eq 0 ]; then
    echo "ИТОГ 4: при занятой очереди компонента ждёт место и возвращает -4 по таймауту"
else
    echo "ИТОГ 4: ожидание свободного места в очереди расходится с ожиданием"
fi

if [ "$DRIVER" -ne 0 ] || [ "$DRIVER_NAK" -ne 0 ] || \
   [ "$DRIVER_BATCH" -ne 0 ] || [ "$DRIVER_FULL" -ne 0 ]; then
    exit 1
fi
exit 0

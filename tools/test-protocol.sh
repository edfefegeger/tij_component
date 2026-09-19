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
echo "=== 3. Отказ принтера: имитатор отвечает NAK ==="
python3 tools/mock_printer.py --port "$PORT" --once --nak-status 67 > "$OUT/mock-nak.log" 2>&1 &
MOCK=$!
sleep 0.5
set +e
"$OUT/test_protocol" "$PORT"
set -e
wait $MOCK 2>/dev/null || true

echo
echo "--- что видел имитатор ---"
cat "$OUT/mock-nak.log"

echo
if [ "$DRIVER" -eq 0 ]; then
    echo "ИТОГ: обмен в штатном режиме прошёл без ошибок"
else
    echo "ИТОГ: в штатном режиме есть ошибки, см. вывод выше"
fi
exit "$DRIVER"

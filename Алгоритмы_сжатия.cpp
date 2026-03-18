#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

// ==================== Вспомогательные функции ====================

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// UTF-8 -> wchar_t
wchar_t* charToWchar(const char* str) {
    if (!str) return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (len == 0) return NULL;
    wchar_t* wstr = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wstr) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
    return wstr;
}

// ==================== RLE ====================

void rleCompression(const char* sequence) {
    printf("\n=== Сжатие RLE ===\n");
    if (!sequence || sequence[0] == '\0') {
        printf("Пустая строка!\n");
        return;
    }

    int count = 1;
    char prev = sequence[0];
    int blocks = 0;

    for (int i = 1; sequence[i] != '\0'; i++) {
        if (sequence[i] == prev) count++;
        else {
            printf("(%c,%d) ", prev, count);
            blocks++;
            prev = sequence[i];
            count = 1;
        }
    }

    printf("(%c,%d)\n", prev, count);
    blocks++;

    int originalSize = strlen(sequence);        // байты
    int compressedSize = blocks * 2;            // байты

    printf("Объем после RLE: %d байт\n", compressedSize);
    printf("Коэффициент сжатия: %.2f\n",
        (double)originalSize / compressedSize);
}

// ==================== Хаффман ====================

typedef struct Node {
    wchar_t data;
    int freq;
    struct Node* left;
    struct Node* right;
} Node;

Node* createNode(wchar_t data, int freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

typedef struct { wchar_t ch; int freq; } CharFreq;

int calculateFrequencyW(const wchar_t* str, CharFreq freq[]) {
    int size = 0;
    for (int i = 0; str[i] != L'\0'; i++) {
        int found = -1;
        for (int j = 0; j < size; j++) if (freq[j].ch == str[i]) { found = j; break; }
        if (found != -1) freq[found].freq++;
        else { freq[size].ch = str[i]; freq[size].freq = 1; size++; }
    }
    return size;
}

Node* buildHuffmanTreeW(CharFreq freq[], int size) {
    Node* nodes[1024];
    for (int i = 0; i < size; i++) nodes[i] = createNode(freq[i].ch, freq[i].freq);

    while (size > 1) {
        int min1 = 0, min2 = 1;
        if (nodes[min2]->freq < nodes[min1]->freq) { int t = min1; min1 = min2; min2 = t; }
        for (int i = 2; i < size; i++) {
            if (nodes[i]->freq < nodes[min1]->freq) { min2 = min1; min1 = i; }
            else if (nodes[i]->freq < nodes[min2]->freq) { min2 = i; }
        }
        Node* parent = createNode(L'\0', nodes[min1]->freq + nodes[min2]->freq);
        parent->left = nodes[min1]; parent->right = nodes[min2];
        if (min1 > min2) { int t = min1; min1 = min2; min2 = t; }
        nodes[min1] = parent; nodes[min2] = nodes[size - 1]; size--;
    }
    return nodes[0];
}

typedef struct { wchar_t ch; char code[100]; } HuffCode;

void generateCodesW(Node* root, char* code, int depth, HuffCode codes[], int* size) {
    if (!root) return;
    if (root->left) { code[depth] = '0'; generateCodesW(root->left, code, depth + 1, codes, size); }
    if (root->right) { code[depth] = '1'; generateCodesW(root->right, code, depth + 1, codes, size); }
    if (!root->left && !root->right) {
        code[depth] = '\0';
        codes[*size].ch = root->data;
        strcpy_s(codes[*size].code, code);
        (*size)++;
        wprintf(L"%lc: %S\n", root->data, code);
    }
}

const char* findCode(wchar_t ch, HuffCode codes[], int size) {
    for (int i = 0; i < size; i++) if (codes[i].ch == ch) return codes[i].code;
    return "";
}

void encodeStringW(const wchar_t* str, HuffCode codes[], int size) {
    for (int i = 0; str[i] != L'\0'; i++)
        printf("%s ", findCode(str[i], codes, size));
    printf("\n");
}

void huffmanCompression(const char* str) {
    printf("\n=== Сжатие Хаффмана ===\n");
    if (!str || str[0] == '\0') { printf("Пустая строка!\n"); return; }
    wchar_t* wstr = charToWchar(str);
    if (!wstr) { printf("Ошибка конвертации.\n"); return; }

    CharFreq freq[1024];
    int freqSize = calculateFrequencyW(wstr, freq);

    wprintf(L"\nЧастоты символов:\n");
    for (int i = 0; i < freqSize; i++) wprintf(L"%lc: %d\n", freq[i].ch, freq[i].freq);

    Node* root = buildHuffmanTreeW(freq, freqSize);
    HuffCode codes[1024]; int codeSize = 0;
    char code[100];
    printf("\nКоды символов:\n");
    generateCodesW(root, code, 0, codes, &codeSize);

    printf("\nЗакодированная строка:\n");
    encodeStringW(wstr, codes, codeSize);

    int originalBytes = strlen(str);
    int compressedBits = 0;

    for (int i = 0; wstr[i] != L'\0'; i++) {
        const char* c = findCode(wstr[i], codes, codeSize);
        compressedBits += strlen(c);
    }

    int compressedBytes = (compressedBits + 7) / 8;

    printf("\nОбъем после Хаффмана: %d байт\n", compressedBytes);
    printf("Коэффициент сжатия: %.2f\n",
        (double)originalBytes / compressedBytes);

    free(wstr);
}

// ==================== KWE ====================

#define MAX_LEXEME_LEN 100
#define MAX_TOKENS 512

typedef struct { wchar_t lexeme[MAX_LEXEME_LEN]; unsigned short token; } TokenEntry;
typedef struct { TokenEntry entries[MAX_TOKENS]; int size; unsigned short nextToken; } TokenDictionary;

unsigned short addLexeme(TokenDictionary* dict, const wchar_t* lexeme) {
    for (int i = 0; i < dict->size; i++) if (wcscmp(dict->entries[i].lexeme, lexeme) == 0) return dict->entries[i].token;
    if (dict->size >= MAX_TOKENS) { wprintf(L"Словарь переполнен!\n"); return 0; }
    wcsncpy_s(dict->entries[dict->size].lexeme, MAX_LEXEME_LEN, lexeme, _TRUNCATE);
    dict->entries[dict->size].token = dict->nextToken++;
    dict->size++;
    return dict->entries[dict->size - 1].token;
}

int isDelimiterW(wchar_t ch) { return iswspace(ch) || iswpunct(ch); }

void kweCompression(const char* phrase) {
    printf("\n=== Сжатие KWE ===\n");
    if (!phrase || phrase[0] == '\0') { printf("Пустая строка!\n"); return; }

    wchar_t* wphrase = charToWchar(phrase);
    if (!wphrase) { printf("Ошибка конвертации.\n"); return; }

    TokenDictionary dict = { 0 };
    dict.nextToken = 1;
    unsigned short tokens[MAX_TOKENS]; int tokenCount = 0;
    wchar_t buffer[MAX_LEXEME_LEN]; int bufIndex = 0;

    for (int i = 0; wphrase[i] != L'\0'; i++) {
        wchar_t ch = wphrase[i];
        if (!isDelimiterW(ch)) {
            if (bufIndex < MAX_LEXEME_LEN - 1) buffer[bufIndex++] = ch;
        }
        else {
            if (bufIndex > 0) { buffer[bufIndex] = '\0'; tokens[tokenCount++] = addLexeme(&dict, buffer); bufIndex = 0; }
            wchar_t delim[2] = { ch,L'\0' };
            tokens[tokenCount++] = addLexeme(&dict, delim);
        }
    }
    if (bufIndex > 0) { buffer[bufIndex] = '\0'; tokens[tokenCount++] = addLexeme(&dict, buffer); }

    printf("\nЗакодированные токены (16 бит):\n");
    for (int i = 0; i < tokenCount; i++) {
        for (int j = 15; j >= 0; j--) wprintf(L"%d", (tokens[i] >> j) & 1);
        wprintf(L" ");
    }
    wprintf(L"\n");

    wprintf(L"\nСловарь токенов:\n");
    for (int i = 0; i < dict.size; i++) wprintf(L"%d : %ls\n", dict.entries[i].token, dict.entries[i].lexeme);

    free(wphrase);

    int originalSize = strlen(phrase);       // байты
    int compressedSize = tokenCount * 2;     // 16 бит = 2 байта

    printf("\nОбъем после KWE: %d байт\n", compressedSize);
    printf("Коэффициент сжатия: %.2f\n",
        (double)originalSize / compressedSize);
}

// ==================== Сжатие цифровой последовательности ====================

void compressDigitalSequence() {
    char sequence[256]; int choice;
    printf("\n=== Сжатие цифровой последовательности ===\nВведите последовательность: ");
    scanf_s("%255s", sequence, (unsigned)sizeof(sequence));

    while (1) {
        printf("\nВыберите алгоритм:\n1. RLE\n2. Хаффман\n3. Возврат\nВаш выбор: ");
        if (scanf_s("%d", &choice) != 1) { printf("Ошибка!\n"); clearInputBuffer(); continue; }
        switch (choice) {
        case 1: rleCompression(sequence); break;
        case 2: huffmanCompression(sequence); break;
        case 3: return;
        default: printf("Неверный ввод\n"); break;
        }
    }
}

// ==================== Сжатие фразы ====================

void compressPhrase() {
    char phrase[1024]; int choice;
    printf("\nВведите фразу: ");
    clearInputBuffer();
    fgets(phrase, sizeof(phrase), stdin);
    size_t len = strlen(phrase); if (len > 0 && phrase[len - 1] == '\n') phrase[len - 1] = '\0';

    while (1) {
        printf("\nВыберите алгоритм:\n1. Хаффман\n2. KWE\n3. Возврат\nВаш выбор: ");
        if (scanf_s("%d", &choice) != 1) { printf("Ошибка!\n"); clearInputBuffer(); continue; }
        switch (choice) {
        case 1: huffmanCompression(phrase); break;
        case 2: kweCompression(phrase); break;
        case 3: return;
        default: printf("Неверный ввод\n"); break;
        }
    }
}

// ==================== Сжатие изображения 8x8 ====================

#define SIZE 8
typedef unsigned char Pixel;
Pixel parseChar(char ch) { return (ch == L'Ч' || ch == 'Ч') ? 0xFF : 0x00; }

void inputImage(Pixel image[SIZE][SIZE]) {
    char line[256];

    printf("\n=== Ввод рисунка 8x8 ===\n");
    printf("Формат: 2Ч2Б2Ч (можно с пробелами), недостающие клетки = Б\n");

    clearInputBuffer();

    for (int i = 0; i < SIZE; i++) {
        printf("Строка %d: ", i + 1);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("Ошибка ввода!\n");
            return;
        }

        // убираем \n
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        // 🔥 ключевой момент — конвертация
        wchar_t* wline = charToWchar(line);
        if (!wline) {
            printf("Ошибка конвертации!\n");
            return;
        }

        int pos = 0;
        int col = 0;

        while (wline[pos] != L'\0' && col < SIZE) {

            // --- читаем число ---
            int count = 0;
            while (wline[pos] >= L'0' && wline[pos] <= L'9') {
                count = count * 10 + (wline[pos] - L'0');
                pos++;
            }

            if (count == 0) count = 1;

            // пропуск пробелов
            while (iswspace(wline[pos])) pos++;

            // --- читаем символ ---
            wchar_t ch = wline[pos];

            Pixel color;
            if (ch == L'Ч')
                color = 0xFF;
            else if (ch == L'Б')
                color = 0x00;
            else {
                pos++;
                continue;
            }

            pos++;

            // --- записываем ---
            for (int k = 0; k < count && col < SIZE; k++) {
                image[i][col++] = color;
            }
        }

        // добивка белым
        while (col < SIZE) {
            image[i][col++] = 0x00;
        }

        free(wline); // ❗ важно
    }
}

// RLE для изображения
void rleImageCompression(Pixel image[SIZE][SIZE]) {
    printf("\n=== RLE сжатие изображения 8x8 ===\n");

    Pixel prev = image[0][0];
    int count = 1;
    int totalBlocks = 0;  // количество блоков (символ + кол-во)

    for (int i = 0; i < SIZE; i++) {
        for (int j = (i == 0 ? 1 : 0); j < SIZE; j++) {
            if (image[i][j] == prev) {
                count++;
            }
            else {
                printf("(%s,%d) ", prev == 0xFF ? "Ч" : "Б", count);
                totalBlocks++;
                prev = image[i][j];
                count = 1;
            }
        }
    }
    printf("(%s,%d)\n", prev == 0xFF ? "Ч" : "Б", count);
    totalBlocks++;

    int compressedSizeBytes = totalBlocks * 2;  // 1 байт для цвета + 1 байт для счетчика
    printf("Объем изображения после RLE: %d байт\n", compressedSizeBytes);
    printf("Коэффициент сжатия: %.2f\n", (double)(SIZE * SIZE) / compressedSizeBytes);
}

// ================= Вывод дерева Хаффмана =================
void printHuffmanTree(Node* root, int level) {
    if (!root) return;

    // Печатаем правое поддерево
    printHuffmanTree(root->right, level + 1);

    // Печатаем текущий узел с отступом
    for (int i = 0; i < level; i++) printf("    "); // 4 пробела на уровень
    if (root->left == NULL && root->right == NULL)
        wprintf(L"%lc (%d)\n", root->data, root->freq); // лист: символ и частота
    else
        printf("* (%d)\n", root->freq); // внутренний узел

    // Печатаем левое поддерево
    printHuffmanTree(root->left, level + 1);
}

// ================= Хаффман для изображения 8x8 =================
void huffmanImageCompression(Pixel image[SIZE][SIZE]) {
    printf("\n=== Хаффман сжатие изображения 8x8 ===\n");

    // Формируем массив символов для изображения
    wchar_t data[SIZE * SIZE + 1];
    int index = 0;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            data[index++] = image[i][j] == 0xFF ? L'Ч' : L'Б';
    data[index] = L'\0';

    // Инициализация частот обоих символов
    CharFreq freq[2];
    freq[0].ch = L'Ч'; freq[0].freq = 0;
    freq[1].ch = L'Б'; freq[1].freq = 0;
    int freqSize = 2;

    // Подсчёт частот
    for (int i = 0; i < SIZE * SIZE; i++) {
        if (data[i] == L'Ч') freq[0].freq++;
        else freq[1].freq++;
    }

    // Вывод таблицы частот
    printf("\nСимвол | Частота\n");
    for (int i = 0; i < freqSize; i++)
        wprintf(L"%lc      | %d\n", freq[i].ch, freq[i].freq);

    // Строим дерево Хаффмана
    Node* root = buildHuffmanTreeW(freq, freqSize);

    // Генерация кодов
    HuffCode codes[2];
    int codeSize = 0;
    char code[10];
    printf("\nКоды Хаффмана:\n");
    generateCodesW(root, code, 0, codes, &codeSize);

    // Подсчёт сжатого объёма в битах
    int totalBits = 0;
    for (int i = 0; i < SIZE * SIZE; i++) {
        const char* c = findCode(data[i], codes, codeSize);
        totalBits += strlen(c);
    }

    int compressedBytes = (totalBits + 7) / 8;
    printf("\nОбъем изображения после Хаффмана: %d байт\n", compressedBytes);
    printf("Коэффициент сжатия: %.2f\n", (double)(SIZE * SIZE) / compressedBytes);

    // Вывод дерева Хаффмана
    printf("\nДерево Хаффмана:\n");
    printHuffmanTree(root, 0);
}

void compressImageMenu() {
    Pixel image[SIZE][SIZE];
    int choice;

    inputImage(image);

    while (1) {
        printf("\nВыберите алгоритм:\n");
        printf("1. RLE\n");
        printf("2. Хаффман\n");
        printf("3. Возврат\n");
        printf("Ваш выбор: ");

        if (scanf_s("%d", &choice) != 1) {
            printf("Ошибка!\n");
            clearInputBuffer();
            continue;
        }

        switch (choice) {
        case 1:
            rleImageCompression(image);
            break;

        case 2:
            huffmanImageCompression(image);
            break;

        case 3:
            return;

        default:
            printf("Неверный ввод\n");
            break;
        }
    }
}

// ==================== Меню ====================

void compressionMenu() {
    int choice;
    while (1) {
        printf("\n=== МЕНЮ СЖАТИЯ ===\n1. Числовая последовательность\n2. Фраза\n3. Рисунок 8x8\n4. Выход\nВаш выбор: ");
        if (scanf_s("%d", &choice) != 1) { printf("Ошибка!\n"); clearInputBuffer(); continue; }
        switch (choice) {
        case 1: compressDigitalSequence(); break;
        case 2: compressPhrase(); break;
        case 3: compressImageMenu(); break;
        case 4: return;
        default: printf("Неверный ввод\n"); break;
        }
    }
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    setlocale(LC_ALL, "");

    int choice;
    while (1) {
        printf("\n=== ГЛАВНОЕ МЕНЮ ===\n1. Сжатие данных\n2. Выход\nВыберите пункт: ");
        if (scanf_s("%d", &choice) != 1) {
            printf("Ошибка!\n");
            clearInputBuffer();
            continue;
        }
        switch (choice) {
        case 1: compressionMenu(); break;
        case 2: return 0;
        default: printf("Неверный ввод\n"); break;
        }
    }
}
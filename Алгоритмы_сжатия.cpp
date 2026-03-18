#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <windows.h>

wchar_t* charToWchar(const char* str);

// Очистка буфера

void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// RLE

void rleCompression(const char* sequence) {
    printf("\n=== Сжатие RLE ===\n");

    if (sequence[0] == '\0')
    {
        printf("Пустая строка!\n");
        return;
    }

    for (int i = 0; sequence[i] != '\0'; i++) {
        if (sequence[i] < '0' || sequence[i] > '9') {
            printf("Ошибка: только цифры!\n");
            clearInputBuffer();
            return;
        }
    }

    int count = 1;
    char prev = sequence[0];

    for (int i = 1; sequence[i] != '\0'; i++) {
        if (sequence[i] == prev)
        {
            count++;
        }
        else
        {
            printf("(%c,%d) ", prev, count);
            prev = sequence[i];
            count = 1;
        }
    }
    printf("(%c,%d)\n", prev, count);  // вывод последней серии
}

// ==================== ХАФФМАН ====================

typedef struct Node {
    wchar_t data;
    int freq;
    struct Node* left;
    struct Node* right;
} Node;

// Создание узла
Node* createNode(wchar_t data, int freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

// Структура для частот
typedef struct {
    wchar_t ch;
    int freq;
} CharFreq;

// Подсчёт частот wchar
int calculateFrequencyW(const wchar_t* str, CharFreq freq[]) {
    int size = 0;

    for (int i = 0; str[i] != L'\0'; i++) {
        int found = -1;

        for (int j = 0; j < size; j++) {
            if (freq[j].ch == str[i]) {
                found = j;
                break;
            }
        }

        if (found != -1) {
            freq[found].freq++;
        }
        else {
            freq[size].ch = str[i];
            freq[size].freq = 1;
            size++;
        }
    }

    return size;
}

// Построение дерева
Node* buildHuffmanTreeW(CharFreq freq[], int size) {
    Node* nodes[1024];

    for (int i = 0; i < size; i++) {
        nodes[i] = createNode(freq[i].ch, freq[i].freq);
    }

    while (size > 1) {
        int min1 = 0, min2 = 1;

        if (nodes[min2]->freq < nodes[min1]->freq) {
            int tmp = min1;
            min1 = min2;
            min2 = tmp;
        }

        for (int i = 2; i < size; i++) {
            if (nodes[i]->freq < nodes[min1]->freq) {
                min2 = min1;
                min1 = i;
            }
            else if (nodes[i]->freq < nodes[min2]->freq) {
                min2 = i;
            }
        }

        Node* parent = createNode(L'\0',
            nodes[min1]->freq + nodes[min2]->freq);
        parent->left = nodes[min1];
        parent->right = nodes[min2];

        if (min1 > min2) {
            int tmp = min1;
            min1 = min2;
            min2 = tmp;
        }

        nodes[min1] = parent;
        nodes[min2] = nodes[size - 1];
        size--;
    }

    return nodes[0];
}

// Хранилище кодов
typedef struct {
    wchar_t ch;
    char code[100];
} HuffCode;

// Генерация кодов
void generateCodesW(Node* root, char* code, int depth, HuffCode codes[], int* size) {
    if (!root) return;

    if (root->left) {
        code[depth] = '0';
        generateCodesW(root->left, code, depth + 1, codes, size);
    }

    if (root->right) {
        code[depth] = '1';
        generateCodesW(root->right, code, depth + 1, codes, size);
    }

    if (!root->left && !root->right) {
        code[depth] = '\0';

        codes[*size].ch = root->data;
        strcpy_s(codes[*size].code, code);
        (*size)++;

        wprintf(L"%lc: %S\n", root->data, code);
    }
}

// Поиск кода символа
const char* findCode(wchar_t ch, HuffCode codes[], int size) {
    for (int i = 0; i < size; i++) {
        if (codes[i].ch == ch)
            return codes[i].code;
    }
    return "";
}

// Кодирование строки
void encodeStringW(const wchar_t* str, HuffCode codes[], int size) {
    printf("\nЗакодированная строка (Хаффман):\n");

    for (int i = 0; str[i] != L'\0'; i++) {
        printf("%s ", findCode(str[i], codes, size));
    }
    printf("\n");
}

// Основная функция
void huffmanCompression(const char* str) {
    printf("\n=== Сжатие Хаффмана ===\n");

    if (!str || str[0] == '\0') {
        printf("Пустая строка!\n");
        return;
    }

    wchar_t* wstr = charToWchar(str);
    if (!wstr) {
        printf("Ошибка конвертации строки.\n");
        return;
    }

    CharFreq freq[1024];
    int freqSize = calculateFrequencyW(wstr, freq);

    wprintf(L"\nЧастоты символов:\n");
    for (int i = 0; i < freqSize; i++) {
        wprintf(L"%lc: %d\n", freq[i].ch, freq[i].freq);
    }

    Node* root = buildHuffmanTreeW(freq, freqSize);

    HuffCode codes[1024];
    int codeSize = 0;
    char code[100];

    printf("\nКоды символов:\n");
    generateCodesW(root, code, 0, codes, &codeSize);

    encodeStringW(wstr, codes, codeSize);

    free(wstr);
}

// KWE
#define MAX_LEXEME_LEN 100
#define MAX_TOKENS 512

typedef struct {
    wchar_t lexeme[MAX_LEXEME_LEN];
    unsigned short token;
} TokenEntry;

typedef struct {
    TokenEntry entries[MAX_TOKENS];
    int size;
    unsigned short nextToken;
} TokenDictionary;

// Добавление лексемы
unsigned short addLexeme(TokenDictionary* dict, const wchar_t* lexeme) {
    for (int i = 0; i < dict->size; i++) {
        if (wcscmp(dict->entries[i].lexeme, lexeme) == 0) {
            return dict->entries[i].token;
        }
    }

    if (dict->size >= MAX_TOKENS) {
        wprintf(L"Словарь переполнен!\n");
        return 0;
    }

    wcsncpy_s(dict->entries[dict->size].lexeme, MAX_LEXEME_LEN, lexeme, _TRUNCATE);
    dict->entries[dict->size].token = dict->nextToken++;

    dict->size++;
    return dict->entries[dict->size - 1].token;
}

// Разделители
int isDelimiterW(wchar_t ch) {
    return iswspace(ch) || iswpunct(ch);
}

// UTF-8 -> wchar
wchar_t* charToWchar(const char* str) {
    if (!str) return NULL;

    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (len == 0) return NULL;

    wchar_t* wstr = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wstr) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);

    return wstr;
}

// KWE сжатие
void kweCompression(const char* phrase) {
    printf("\n=== Сжатие KWE ===\n");

    if (!phrase || phrase[0] == '\0') {
        printf("Пустая строка!\n");
        return;
    }

    wchar_t* wphrase = charToWchar(phrase);
    if (!wphrase) {
        printf("Ошибка конвертации строки.\n");
        return;
    }

    TokenDictionary dict = { 0 };
    dict.nextToken = 1;

    unsigned short tokens[MAX_TOKENS];
    int tokenCount = 0;

    wchar_t buffer[MAX_LEXEME_LEN];
    int bufIndex = 0;

    for (int i = 0; wphrase[i] != L'\0'; i++) {
        wchar_t ch = wphrase[i];

        if (!isDelimiterW(ch)) {
            if (bufIndex < MAX_LEXEME_LEN - 1) {
                buffer[bufIndex++] = ch;
            }
        }
        else {
            if (bufIndex > 0) {
                buffer[bufIndex] = L'\0';

                if (tokenCount < MAX_TOKENS)
                    tokens[tokenCount++] = addLexeme(&dict, buffer);

                bufIndex = 0;
            }

            wchar_t delim[2] = { ch, L'\0' };

            if (tokenCount < MAX_TOKENS)
                tokens[tokenCount++] = addLexeme(&dict, delim);
        }
    }

    if (bufIndex > 0) {
        buffer[bufIndex] = L'\0';

        if (tokenCount < MAX_TOKENS)
            tokens[tokenCount++] = addLexeme(&dict, buffer);
    }

    // Вывод токенов
    wprintf(L"\nЗакодированные токены (16 бит):\n");
    for (int i = 0; i < tokenCount; i++) {
        for (int j = 15; j >= 0; j--) {
            wprintf(L"%d", (tokens[i] >> j) & 1);
        }
        wprintf(L" ");
    }
    wprintf(L"\n");

    // Словарь
    wprintf(L"\nСловарь токенов:\n");
    for (int i = 0; i < dict.size; i++) {
        wprintf(L"%d : %ls\n", dict.entries[i].token, dict.entries[i].lexeme);
    }

    free(wphrase);
}

void compressDigitalSequence()
{
    char sequence[256]; // буфер для ввода цифровой последовательности
    int choice;

    printf("\n=== Сжатие цифровой последовательности ===\n");
    printf("Введите последовательность, например: 0000004455512222\n");
    printf("Ваш ввод: ");

    scanf_s("%255s", sequence, (unsigned)sizeof(sequence));
    printf("\nВы ввели: %s\n", sequence);

    while (1) {
        printf("\nВыберите алгоритм сжатия:\n");
        printf("1. RLE\n");
        printf("2. Хаффман\n");
        printf("3. Возврат в меню сжатия\n");
        printf("Ваш выбор: ");

        if (scanf_s("%d", &choice) != 1) {
            printf("\nОшибка! Введите число.\n");
            clearInputBuffer();
            continue;
        }

        switch (choice) {
        case 1:
            rleCompression(sequence);
            break;

        case 2:
            huffmanCompression(sequence);
            break;

        case 3:
            return;  // возврат в меню сжатия

        default:
            printf("\nОшибка! Неверный ввод.\n");
            break;
        }
    }
}

void compressPhrase() {
    char phrase[1024];
    int choice;
    printf("\nВведите фразу: ");
    clearInputBuffer();
    fgets(phrase, sizeof(phrase), stdin);

    size_t len = strlen(phrase);
    if (len > 0 && phrase[len - 1] == '\n') phrase[len - 1] = '\0';

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

void compressionMenu()
{
    int choice;

    while (1) 
    {
        printf("\n=== МЕНЮ СЖАТИЯ ===\n");
        printf("1. Сжатие цифровой последовательности\n");
        printf("2. Сжатие фразы\n");
        printf("3. Сжатие рисунка (8x8)\n");
        printf("4. Возврат в главное меню\n");
        printf("Выберите пункт: ");

        if (scanf_s("%d", &choice) != 1) {
            printf("\nОшибка! Введите число.\n");
            clearInputBuffer();
            continue;
        }

        switch (choice) {
        case 1:
            compressDigitalSequence();
            break;

        case 2:
            compressPhrase();
            break;

        case 3:
            printf("\nВы выбрали: Сжатие рисунка (8x8)\n");
            // сюда потом вставим логику сжатия изображения 8x8
            break;

        case 4:
            return;  // выход в главное меню

        default:
            printf("\nОшибка! Неверный ввод.\n");
            break;
        }
    }
}

int main() {

	setlocale(LC_ALL, "");

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

	int choice;

	while (1) 
	{
		printf("=== МЕНЮ ===\n");
		printf("1. Сжатие данных\n");
		printf("2. Выход\n");
		printf("Выберите пункт: ");

        if (scanf_s("%d", &choice) != 1) { 
            printf("\nОшибка! Введите число.\n");
            clearInputBuffer();
            continue;
        }

        switch (choice) {
        case 1:
            compressionMenu();
            break;

        case 2:
            printf("\nВыход из программы...\n");
            return 0;

        default:
            printf("\nОшибка! Неверный ввод.\n");
            break;
        }
    }

    return 0;
}

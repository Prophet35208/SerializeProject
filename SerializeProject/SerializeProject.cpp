#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

struct ListNode { // ListNode модифицировать нельзя
    ListNode* prev = nullptr; // указатель на предыдущий элемент или nullptr
    ListNode* next = nullptr;
    ListNode* rand = nullptr; // указатель на произвольный элемент данного списка, либо `nullptr`
    string data; // произвольные пользовательские данные
};

// Структура для временного хранения узла
struct NodeEntry {
    ListNode* node;
    int rand_index;
};

// Создаём список из файла inlet.in
pair<ListNode*, vector<NodeEntry>> BuildListFromFile(const string& filename) {

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Не получилось открыть файл: " << filename << endl;
        return { nullptr, {} };
    }

    // Составим временный список узлов, чтобы легко задать параметр rand для конечных узлов
    // Иначе пришлось бы долго по цепочке двусвязного списка искать нужный индекс
    vector<NodeEntry> entries;

    string line; // Буфер
    int line_num = 0;

    // Читаем файл построчно
    while (getline(file, line)) {
        line_num++;

        stringstream ss(line);
        string data;
        string index_str;

        // Читаем data до точки с запятой
        if (!getline(ss, data, ';')) {
            cerr << "В строке " << line_num << " нет данных. Пропускаем" << endl;
            continue;
        }

        // Читаем rand_index после точки с запятой
        if (!getline(ss, index_str)) {
            cerr << "В строке " << line_num << " нет индекса. Пропускаем" << endl;
            continue;
        }

        // Пробуем преобразовать индекс в число
        try {
            int rand_index = stoi(index_str);

            // Создаем новый узел
            ListNode* new_node = new ListNode();
            new_node->data = data;

            // Связываем с предыдущим узлом (если есть)
            if (!entries.empty()) {
                ListNode* prev_node = entries.back().node;
                prev_node->next = new_node;
                new_node->prev = prev_node;
            }

            // Сохраняем узел и его rand_index
            entries.push_back({ new_node, rand_index });

        }
        catch (const exception& e) {
            cerr << "В строке " << line_num << " не получилось преобразовать индекс '" << index_str << "', пропускаем" << endl;
        }
    }

    file.close();

    // Если не создано ни одного узла
    if (entries.empty()) {
        return { nullptr, {} };
    }

    // Устанавливаем rand ссылки
    size_t node_count = entries.size();
    for (size_t i = 0; i < node_count; ++i) {
        int rand_idx = entries[i].rand_index;

        if (rand_idx >= 0 && rand_idx < static_cast<int>(node_count)) {
            entries[i].node->rand = entries[rand_idx].node;
        }
        // Если rand_idx == -1, rand остается nullptr
    }

    // Возвращаем указатель на голову списка
    return { entries[0].node, entries };
}

// Функция для освобождения памяти списка
void DestroyList(ListNode* head) {
    while (head) {
        ListNode* next = head->next;
        delete head;
        head = next;
    }
}

// Будем использовать vector<NodeEntry> из BuildListFromFile, чтобы не составлять список второй раз.
// Формат бинарного файла следующий
// 
// [4 байта] node_count: количество узлов(uint32_t). Пишем для удобства десериализации
// Далее node_count раз повторяется:
// [4 байта] data_size : размер строки в байтах(uint32_t) 
// [data_size байт] data : строка в UTF - 8 
// [4 байта] rand_index : индекс узла для rand(int32_t, -1 если nullptr)
bool SerializeList(ListNode* head, const vector<NodeEntry>& entries, const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Не удалось создать файл: " << filename << endl;
        return false;
    }

    uint32_t node_count = static_cast<uint32_t>(entries.size());

    // Записываем количество узлов
    file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));

    // Записываем каждый узел (используем готовые entries!)
    for (size_t i = 0; i < entries.size(); ++i) {
        const ListNode* node = entries[i].node;

        // Длина строки в байтах
        uint32_t data_size = static_cast<uint32_t>(node->data.size());
        file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));

        // Сама строка
        file.write(node->data.c_str(), data_size);

        // rand_index берём из entries, зачем делать лишнюю работу
        int32_t rand_index = entries[i].rand_index;
        file.write(reinterpret_cast<const char*>(&rand_index), sizeof(rand_index));
    }

    file.close();
    return true;
}

int main()
{
    setlocale(LC_ALL, "Rus");

    const string input_filename = "inlet.in";
    const string binary_filename = "outlet.out";

    // Строим список из входного файла. Пункт 2 из задания
    auto result = BuildListFromFile(input_filename);  // Получаем pair

    ListNode* head = result.first;                    // Забираем голову списка
    vector<NodeEntry> entries = result.second;        // Забираем entries

    if (!head) {
        cerr << "Не удалось построить список" << endl;
        return 1;
    }

    // Сериализуем список в бинарный файл. Пункт 3 из задания. И на этом конец. 
    // (странно, что про десериализацию в задании не слова, ведь надо как-то данные восстановить из бинарного представления)
    if (!SerializeList(head, entries, binary_filename)) {
        cerr << "Ошибка сериализации" << endl;
        DestroyList(head);
        return 1;
    }

    // Очищаем исходный список
    DestroyList(head);

}

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

// 
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
    cout << "Список сериализован в файл " << filename << endl;
    return true;
}

ListNode* DeserializeList(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл: " << filename << endl;
        return nullptr;
    }

    // Читаем заголовок (кол-во узлов)
    uint32_t node_count;
    file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));

    if (file.gcount() != sizeof(node_count)) {
        cerr << "Ошибка чтения количества узлов" << endl;
        return nullptr;
    }

    if (node_count == 0) {
        return nullptr;
    }

    // Векторы для хранения данных
    vector<ListNode*> nodes(node_count, nullptr);
    vector<int32_t> rand_indices(node_count); // Можем хранить индексы rand в памяти. Даже при 10^6 узлов это не больше, чем 4 Мб оперативы

    // Читаем все узлы 
    for (uint32_t i = 0; i < node_count; ++i) {
        // Читаем размер строки
        uint32_t data_size;
        file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));

        if (file.gcount() != sizeof(data_size)) {
            cerr << "Ошибка чтения размера данных для узла " << i << endl;
            // Очищаем уже созданные узлы при ошибке
            for (uint32_t j = 0; j < i; ++j) delete nodes[j];
            return nullptr;
        }

        // Читаем строку
        string data(data_size, '\0');
        file.read(&data[0], data_size);

        if (static_cast<uint32_t>(file.gcount()) != data_size) {
            cerr << "Ошибка чтения данных для узла " << i << endl;
            // Очищаем уже созданные узлы при ошибке
            for (uint32_t j = 0; j < i; ++j) delete nodes[j];
            return nullptr;
        }

        // Читаем rand_index
        file.read(reinterpret_cast<char*>(&rand_indices[i]), sizeof(int32_t));

        if (file.gcount() != sizeof(int32_t)) {
            cerr << "Ошибка чтения rand_index для узла " << i << endl;
            // Очищаем уже созданные узлы при ошибке
            for (uint32_t j = 0; j < i; ++j) delete nodes[j];
            return nullptr;
        }

        // Создаём узел (сейчас только data, связи prev, next и rand чуть позже)
        nodes[i] = new ListNode();
        nodes[i]->data = move(data);
        // prev, next, rand пока nullptr
    }

    // Устанавливаем связи
    for (uint32_t i = 0; i < node_count; ++i) {
        // Связываем prev/next
        if (i > 0) {
            nodes[i]->prev = nodes[i - 1];
        }
        if (i < node_count - 1) {
            nodes[i]->next = nodes[i + 1];
        }

        // Связываем rand
        int32_t rand_idx = rand_indices[i];
        if (rand_idx >= 0 && rand_idx < static_cast<int32_t>(node_count)) {
            nodes[i]->rand = nodes[rand_idx];
        }
        // Если rand_idx == -1, rand остается nullptr
    }

    file.close();
    cout << "Список десериализован из файла " << filename << endl;

    return nodes[0];
}

// Функция для печати списка. Только для отладки.
void PrintList(ListNode* head) {
    if (!head) {
        cout << "Список пуст" << endl;
        return;
    }

    // Собираем все узлы для определения индексов
    vector<ListNode*> nodes;
    for (ListNode* curr = head; curr; curr = curr->next) {
        nodes.push_back(curr);
    }

    cout << "\nСписок:" << endl;
    for (size_t i = 0; i < nodes.size(); ++i) {
        ListNode* node = nodes[i];

        // Находим индекс rand/ 
        // Приходится считать от начала, до rand узла, чтобы получить номер
        int rand_idx = -1;
        if (node->rand) {
            auto it = find(nodes.begin(), nodes.end(), node->rand);
            if (it != nodes.end()) {
                rand_idx = distance(nodes.begin(), it);
            }
        }

        cout << "Узел " << i << ": \"" << node->data << "\""
            << " [prev: " << (node->prev ? "node " + to_string(i - 1) : "null")
            << ", next: " << (node->next ? "node " + to_string(i + 1) : "null")
            << ", rand: " << (rand_idx != -1 ? "node " + to_string(rand_idx) : "null")
            << "]" << endl;
    }
}

// Сравнение списков для определения правильности работы сериализации. Только для отладки.
bool CompareLists(ListNode* head1, ListNode* head2) {
    if (!head1 && !head2) return true;
    if (!head1 || !head2) return false;

    // Собираем узлы первого списка в вектор
    vector<ListNode*> nodes1;
    for (ListNode* curr = head1; curr; curr = curr->next) {
        nodes1.push_back(curr);
    }

    // Собираем узлы второго списка в вектор
    vector<ListNode*> nodes2;
    for (ListNode* curr = head2; curr; curr = curr->next) {
        nodes2.push_back(curr);
    }

    // Проверяем количество узлов
    if (nodes1.size() != nodes2.size()) {
        cout << "Разное количество узлов: " << nodes1.size() << " vs " << nodes2.size() << endl;
        return false;
    }

    size_t node_count = nodes1.size();

    // Сравниваем каждый узел
    for (size_t i = 0; i < node_count; ++i) {
        ListNode* node1 = nodes1[i];
        ListNode* node2 = nodes2[i];

        // Сравниваем data
        if (node1->data != node2->data) {
            cout << "Узел " << i << ": данные не совпадают: \""
                << node1->data << "\" vs \"" << node2->data << "\"" << endl;
            return false;
        }

        // Проверяем prev связи
        if ((node1->prev == nullptr) != (node2->prev == nullptr)) {
            cout << "Узел " << i << ": prev не совпадает" << endl;
            return false;
        }

        // Проверяем next связи
        if ((node1->next == nullptr) != (node2->next == nullptr)) {
            cout << "Узел " << i << ": next не совпадает" << endl;
            return false;
        }

        // Проверяем rand связи (по индексам)
        int rand_idx1 = -1;
        if (node1->rand) {
            auto it = find(nodes1.begin(), nodes1.end(), node1->rand);
            if (it != nodes1.end()) {
                rand_idx1 = distance(nodes1.begin(), it);
            }
        }

        int rand_idx2 = -1;
        if (node2->rand) {
            auto it = find(nodes2.begin(), nodes2.end(), node2->rand);
            if (it != nodes2.end()) {
                rand_idx2 = distance(nodes2.begin(), it);
            }
        }

        if (rand_idx1 != rand_idx2) {
            cout << "Узел " << i << ": rand не совпадает: "
                << rand_idx1 << " vs " << rand_idx2 << endl;
            return false;
        }
    }

    return true;
}

// Функция сравнения списка из inlet и списка из бинарного файла. Только для отладки.
bool CompareWithDeserialized(ListNode* original_head, const string& binary_filename) {

    // Десериализуем список из бинарного файла
    ListNode* deserialized_head = DeserializeList(binary_filename);
    if (!deserialized_head) {
        return false;
    }

    // Сравниваем списки
    bool is_equal = CompareLists(original_head, deserialized_head);

    if (is_equal) {
        cout << "\nСписки  совпадают";
    }
    else {
        cout << "\n Ошибка: списки не совпадают" << endl;
    }

    // Очищаем десериализованный список
    DestroyList(deserialized_head);

    return is_equal;
}


int main()
{
    setlocale(LC_ALL, "Rus");

    const string input_filename = "inlet.in";
    const string binary_filename = "outlet.out";

    // 1. Строим список из входного файла
    cout << "Читаем список из файла: " << input_filename << endl;
    auto result = BuildListFromFile(input_filename);  // получаем pair

    ListNode* head = result.first;                    // забираем голову списка
    vector<NodeEntry> entries = result.second;        // забираем entries

    if (!head) {
        cerr << "Не удалось построить список" << endl;
        return 1;
    }

    // Сериализуем список в бинарный файл
    cout << "\nСериализуем список в " << binary_filename << endl;
    if (!SerializeList(head, entries, binary_filename)) {
        cerr << "Ошибка сериализации" << endl;
        DestroyList(head);
        return 1;
    }

    // Сравниваем исходный список с десериализованным из бинарного файла
    bool comparison_result = CompareWithDeserialized(head, binary_filename);

    // Очищаем исходный список
    DestroyList(head);

    cout << "\n  Итог " << endl;
    if (comparison_result) {
        cout << "Все тесты пройдены успешно" << endl;
        return 0;
    }
    else {
        cout << "Тесты не пройдены" << endl;
        return 1;
    }
}

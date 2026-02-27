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
ListNode* BuildListFromFile(const string& filename) {

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Не получилось открыть файл: " << filename << endl;
        return nullptr;
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
        return nullptr;
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
    return entries[0].node;
}

// Функция для освобождения памяти списка
void DestroyList(ListNode* head) {
    while (head) {
        ListNode* next = head->next;
        delete head;
        head = next;
    }
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


int main()
{
    setlocale(LC_ALL, "Rus");

    ListNode* head = BuildListFromFile("inlet.in");

    if (!head) {
        cerr << "Не удалось построить список" << endl;
        return 1;
    }

    // Печатаем список 
    PrintList(head);

    // Очищаем память
    DestroyList(head);

    return 0;
}

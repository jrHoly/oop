#include <iostream>
#include <string>
#include <vector>

class Student {
private:
    std::string name;        
    std::string surname;     
    std::string discipline; 
    std::vector<int> scores; 

public:
    // moveчтобы не копировать строки
    Student(std::string n, std::string sur, std::string d)
        : name(std::move(n)), surname(std::move(sur)), discipline(std::move(d)) {}
// добавить оценку
    void push_score(int new_score) {
        if (new_score >= 0 && new_score <= 100) {
            scores.push_back(new_score); 
        } else {
            std::cerr << "оценка идет от 0 до 100 (int)\n";
        }
    }

    // информация о студенте (const тк метод не меняет объект)
    void get_info() const {
        std::cout << "Имя: " << name << "\n"
                  << "Фамилия: " << surname << "\n"
                  << "Дисциплина: " << discipline << "\n"
                  << "Оценки: ";

        for (size_t i = 0; i < scores.size(); ++i) {
            std::cout << scores[i];
            if (i + 1 < scores.size()) std::cout << ", ";
        }
        std::cout << "\n";
    }
};

int main() {
    // вектор хранения нескольких студентов
    std::vector<Student> students;
    students.emplace_back("Jane", "Doe", "OOP");
    students.back().push_score(85);
    students.back().push_score(92);
    students.back().push_score(78);
    students.back().get_info();

    return 0;
}

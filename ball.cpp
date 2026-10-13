#include <iostream>
#include <string>
#include <vector>

class Ball {
    std::string color = "black";
    std::string box = "";
public: 
    Ball(std::string color_) : color(color_) {}

    std::string get_color() {return color; }
    void set_box(std::string box_ ) { box = box_ ;}

    void where() {
        if (box == "") std::cout << "шар " << color << " не в коробке\n";
        else std::cout << "шар " << color << " в коробке " << box << "\n";
    }
}  ;

class Box {
private:
    std::string name = "еее";
    double lenght = 0;        
    double weight = 0;     
    double height = 0;
    std::vector<Ball*> balls;
public:
    Box(double lenght_, double weight_, double height_, std::string name_ = "eee") : 
    lenght(lenght_), weight(weight_), height(height_), name(name_) {};
    
    double get_volume() {return weight * lenght * height; }

    void put(Ball& ball) {
        balls.push_back(&ball);
        ball.set_box(name);
    }

    void print() {
        std::cout << "коробки " << name << ":";
        for (Ball* b : balls) std::cout << " " << b -> get_color();
        std::cout << "\n"; 
    }
};


int main() {
    Box box1(10, 9, 10, "Press F");
    Box box2(10, 10, 10);

        Ball red("red"), green("green"), blue("blue"), white("white");
 
    box1.put(red);
    box1.put(green);
    box2.put(blue);
 
    box1.print();
    box2.print();
 
    red.where();
    green.where();
    blue.where();
    white.where();

    return 0;
}

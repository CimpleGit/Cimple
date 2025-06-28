#include <iostream>
#include <string>
using namespace std;


int main() {
std::cout << "Enter The Number to calculate factorial :" << std::endl;
int a;
cin >> a;
int b=1;
std::cout << a << std::endl;
for(int i=1;i<=a;i++) {
    b=b*i;
}
std::cout << "factorial is :" << b << std::endl;
    return 0;
}

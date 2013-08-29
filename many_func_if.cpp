#include<iostream>

void bar()
{

}

void foo(int i)
{
    if(i>12) {

        std::cout << "What" << std::endl;
    } else {
        std::cout << "ever" << std::endl;
        bar();
    }
}

int i;
int main(int argc, char* argv[])
{
    int j;
    std::cout << "Hello World" << std::endl;
    foo(21);
    return 0;
}

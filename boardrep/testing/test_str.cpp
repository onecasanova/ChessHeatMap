#include <iostream>
#include <string>


using namespace std;


int main() {
    string move = "e2e4";
    
    string start = move.substr(0,2);
    string end = move.substr(2,3);

    cout << "start: " << start << endl;
    cout << "end: " << end << endl;

    return 0;
}
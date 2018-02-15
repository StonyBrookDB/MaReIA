#include <iostream>
using namespace std;

int main() {
    string input_line;                                                         

    while (cin) {
        getline(cin, input_line);
        if (!cin.eof())
	    cout << input_line << endl;
    };

    return 0;
}

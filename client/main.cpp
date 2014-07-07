#include "IO.h"
#include <iostream>
#include <string>
using namespace std;

using namespace holdem;

int main(int argc, char** argv)
{
	if (argc != 3){
		std::cout << "Usage: client <ip> <port>" << endl;
		return 0;
	}

	std::cerr << "Starting client with ip " << argv[1]
			  << " port " << argv[2] << endl;

    IO io(argv[1], argv[2]);

	std::string action;
	std::string message;
	while (true) {
		getline(cin, action);


		if (action[0] == 'r'){
			io.receive(message);
			cout << "[received] " << message << endl;
		}

		else if (action[0] == 'e'){
			cout << "[ending]" << endl;
			break;
		}

		else {
			if (action.empty() || action.back() != '\n')
				action.push_back('\n');
			io.send(action);
		}
	}

	return 0;
}

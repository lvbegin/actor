#include "test.h"

#include <iostream>

void _assert_true(bool e, std::string fileName, unsigned int line)
{

    if (!e)
        throw std::runtime_error("assert failed at line " + std::to_string(line) + " in file " + fileName);
}
#include <sstream>
void _assert_eq(int i, int j, std::string fileName, unsigned int line)
{
	
    if (i != j)
	{	
		std::ostringstream stream;
		stream << i << " != " << j << " : assert failed at line " << line << " in file "  + fileName;
        throw std::runtime_error(stream.str());
	}
}

void _assert_eq(void *i, void *j, std::string fileName, unsigned int line)
{
    if (i != j)
	{	
		std::ostringstream stream;
		stream << i << " != " << j << " : assert failed at line " << line << " in file " + fileName;
        throw std::runtime_error(stream.str());
	}	
}

void _assert_eq(std::string i, std::string j, std::string fileName, unsigned int line) 
{
    if (i != j)
	{	
		std::ostringstream stream;
		stream << i << " != " << j << " : assert failed at line " << line << " in file " + fileName;
        throw std::runtime_error(stream.str());
	}		
}


int _runTest(const _test *suite, size_t nbTests) {
	int nbFailure = 0;

	for (size_t i = 0; i < nbTests; i++) {
		std::cout << suite[i].name << ": " << std::flush;
        try {
		    suite[i].test(); 
			std::cout << "OK" << std::endl;

        }
        catch (std::exception &e) {
			nbFailure ++;
			std::cout << "FAILURE" << std::endl;
			std::cout << "ERROR: " << e.what() << std::endl << std::endl;
		}
	}
	return nbFailure;
}

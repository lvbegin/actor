#ifndef TEST_H__
#define TEST_H__

#include <cstddef>
#include <string>

struct _test{
	const char *name;
	void (*test)(void);
	_test(const char *name, void (*test)(void)) : name(name), test(test) { }
};

#define TEST(t) _test(#t, t)

#define runTest(suite) _runTest(suite, sizeof(suite)/sizeof(_test))

int _runTest(const _test *suite, size_t nbTests);

#define assert_true(e) _assert_true((e),__FILE__,__LINE__)

#define assert_false(e) _assert_true(!(e),__FILE__,__LINE__)

void _assert_true(bool e, std::string fileName, unsigned int line);

#define assert_eq(i, j) _assert_eq((i), (j), __FILE__, __LINE__)

void _assert_eq(int i, int j, std::string fileName, unsigned int line);

void _assert_eq(void *i, void *j, std::string fileName, unsigned int line);

void _assert_eq(std::string i, std::string j, std::string fileName, unsigned int line);

#define assert_exception(e, c)  {\
                                   auto exception_thrown__ = false;\
                                    try { \
									    c;\
								    }\
								    catch (e &) { exception_thrown__ = true; }\
								    if (!exception_thrown__)\
        						        throw std::runtime_error("No exception thrown at line " +\
										std::to_string(__LINE__) + " in file "  __FILE__);\
								}

#include <exception>
#include <sstream>

template<class T>
void _assert_eq(T i, T j, std::string fileName, int line) {
	if (i != j)
	{	
		std::ostringstream stream;
		stream << "assert failed at line " << line << " in file "  + fileName;
        throw std::runtime_error(stream.str());
	}

}
#endif
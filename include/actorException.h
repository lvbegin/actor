#ifndef ACTOR_EXCEPTION_H__
#define ACTOR_EXCEPTION_H__

#include <stdexcept>

class ActorException : public std::runtime_error {
public:
	ActorException(int code, const std::string& what_arg) : std::runtime_error(what_arg), code(code) {}
	int getErrorCode() const { return code; };
private:
	int code;
};

#endif

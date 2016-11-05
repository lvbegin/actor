#ifndef EXCEPTION_H__
#define EXCEPTION_H__

#define THROW(E, message) throw E(std::string(__func__) + std::string(": ") + std::string(message))

#endif

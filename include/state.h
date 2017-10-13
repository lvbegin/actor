#include <string>

class State {
public:
	virtual ~State() = default;

	virtual void init(const std::string &name) = 0;
protected:
	State() = default;
};

class NoState : public State {
public:
	NoState() = default;
	~NoState() = default;
	void init(const std::string &name) { }
};

#ifndef SHARED_VECTOR_H__
#define SHARED_VECTOR_H__

#include <vector>
#include <mutex>
#include <algorithm>

template <typename T>
using FindTest=std::function<bool(const T&)>;

template <typename T>
using VectorIterator= typename std::vector<T>::const_iterator;

template <typename T>
class SharedVector {
public:
	SharedVector() = default;
	~SharedVector() = default;

	SharedVector(const SharedVector &m) = delete;
	SharedVector &operator=(const SharedVector &m) = delete;
	SharedVector(SharedVector &&m) = delete;
	SharedVector &operator=(SharedVector &&m) = delete;

	void push_back(T&& value) {
		std::unique_lock<std::mutex> l(mutex);

		vector.push_back(value);
	}

	void erase(FindTest<T> findTest) {
		std::unique_lock<std::mutex> l(mutex);

		vector.erase(internalFind_if(findTest));
	}

	T find_if(FindTest<T> findTest) const {
		std::unique_lock<std::mutex> l(mutex);

		return *internalFind_if(findTest);
	}
private:
	mutable std::mutex mutex;
	std::vector<T> vector;

	VectorIterator<T> internalFind_if(FindTest<T> findTest) const {
		const auto it = std::find_if(vector.begin(), vector.end(), findTest);
		if (vector.end() == it)
			THROW(std::out_of_range, "Actor not found locally");
		return it;
	}
};

#endif

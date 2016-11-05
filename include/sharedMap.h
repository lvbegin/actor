#ifndef SHARED_MAP_H__
#define SHARED_MAP_H__

#include <exception.h>

#include <map>
#include <mutex>

template<typename K, typename T>
class SharedMap {
public:
	SharedMap() = default;
	~SharedMap() = default;

	template <typename L, typename M>
	void insert(L&& key, M &&value) {
		std::unique_lock<std::mutex> l(mutex);
		if (map.end() != map.find(key))
			THROW(std::runtime_error, "actor already exist.");
		map.insert(std::make_pair(std::forward<L>(key), std::forward<M>(value)));
	}
	void erase(K key) {
		std::unique_lock<std::mutex> l(mutex);
		auto it = map.find(key);
		if (map.end() == it)
			THROW(std::runtime_error, "element to erase does not exist.");
		map.erase(it);
	}
	T find (K key) {
		std::unique_lock<std::mutex> l(mutex);
		auto it = map.find(key);
		if (map.end() == it)
			THROW(std::out_of_range, "element not found.");
		return it->second;
	}
private:
	std::map<K, T> map;
	std::mutex mutex;
};

#endif

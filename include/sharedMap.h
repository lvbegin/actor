#ifndef SHARED_MAP_H__
#define SHARED_MAP_H__

#include <map>
#include <mutex>

template<typename K, typename T>
class SharedMap {
public:
	void insert(K key, T value) {
		std::unique_lock<std::mutex> l(mutex);
		if (map.end() != map.find(key))
			throw std::runtime_error("actorRegistry: actor already exist");
		map.insert(std::pair<K, T>(key, std::move(value)));
	}
	void erase(K key) {
		std::unique_lock<std::mutex> l(mutex);
		map.erase(key);
	}
private:
	std::map<K, T> map;
	std::mutex mutex;
};

#endif

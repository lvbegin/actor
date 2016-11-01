#ifndef SHARED_MAP_H__
#define SHARED_MAP_H__

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
			throw std::runtime_error("actorRegistry: actor already exist");
		map.insert(std::make_pair(std::forward<L>(key), std::forward<M>(value)));
	}
	void erase(K key) {
		std::unique_lock<std::mutex> l(mutex);
		map.erase(key);
	}

	typename std::map<K, T>::iterator find (K key) {
		std::unique_lock<std::mutex> l(mutex);
		return map.find(key);
	}

	typename std::map<K, T>::iterator end() { //should be simplified
		return map.end();
	}

private:
	std::map<K, T> map;
	std::mutex mutex;
};

#endif

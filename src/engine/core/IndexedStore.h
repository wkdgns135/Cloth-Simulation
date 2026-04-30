#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/core/Object.h"

template <typename T>
class IndexedStore
{
public:
	using Storage = std::vector<std::unique_ptr<T>>;

	template <typename U = T, typename... Args>
	U& emplace(Args&&... args)
	{
		static_assert(std::is_base_of_v<T, U>);

		auto object = std::make_unique<U>(std::forward<Args>(args)...);
		U& result = *object;
		insert(std::move(object));
		return result;
	}

	void insert(std::unique_ptr<T> object)
	{
		if (!object)
		{
			return;
		}

		T* raw_object = object.get();
		by_id_[raw_object->id()] = raw_object;
		ordered_.push_back(std::move(object));
	}

	T* find(ObjectId id)
	{
		const auto it = by_id_.find(id);
		return it != by_id_.end() ? it->second : nullptr;
	}

	const T* find(ObjectId id) const
	{
		const auto it = by_id_.find(id);
		return it != by_id_.end() ? it->second : nullptr;
	}

	bool erase(ObjectId id)
	{
		const auto it = by_id_.find(id);
		if (it == by_id_.end())
		{
			return false;
		}

		ordered_.erase(
			std::remove_if(ordered_.begin(), ordered_.end(), [id](const std::unique_ptr<T>& candidate) {
				return candidate && candidate->id() == id;
			}),
			ordered_.end());
		by_id_.erase(it);
		return true;
	}

	bool erase(const T* object)
	{
		return object ? erase(object->id()) : false;
	}

	void clear()
	{
		ordered_.clear();
		by_id_.clear();
	}

	std::size_t size() const { return ordered_.size(); }
	bool empty() const { return ordered_.empty(); }

	Storage& ordered() { return ordered_; }
	const Storage& ordered() const { return ordered_; }

private:
	Storage ordered_;
	std::unordered_map<ObjectId, T*> by_id_;
};

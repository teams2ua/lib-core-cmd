#pragma once
#include "api/EventReceiver.hpp"
#include "api/ErrorCode.hpp"
#include "api/Error.hpp"
#include "api/Event.hpp"
#include "api/EventCode.hpp"
#include "api/Operation.hpp"
#include <mutex>

using namespace ledger::core;

#define GET_SYNC(type, operation) sync<api::type ## Callback, api::type>([&](std::shared_ptr<api::type ## Callback> & callback) { operation; });

class EventReceiver : public api::EventReceiver {
public:
	void onEvent(const std::shared_ptr<api::Event> & incomingEvent) override {
		std::lock_guard<std::mutex> guard(_lock);
		if (incomingEvent->getCode() == api::EventCode::SYNCHRONIZATION_FAILED) {
			_error = api::Error(api::ErrorCode::UNKNOWN, "Synchronization failed");
			_cond.notify_all();
			return;
		}
		if (incomingEvent->getCode() == api::EventCode::SYNCHRONIZATION_SUCCEED ||
			incomingEvent->getCode() == api::EventCode::SYNCHRONIZATION_SUCCEED_ON_PREVIOUSLY_EMPTY_ACCOUNT) {
			done = true;
			_cond.notify_all();
		}
	}

	void wait() {
		std::unique_lock<std::mutex> lk(_lock);
		_cond.wait(lk, [&] { return _error || done; });
		if (done)
			return;
		throw std::runtime_error(_error.value().message);
	}
private:
	std::experimental::optional<api::Error> _error;
	bool done{ false };
	std::mutex _lock;
	std::condition_variable _cond;
};

template<typename CallbackType, typename ParameterType>
class Callback : public CallbackType {
public:

	void onCallback(const ParameterType & value, const std::experimental::optional<api::Error> & error) override {
		std::lock_guard<std::mutex> guard(_lock);
		if (error) {
			_error = error;
		}
		else {
			_result = value;
		}
		_cond.notify_all();
	}

	ParameterType WaitForResult() {
		std::unique_lock<std::mutex> lk(_lock);
		_cond.wait(lk, [&] { return _error || _result; });
		if (_result)
			return _result;
		throw std::runtime_error(_error.value().message);
	}
private:
	std::experimental::optional<api::Error> _error;
	ParameterType _result;
	std::mutex _lock;
	std::condition_variable _cond;
};

template<typename CallbackType, typename ParameterType>
std::shared_ptr<ParameterType> sync(std::function<void(std::shared_ptr<CallbackType>)> operation) {
	auto cb = std::make_shared<Callback<CallbackType, std::shared_ptr<ParameterType>>>();
	operation(cb);
	return cb->WaitForResult();
}

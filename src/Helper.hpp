#pragma once
#include "api/EventReceiver.hpp"
#include "api/ErrorCode.hpp"
#include "api/Error.hpp"
#include "api/Event.hpp"
#include "api/EventCode.hpp"
#include "api/Operation.hpp"
#include <mutex>

#define GET_SYNC(type, operation) sync<ledger::core::api::type ## Callback, std::shared_ptr<ledger::core::api::type> >([&](std::shared_ptr<ledger::core::api::type ## Callback> & callback) { operation; });
#define GET_SYNC_LIST(type, operation) sync<ledger::core::api::type ## ListCallback, std::experimental::optional<std::vector<std::shared_ptr<ledger::core::api::type>>>> ([&](std::shared_ptr<ledger::core::api::type ## ListCallback>& callback) { operation; });

class EventReceiver : public ledger::core::api::EventReceiver {
public:
	void onEvent(const std::shared_ptr<ledger::core::api::Event> & incomingEvent) override {
		std::lock_guard<std::mutex> guard(_lock);
		if (incomingEvent->getCode() == ledger::core::api::EventCode::SYNCHRONIZATION_FAILED) {
			_error = ledger::core::api::Error(ledger::core::api::ErrorCode::UNKNOWN, "Synchronization failed");
			_cond.notify_all();
			return;
		}
		if (incomingEvent->getCode() == ledger::core::api::EventCode::SYNCHRONIZATION_SUCCEED ||
			incomingEvent->getCode() == ledger::core::api::EventCode::SYNCHRONIZATION_SUCCEED_ON_PREVIOUSLY_EMPTY_ACCOUNT) {
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
	std::experimental::optional<ledger::core::api::Error> _error;
	bool done{ false };
	std::mutex _lock;
	std::condition_variable _cond;
};

template<typename CallbackType, typename ParameterType>
class Callback : public CallbackType {
public:

	void onCallback(const ParameterType & value, const std::experimental::optional<ledger::core::api::Error> & error) override {
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
	std::experimental::optional<ledger::core::api::Error> _error;
	ParameterType _result;
	std::mutex _lock;
	std::condition_variable _cond;
};

template<typename CallbackType, typename ParameterType>
ParameterType sync(std::function<void(std::shared_ptr<CallbackType>)> operation) {
	auto cb = std::make_shared<Callback<CallbackType, ParameterType>>();
	operation(cb);
	return cb->WaitForResult();
}

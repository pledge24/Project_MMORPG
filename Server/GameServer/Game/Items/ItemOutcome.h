#pragma once

/** 아이템 요청을 거절한 사유. ItemOutcome으로 바뀌므로 판정 함수가 그대로 돌려준다. */
struct ItemRejection
{
    Protocol::ItemResult result = Protocol::ITEM_RESULT_INTERNAL_ERROR;
};

/**
 * 아이템 요청 하나의 판정.
 *
 * 받아들였으면 결과를, 거절했으면 사유를 담는다. 사유는 응답의 결과 enum으로 클라이언트에 그대로 간다.
 */
template<typename T>
class ItemOutcome
{
public:
    ItemOutcome(T value) : _value(std::move(value)) {}
    ItemOutcome(ItemRejection rejection) : _result(rejection.result) {}

    bool IsSuccess() const { return _value.has_value(); }
    /** 받아들였으면 ITEM_RESULT_SUCCESS. */
    Protocol::ItemResult GetResult() const { return _result; }

    /** 받아들였을 때만 부른다. */
    T& operator*() { return _value.value(); }
    const T& operator*() const { return _value.value(); }
    T* operator->() { return &_value.value(); }
    const T* operator->() const { return &_value.value(); }

private:
    Protocol::ItemResult _result = Protocol::ITEM_RESULT_SUCCESS;
    optional<T> _value;
};

#pragma once

#define ASIO_STANDALONE 
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS
#define ASIO_HAS_STD_ATOMIC

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <stdint.h>
#include <memory>

enum class MessageType: uint32_t {
    Server_GetPing
};

template <typename T>
struct message {

};


// Forward declare the connection
template <typename T>
class connection;

template <typename T>
struct owned_message
{
    std::shared_ptr<connection<T>> remote = nullptr;
    message<T> msg;

    // Again, a friendly string maker
    friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
    {
        os << msg.msg;
        return os;
    }
};


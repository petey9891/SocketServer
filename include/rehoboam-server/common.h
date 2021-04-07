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
#include <vector>

enum MessageType: uint32_t {
    Success,

    ServerPing,

    CubeDisplayOnOff,
    CubeBrightness,
    CubePulse,
    CubeRehoboam
};

template <typename T>
struct MessageHeader {
    T id {};
    uint32_t size = 0;
    MessageType type;
};
template <typename T>
struct Message {
    MessageHeader<T> header {};
    std::vector<uint8_t> body;

    // Returns the size of the entire message body in bytes.
    size_t size() const {
        return this->body.size();
    }

    // Pushes data into the message buffer
    template <typename DataType>
    friend Message<T>& operator << (Message<T>& msg, const DataType& data) {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

        // Save the current size of the body, this will be the point where we save the new data
        size_t currentBodySize = msg.body.size();

        // Resize the body vector
        msg.body.resize(msg.body.size() + sizeof(DataType));

        // Copy the data into the new space
        std::memcpy(msg.body.data() + currentBodySize, &data, sizeof(DataType));

        // Recalculate the message size
        msg.header.size = msg.size();

        return msg;
    }

    // Pops data from the message buffer
    template <typename DataType>
	friend Message<T>& operator >> (Message<T>& msg, DataType& data) {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");

        // Cache the location towards the end of the vector where the pulled data starts
        size_t i = msg.body.size() - sizeof(DataType);

        // Physically copy the data from the vector into the user variable
        std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

        // Shrink the vector to remove read bytes, and reset end position
        msg.body.resize(i);

        // Recalculate the message size
        msg.header.size = msg.size();

        // Return the target message so it can be "chained"
        return msg;
    }
};


// Forward declare the connection
template <typename T>
class connection;

/**
 * Owned Messages are identical to regular messages, however, they are associated with a conneciton. 
 * On the server, the owner would be the client that sent the message and visa versa.
 */
template <typename T>
struct OwnedMessage
{
    std::shared_ptr<connection<T>> remote = nullptr;
    Message<T> message;
};


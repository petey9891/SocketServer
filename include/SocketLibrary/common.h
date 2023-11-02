#pragma once

// Define ASIO standalone mode and required features.
#define ASIO_STANDALONE 
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS
#define ASIO_HAS_STD_ATOMIC

// Including required headers from ASIO library for networking and SSL support.
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/ssl.hpp>

// Including standard library headers for fixed-width integer types, memory management, and dynamic arrays.
#include <stdint.h>
#include <memory>
#include <vector>

#include "logging.h"

namespace Rehoboam {
    namespace SocketLibrary {

        // Enum defining various message types for network communication.
        enum MessageType: uint32_t {
            Success,            // Indicating a successful operation.
            ServerPing,         // Server ping message.
            ServerShutdown,     // Notification for server shutdown.
            CubeDisplayToggle,  // Command to toggle the display of a cube.
            CubeBrightness,     // Command to change the brightness of a cube.
            CubeDisplayChange   // Command to change the display mode of a cube.
        };

        // Enum defining the types of clients that can connect to the server.
        enum ClientType: uint8_t {
            WEB,  // A web-based client.
            CUBE  // A cube hardware client.
        };

        // Struct representing the header of a network message, containing metadata about the message.
        template <typename T>
        struct MessageHeader {
            T id {};                   // Unique identifier for the message.
            uint32_t size = 0;         // Size of the message body in bytes.
            MessageType type;          // Type of the message.
        };

        // Struct representing a network message with a header and a body.
        template <typename T>
        struct Message {
            MessageHeader<T> header {};             // Header of the message.
            std::vector<uint8_t> body;              // Body of the message, containing the actual data.

            // Returns the size of the entire message body in bytes.
            size_t size() const {
                return this->body.size();
            }

            // Clears the message, resetting the header and emptying the body.
            void clear() {
                this->header = {};
                this->body.clear();
            }

            // Pushes data into the message buffer.
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

            // Pops data from the message buffer.
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

        // Forward declaration of the SocketConnection template class.
        template <typename T>
        class SocketConnection;

        /**
         * @brief Represents a message owned by a particular connection.
         *
         * Owned Messages are identical to regular messages, however, they are associated with a connection.
         * On the server, the owner would be the client that sent the message, and vice versa on the client side.
         */
        template <typename T>
        struct OwnedMessage {
            std::shared_ptr<SocketConnection<T>> remote = nullptr;  // Pointer to the owning connection.
            Message<T> message;                                       // The owned message.
        };
    }  // namespace SocketLibrary
}  // namespace Rehoboam
